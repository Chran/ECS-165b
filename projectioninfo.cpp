#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdio>
#include <cctype>

#include "HW4-expr.h"
#include "expression.h"

using namespace std;

class ProjectionInfo{
public:
    vector<string> tableNames;
    expression* selectSubTree;
    vector<expression *> subTrees;
    map<string, string> projectionAliasMap;

    string lastJoinFile;
    string indexOne;
    string indexTwo;

    vector <string> whereClauseColumns;
    vector <string> joinedTables;
    vector <string> allWhereColumns;
    bool validQuery;

    void checkForColumns(expression* treeNode){
        if(!treeNode || treeNode->func == OP_STRING || treeNode->func == OP_NUMBER) { 
            return; 
        }
        else if(treeNode->func == OP_COLNAME){
            allWhereColumns.push_back(treeNode->values[0].name);
            return;
        }
        else{
            checkForColumns(treeNode->values[0].ep);
            checkForColumns(treeNode->values[1].ep);
        }
    }

    vector<string> getAllTableNames(){
        vector<string>allTableNames;
        for(auto it = createTableInfo.tableInfo.begin(); it != createTableInfo.tableInfo.end(); it++){
            allTableNames.push_back(it->first);
        }
        return allTableNames;
    }

    vector<string> getQueryHeaders(vector<string>tableNames){
        vector<string> queryHeaders;
        for(int i = 0; i < tableNames.size(); i++){
            vector<string>temp = createTableInfo.tableInfo[tableNames[i] + ".tbl"];
            for(int j = 0; j < temp.size(); j++){
                transform(temp[j].begin(), temp[j].end(), temp[j].begin(), ::tolower);
                queryHeaders.push_back(temp[j]);
            }
        }
        return queryHeaders;
    }

    bool isValidQuery(){
        vector<string>allTableNames = getAllTableNames();
        for(int i = 0; i < tableNames.size(); i++){
            vector<string>::iterator it = find (allTableNames.begin(), allTableNames.end(), tableNames[i] +".tbl");
            if(it == allTableNames.end()){
                cout << "Table name: " << tableNames[i] << " does not exist" << endl;
                validQuery = false;
                Clear();
                return false;
            }
        }

        //If it gets here then tables exist!
        vector<string> queryHeaders = getQueryHeaders(tableNames);
        for(auto it2 = projectionAliasMap.begin(); it2 != projectionAliasMap.end(); it2++){
            if(find(queryHeaders.begin(), queryHeaders.end(), it2->first) == queryHeaders.end()){
                cout << "Column " << it2->first << " does not exist (SELECT Clause)" << endl;
                validQuery = false;
                Clear();
                return false;
            }
        }

        for(int i = 0; i < whereClauseColumns.size(); i++){
            if(find(queryHeaders.begin(), queryHeaders.end(), whereClauseColumns[i]) == queryHeaders.end()){
                cout << "Column " << whereClauseColumns[i] << " does not exist (WHERE Clause)" << endl;
                validQuery = false;
                Clear();
                return false;
            }
            break;
        }

        validQuery = true;
        return true;
    }

    void TableParse(expr e){
        expression *ep = (expression *)e;

        if(!ep || ep->func == OP_STRING || ep->func == OP_COLNAME || ep->func == OP_NUMBER) { 
            return; 
        }

        if(ep->func == OP_TABLENAME){
                tableNames.push_back(ep->values[0].name);
                return;
        }

        TableParse(ep->values[0].ep);
        TableParse(ep->values[1].ep);
    }

    void SelectParse(expr e){
        expression *ep = (expression *)e;

        while(ep){
            string originalColumnName(ep->values[0].ep->values[0].ep->values[0].name); //Original colname
            string aliasColumnName(ep->values[0].ep->values[1].ep->values[0].name); //ALIAS colname

            projectionAliasMap[originalColumnName] = aliasColumnName;
            ep = ep->values[1].ep;
        } //only go right because the node we're inspecting should be OP_RLIST
    }

    void PrintProjectionTables(){
        cout<< "Printing projection tables"<<endl;
        for(auto tblnames: tableNames){
            cout<<tblnames<<",";
        }
    }

    void Parse(expr e){
      expression *ep = (expression *)e;
      
      SelectParse(ep->values[1].ep);
      switch(ep->values[0].ep->func){
        case OP_TABLENAME:
        case OP_PRODUCT: 
            TableParse(ep->values[0].ep);
            break;
        case OP_SELECTION:
            TableParse(ep->values[0].ep->values[0].ep);
            selectSubTree = ep->values[0].ep->values[1].ep;
            SplitSubTrees(ep->values[0].ep->values[1].ep);
            for(auto tree : subTrees){
                checkForColumns(tree);
            }
            break;
      }
      isValidQuery();  
      // PrintProjectionTables();
    }
    //select a from b where (c+d) * (e+f) = g AND h-i<j;

    void Clear(){
        allWhereColumns.clear();
        whereClauseColumns.clear();
        joinedTables.clear();
        subTrees.clear();
        projectionAliasMap.clear();
        tableNames.clear();
        indexOne = "";
        indexTwo = "";
        lastJoinFile = "";
        remove("./data/hash.JOIN");
        remove("./data/index.JOIN");
        remove("./data/nestedloop.JOIN");
        //GET RID OF .JOIN FILES
    }

    void removeTableFromVector(string tableNameToRemove){
        tableNames.erase(remove(tableNames.begin(), tableNames.end(), tableNameToRemove), tableNames.end());
    }

    void GetWhereColumns(expression* treeNode){
        // cout<<"Checking which Columns are referenced in WHERE clause"<<endl;
        if(treeNode->func == OP_NUMBER || treeNode->func == OP_STRING){
            return;
        }
        else if(treeNode->func == OP_COLNAME){
            if(find(whereClauseColumns.begin(), whereClauseColumns.end(), treeNode->values[0].name) == whereClauseColumns.end()){
                whereClauseColumns.push_back(treeNode->values[0].name);
                cout<<"Column found: "<< treeNode->values[0].name<<endl;
            }
            return;
        }
        else{
            GetWhereColumns(treeNode->values[0].ep);
            GetWhereColumns(treeNode->values[1].ep);
        }
    }//for each WHERE clause, push in all the columns involved
    //this is used to check if tables are cross referenced (i.e: tbl1.a = tbl2.b)

    void CheckForJoinType(){
        for(auto whereColumnName: whereClauseColumns){
            for(auto& table: createTableInfo.tableInfo){
                for(auto& tableColumnName : table.second){

                    if(boost::iequals(whereColumnName,tableColumnName)){
                        if(find(joinedTables.begin(), joinedTables.end(), table.first) == joinedTables.end()){
                            joinedTables.push_back(table.first);
                        }//check the joinedtables vector to see if the column's table is already in it
                    }//when we find that column's corresponding table (which it belongs in)

                }//check each column's name within each table to find a match

            }//for each column used in where clause, check which table they belong to

        }//for each column used in the where clause

        if(joinedTables.size() == 2){
            for(auto columnName : whereClauseColumns){

                for(auto& table : indexInfo.stringIndexTable){
                    for(auto& indexFile: table.second){
                        if(boost::iequals(columnName,indexFile.first)){
                            if(indexOne.empty()){
                                indexOne = table.first + "." + indexFile.first + ".string";
                                //the column name is the index file's name (ALL CAPS if primary key, and just first letter caps if the column is a key)
                            } //append ".string" so we know which index table we're accessing
                            else{
                                indexTwo = table.first + "." + indexFile.first + ".string"; 
                                goto doneCheckingIndex;
                            }
                        }
                    }
                }

                for(auto& table : indexInfo.floatIndexTable){
                    for(auto& indexFile: table.second){
                        if(boost::iequals(columnName,indexFile.first)){
                            if(indexOne.empty()){
                                indexOne = table.first + "." + indexFile.first + ".float";
                                //the column name is the index file's name (ALL CAPS if primary key, and just first letter caps if the column is a key)
                            } //append ".float" so we know which index table we're accessing
                            else{
                                indexTwo = table.first + "." + indexFile.first + ".float"; 
                                goto doneCheckingIndex;
                            }
                        }
                    }
                }

            }//check if any of the columns in that where clause has an index (check is only done if the columns belong to different tables)
        }

        doneCheckingIndex:
        return;
    }//checks the whereClauseColumns Vector to see if there are cross referenced tables
    //assumes that each table has unique column names (because dot operator won't be supported)

    void writeResultsToJoinFile(string joinType, map<string, vector<string> > tablesJoined){
        ofstream joinTableFileOut("./data/" + joinType + ".JOIN", ios::out);

        int rowAmount;

        for(map<string,vector<string> >::iterator iter = tablesJoined.begin(); iter != tablesJoined.end(); ++iter){
            if(distance(iter,tablesJoined.end()) == 1){
                joinTableFileOut << iter->first <<endl;
                break;
            }
            joinTableFileOut << iter->first << "\t";
            rowAmount = tablesJoined[iter->first].size();
        }

        int rowNumber = 0;
        for(rowNumber; rowNumber < rowAmount; rowNumber++){
            for(map<string,vector<string> >::iterator iter = tablesJoined.begin(); iter != tablesJoined.end(); ++iter){
                if(distance(iter,tablesJoined.end()) == 1){
                    joinTableFileOut << (iter->second)[rowNumber];
                    break;
                }
                joinTableFileOut << (iter->second)[rowNumber]<< "\t";
            }
            joinTableFileOut << endl;
        }

    }

    void IndexJoin(){
        //Map of new joined rows after Index Join
        map<string, vector<string> > indexJoinedRows;
        //Vectors to hold parsed index
        vector<string> indexTableNameOne;
        vector<string> indexTableNameTwo;
        //Parsing indexOne and indexTwo into vectors
        boost::split(indexTableNameOne, indexOne, boost::algorithm::is_any_of("."), boost::token_compress_on); 
        boost::split(indexTableNameTwo, indexTwo, boost::algorithm::is_any_of("."), boost::token_compress_on); 
        //Initialize potentially useful variables
        string tableNameOne = indexTableNameOne[0];
        string tableNameTwo = indexTableNameTwo[0];
        string indexColOne = indexTableNameOne[1];
        string indexColTwo = indexTableNameTwo[1];
        vector<string>indexOneHeaders = createTableInfo.tableInfo[tableNameOne + ".tbl"];
        vector<string>indexTwoHeaders = createTableInfo.tableInfo[tableNameTwo + ".tbl"];

        //Write the headers of the new indexjoin file
        for(int i = 0 ; i < indexOneHeaders.size(); i++){
            vector<string>temp;
            indexJoinedRows[indexOneHeaders[i]] = temp;
        }
        for(int i = 0 ; i < indexTwoHeaders.size(); i++){
            vector<string>temp;
            indexJoinedRows[indexTwoHeaders[i]] = temp;
        }


        cout<< endl << "Doing an INDEX JOIN between " << joinedTables[1] << " and " <<joinedTables[0] <<" (has index) ON " << whereClauseColumns[0] << " == "<<whereClauseColumns[1] << endl;

        //Use string btreemap
        if(indexTableNameOne[2] == "string"){
            map<string, btree::btree_multimap<string, vector<string> > > tableOneIndices;
            tableOneIndices = indexInfo.stringIndexTable[tableNameOne];
            map<string, btree::btree_multimap<string, vector<string> > > tableTwoIndices;
            tableTwoIndices = indexInfo.stringIndexTable[tableNameTwo];
            btree::btree_multimap<string, vector<string> > indexOneBTree;
            indexOneBTree = tableOneIndices[indexColOne];
            btree::btree_multimap<string, vector<string> > indexTwoBTree;
            indexTwoBTree = tableOneIndices[indexColTwo];

            //for(iterate through keys in multimap one)
                //for()
            for(auto it = indexOneBTree.begin(), end = indexOneBTree.end(); it!=end; it = indexOneBTree.upper_bound(it->first)){
                if(indexInfo.stringIndexTable[tableNameTwo][indexColTwo].count(it->first) >=1){
                    for(auto mm1 = indexInfo.stringIndexTable[tableNameOne][indexColOne].equal_range(it->first).first; mm1 != indexInfo.stringIndexTable[tableNameOne][indexColOne].equal_range(it->first).second; mm1++){
                        for(auto mm2 = indexInfo.stringIndexTable[tableNameTwo][indexColTwo].equal_range(it->first).first; mm2 != indexInfo.stringIndexTable[tableNameTwo][indexColTwo].equal_range(it->first).second; mm2++){
                            for(int i = 0; i < mm1->second.size(); i++){
                                indexJoinedRows[indexOneHeaders[i]].push_back(mm1->second[i]);
                            }

                            for(int i = 0; i < mm2->second.size(); i++){
                                indexJoinedRows[indexTwoHeaders[i]].push_back(mm2->second[i]);
                            }
                        }
                    }
                }
            }
        }
        //Use float btreemap
        else{
            map<string, btree::btree_multimap<float, vector<string> > > tableOneIndices;
            tableOneIndices = indexInfo.floatIndexTable[tableNameOne];
            map<string, btree::btree_multimap<float, vector<string> > > tableTwoIndices;
            tableTwoIndices = indexInfo.floatIndexTable[tableNameTwo];
            btree::btree_multimap<float, vector<string> > indexOneBTree;
            indexOneBTree = tableOneIndices[indexColOne];
            btree::btree_multimap<float, vector<string> > indexTwoBTree;
            indexTwoBTree = tableOneIndices[indexColTwo];

            //for(iterate through keys in multimap one)
                //for()
            for(auto it = indexOneBTree.begin(), end = indexOneBTree.end(); it!=end; it = indexOneBTree.upper_bound(it->first)){
                if(indexInfo.floatIndexTable[tableNameTwo][indexColTwo].count(it->first) >=1){
                    for(auto mm1 = indexInfo.floatIndexTable[tableNameOne][indexColOne].equal_range(it->first).first; mm1 != indexInfo.floatIndexTable[tableNameOne][indexColOne].equal_range(it->first).second; mm1++){
                        for(auto mm2 = indexInfo.floatIndexTable[tableNameTwo][indexColTwo].equal_range(it->first).first; mm2 != indexInfo.floatIndexTable[tableNameTwo][indexColTwo].equal_range(it->first).second; mm2++){
                            for(int i = 0; i < mm1->second.size(); i++){
                                indexJoinedRows[indexOneHeaders[i]].push_back(mm1->second[i]);
                            }

                            for(int i = 0; i < mm2->second.size(); i++){
                                indexJoinedRows[indexTwoHeaders[i]].push_back(mm2->second[i]);
                            }
                        }
                    }
                }
            }
        }
        
        removeTableFromVector(tableNameOne);
        removeTableFromVector(tableNameTwo);
        writeResultsToJoinFile("index", indexJoinedRows);
    }

    void NestedLoopJoin(string tableOne, string tableTwo){
        //Create temporary file to hold Cartesian Product
        ofstream CPTable;
        CPTable.open("./data/temp.JOIN");

        //Create map of column names and vector of values for the new table
        map<string, vector<string> > NLJMap;

        ifstream tableOneFile("./data/"+tableOne, ios::in);
        ifstream tableTwoFile("./data/"+tableTwo, ios::in);
        string tableOneFileLine;
        string tableTwoFileLine;

        //Declare vector of header names for CP table
        vector<string> CPHeaders;
        vector<string>tempHeaders;
        vector<string>tableOneTemp;

        //Store headers of each table into CPHeaders vector/ Disregard the headers from the file.
        getline(tableOneFile,tableOneFileLine);
        boost::split(tableOneTemp,tableOneFileLine, boost::algorithm::is_any_of("\t"), boost::token_compress_on);
        boost::split(CPHeaders,tableOneFileLine, boost::algorithm::is_any_of("\t"), boost::token_compress_on);
        getline(tableTwoFile, tableTwoFileLine);
        boost::split(tempHeaders,tableTwoFileLine, boost::algorithm::is_any_of("\t"), boost::token_compress_on);

        //Add the tabletwo headers into CPHeaders
        for(int i = 0; i < tempHeaders.size(); i++){
            CPHeaders.push_back(tempHeaders[i]);
        }

        for(int i = 0; i < CPHeaders.size(); i++){
            vector<string>valueVector;
            NLJMap[CPHeaders[i]] = valueVector;
        }

        //Temporary vectors to hold values of the tuples of each table
        vector<string> tableOneTuple;
        vector<string> tableTwoTuple;

        //Output the headers onto the temp file
        CPTable << tableOneFileLine << "\t";
        CPTable << tableTwoFileLine << "\n";

        //Main loop
        while(getline(tableOneFile,tableOneFileLine)){
            tableOneTuple.clear();
            tableTwoTuple.clear();
            boost::split(tableOneTuple, tableOneFileLine, boost::algorithm::is_any_of("\t"), boost::token_compress_on);
            while(getline(tableTwoFile, tableTwoFileLine)){
                boost::split(tableTwoTuple, tableTwoFileLine, boost::algorithm::is_any_of("\t"), boost::token_compress_on);
                int temp = 0;
                for(int i = 0; i < tableOneTuple.size(); i++){
                    map<string, vector<string> >::iterator NLJoinIter = NLJMap.find(CPHeaders[i]);
                    NLJoinIter->second.push_back(tableOneTuple[i]);
                    temp++;
                }
                for(int i = 0; i < tableTwoTuple.size(); i++){
                    map<string, vector<string> >::iterator NLJoinIter2 = NLJMap.find(CPHeaders[i+temp]);
                    NLJoinIter2->second.push_back(tableTwoTuple[i]);
                }
                CPTable << tableOneFileLine << "\t";
                CPTable << tableTwoFileLine << "\n";
            }
            tableTwoFile.close();
            tableTwoFile.open("./data/"+tableTwo, ios::in);
            getline(tableTwoFile, tableTwoFileLine);
        }

        CPTable.close();
        ifstream tempJoinFile("./data/temp.JOIN", ios::in);
        ofstream nestedLoopJoinFile("./data/nestedloop.JOIN", ios::out);
        string tempJoinLine;

        while(getline(tempJoinFile, tempJoinLine)){
            nestedLoopJoinFile << tempJoinLine <<endl;
        }
        remove("./data/temp.JOIN");

        vector<string> tempTableName1;
        vector<string> tempTableName2;
        boost::split(tempTableName1, tableOne,boost::algorithm::is_any_of("."), boost::token_compress_on); 
        boost::split(tempTableName2, tableTwo,boost::algorithm::is_any_of("."), boost::token_compress_on); 
        removeTableFromVector(tempTableName1[0]);
        removeTableFromVector(tempTableName2[0]);
    }


    // void PrintJoinMap(map <string, vector<string> > joinMap){
    // for(auto& i:joinMap){  
    //   cout<<"This NLJoin map column name is : "<< i.first<<endl;
    //   cout <<"These are the column's values: ";
    //   for(auto& j: i.second){
    //     cout << j << " ";
    //   }
    //   cout << endl;
    // }
    // }


    void HashJoin(){
        vector<string> indexTableName;
        map<string, vector<string> > hashJoinedRows;
        boost::split(indexTableName, indexOne, boost::algorithm::is_any_of("."), boost::token_compress_on); 
        cout<< "table 1: "<< joinedTables[0] << " table 2: " << joinedTables[1] <<endl;

        vector<string> tempTableName1;
        vector<string> tempTableName2;
        boost::split(tempTableName1, joinedTables[0],boost::algorithm::is_any_of("."), boost::token_compress_on); 
        boost::split(tempTableName2, joinedTables[1],boost::algorithm::is_any_of("."), boost::token_compress_on); 
        removeTableFromVector(tempTableName1[0]);
        removeTableFromVector(tempTableName2[0]);

        //indexTableName: [0] Tblname, [1] Columnname, [2] datatype
        if(indexTableName[2] == "string"){
            ifstream inputTable;

            if(indexTableName[0]+ ".tbl" == joinedTables[0]){
                cout<< endl << "Doing a HASH JOIN between the " << joinedTables[1] << " and " <<joinedTables[0] <<" (has index) ON " << whereClauseColumns[0] << " == "<<whereClauseColumns[1] << endl;
                inputTable.open("./data/" + joinedTables[1], ios::in);
            }
            else{
                cout<< endl << "Doing a HASH JOIN between the " << joinedTables[0] << " and " <<joinedTables[1] <<" (has index) ON " << whereClauseColumns[0] << " == "<<whereClauseColumns[1] << endl;
                inputTable.open("./data/" + joinedTables[0], ios::in);

            }

            string tableFileLine;
            vector<string> tableFileLineValues;
            map<string, string> tableRowTuple;
            //key is column name, value is that column's value
            string tempHeaders;
            vector <string> tableHeaders;

            getline(inputTable, tempHeaders);
            //get the table headers;
            boost::split(tableHeaders, tempHeaders, boost::algorithm::is_any_of("\t"), boost::token_compress_on);
            int checkedColumn;

            for(int i = 0; i< tableHeaders.size(); i++){
                if(boost::iequals(tableHeaders[i], whereClauseColumns[0]) || boost::iequals(tableHeaders[i], whereClauseColumns[1])){ //need to make sure the casing matches or else program fails
                    checkedColumn = i;
                    break;
                }
            }

            // cout<< "Checked column is: " <<checkedColumn<<endl;

            while(getline(inputTable, tableFileLine)){
                boost::split(tableFileLineValues, tableFileLine, boost::algorithm::is_any_of("\t"), boost::token_compress_on);

                // cout<< tableFileLineValues[checkedColumn]<<endl;
                // cout<<"IF STATEMENT: "<< indexInfo.stringIndexTable[indexTableName[0]][indexTableName[1]].count(tableFileLineValues[checkedColumn])<<endl;
                if(indexInfo.stringIndexTable[indexTableName[0]][indexTableName[1]].count(tableFileLineValues[checkedColumn]) >=1){
                    btree::btree_multimap<string, vector<string> >::iterator it;
                    for(it = indexInfo.stringIndexTable[indexTableName[0]][indexTableName[1]].equal_range(tableFileLineValues[checkedColumn]).first; it != indexInfo.stringIndexTable[indexTableName[0]][indexTableName[1]].equal_range(tableFileLineValues[checkedColumn]).second; it++){

                        for(int i = 0; i< tableHeaders.size(); i++){
                            hashJoinedRows[tableHeaders[i]].push_back(tableFileLineValues[i]);
                            // cout<< tableFileLineValues[i]<< " ";
                        }//push in the column values for non indexed rows
                        int indexColumnNumber = 0;
                        for(auto tupleValue: it->second){
                            hashJoinedRows[createTableInfo.tableInfo[indexTableName[0] + ".tbl"][indexColumnNumber]].push_back(tupleValue);
                            // cout<<" "<< tupleValue;
                            // cout<<"("<<createTableInfo.tableInfo[indexTableName[0]+ ".tbl"][indexColumnNumber] << ")";
                            indexColumnNumber++;
                        } // push in the tuple values for that index

                        cout<<endl;
                    }
                } //if we can find an index that matches the column we're checking

            }//get the table tuple at that row then see if the column we're checking for the join matches an index.

        }//get the non indexed table for index comparison
        else{

            ifstream inputTable;

            if(indexTableName[0]+ ".tbl" == joinedTables[0]){
                cout<< endl << "Doing a HASH JOIN between the " << joinedTables[1] << " and " <<joinedTables[0] <<" (has index) ON " << whereClauseColumns[0] << " == "<<whereClauseColumns[1] << endl;
                inputTable.open("./data/" + joinedTables[1], ios::in);
            }
            else{
                cout<< endl << "Doing a HASH JOIN between the " << joinedTables[0] << " and " <<joinedTables[1] <<" (has index) ON " << whereClauseColumns[0] << " == "<<whereClauseColumns[1] << endl;
                inputTable.open("./data/" + joinedTables[0], ios::in);

            }

            string tableFileLine;
            vector<string> tableFileLineValues;
            map<string, string> tableRowTuple;
            //key is column name, value is that column's value
            string tempHeaders;
            vector <string> tableHeaders;

            getline(inputTable, tempHeaders);
            //get the table headers;
            boost::split(tableHeaders, tempHeaders, boost::algorithm::is_any_of("\t"), boost::token_compress_on);
            int checkedColumn;

            for(int i = 0; i< tableHeaders.size(); i++){
                if(boost::iequals(tableHeaders[i], whereClauseColumns[0]) || boost::iequals(tableHeaders[i], whereClauseColumns[1])){ //need to make sure the casing matches or else program fails
                    checkedColumn = i;
                    break;
                }
            }

            // cout<< "Checked column is: " <<checkedColumn<<endl;

            while(getline(inputTable, tableFileLine)){
                boost::split(tableFileLineValues, tableFileLine, boost::algorithm::is_any_of("\t"), boost::token_compress_on);

                // cout<< tableFileLineValues[checkedColumn]<<endl;
                // cout<<"IF STATEMENT: "<< indexInfo.stringIndexTable[indexTableName[0]][indexTableName[1]].count(tableFileLineValues[checkedColumn])<<endl;
                if(indexInfo.floatIndexTable[indexTableName[0]][indexTableName[1]].count(stof(tableFileLineValues[checkedColumn])) >=1){
                    btree::btree_multimap<float, vector<string> >::iterator it;
                    for(it = indexInfo.floatIndexTable[indexTableName[0]][indexTableName[1]].equal_range(stof(tableFileLineValues[checkedColumn])).first; it != indexInfo.floatIndexTable[indexTableName[0]][indexTableName[1]].equal_range(stof(tableFileLineValues[checkedColumn])).second; it++){

                        for(int i = 0; i< tableHeaders.size(); i++){
                            hashJoinedRows[tableHeaders[i]].push_back(tableFileLineValues[i]);
                            // cout<< tableFileLineValues[i]<< " ";
                        }//push in the column values for non indexed rows
                        int indexColumnNumber = 0;
                        for(auto tupleValue: it->second){
                            hashJoinedRows[createTableInfo.tableInfo[indexTableName[0] + ".tbl"][indexColumnNumber]].push_back(tupleValue);
                            // cout<<" "<< tupleValue;
                            // cout<<"("<<createTableInfo.tableInfo[indexTableName[0]+ ".tbl"][indexColumnNumber] << ")";
                            indexColumnNumber++;
                        } // push in the tuple values for that index

                        cout<<endl;
                    }
                } //if we can find an index that matches the column we're checking

            }//get the table tuple at that row then see if the column we're checking for the join matches an index.
        }//for float indices
        
        writeResultsToJoinFile("hash", hashJoinedRows);
    }//returns a unordered map of tuples, where the key is the column name and the values are the column values


    void Execute(){
        int joinType;
        if(tableNames.size() == 1){
            lastJoinFile = tableNames[0] + ".tbl";
        } 
        else{
            for(auto tree : subTrees){
                GetWhereColumns(tree);
                CheckForJoinType(); 

                cout<<"index one is: "<< indexOne<<endl; //ex: coupe.Cmodel.string
                cout<<"index two is: "<< indexTwo<<endl; 
                // cout<<"example table name: " << tableNames[0]<<endl; //ex: coupe
                if(!indexTwo.empty()){
                    IndexJoin();
                    lastJoinFile = "index.JOIN";
                    break;
                }
                else if(!indexOne.empty()){
                    HashJoin();
                    lastJoinFile = "hash.JOIN";
                    break;
                }
                else{
                    joinedTables.clear();
                    whereClauseColumns.clear(); 
                }//possible nested loop join, will only know if we check all the where clauses
            }
            while(!tableNames.empty()){
                if(!lastJoinFile.empty()){
                    NestedLoopJoin(lastJoinFile, tableNames[0] + ".tbl");
                } // doing nested loop joins between .JOIN file + .tbl files
                else{
                    NestedLoopJoin(tableNames[0] + ".tbl", tableNames[1] + ".tbl");
                } // only doing nested loop joins between .tbl files

                lastJoinFile = "nestedloop.JOIN";
            }
        }


        cout<<"Final table used for evaluation is: " << lastJoinFile<<endl;

        ifstream projectedTable("./data/" + lastJoinFile, ios::in);

        string fileHeader;
        string tuple;

        getline(projectedTable, fileHeader);

        vector<string> fileHeaderVector;
        vector<string> tupleVector;
        map<string, string> tuples;


        boost::split(fileHeaderVector, fileHeader, boost::algorithm::is_any_of("\t"), boost::token_compress_on);

        for(auto header: fileHeaderVector){
            tuples[header] = "null";
        }


        while(getline(projectedTable, tuple)){
            tupleVector.clear();
            boost::split(tupleVector, tuple, boost::algorithm::is_any_of("\t"), boost::token_compress_on);

            int tupleLocation = 0;
            for(auto header: fileHeaderVector){
                // cout<<header<<"(header)" << tupleVector[tupleLocation] <<endl;
                tuples[header] =tupleVector[tupleLocation]; 
                tupleLocation++;
            }
            // for(auto value: tuples){
            //     cout<<"Header: "<< value.first << "\t Value: "<<value.second<<endl;
            // }


            if(EvaluateInequalityExpression(selectSubTree, tuples)){
                for(auto tupleValue: tuples){
                    cout << tupleValue.second<< "\t";
                }
                cout<<endl;
            }
        }

        Clear();
    }

    string getEvaluationType(expression* treeNode){
        // cout<<"Check func for type: "<< treeNode->func<<endl;
        switch(treeNode->func){
            case OP_LEQ:
            case OP_LT:
            case OP_GEQ:
            case OP_GT:
            case OP_EQUAL:
            case OP_NOTEQ:
            case OP_AND:
            case OP_OR:
                // cout<<"returning inequality"<<endl;
                return "inequality";

            case OP_PLUS:
            case OP_BMINUS:
            case OP_TIMES:
            case OP_DIVIDE:
            case OP_NUMBER:
                // cout<<"returning math"<<endl;
                return "arithmetic";

            case OP_STRING:
            case OP_COLNAME:
                // cout<<"returning string"<<endl;
                return "string";
        }
    }

    string EvaluateStringExpression(expression* treeNode, map<string, string> tableRow){
        if(treeNode->func == OP_STRING){
            // cout<< "Evaluated: "<< treeNode->values[0].data <<endl;
            return treeNode->values[0].data;
        }
        else{
            // cout<<"THIS MIGHT BE SEGFAULTING: ";
            // cout<<tableRow[treeNode->values[0].name];
            // cout<<endl << "OR NOT..."<<endl;

            // cout<< "Evaluated: "<< treeNode->values[0].name<< " and retrieved: "<< tableRow[treeNode->values[0].name] <<endl;
            string key = treeNode->values[0].name;
            key[0] = toupper(key[0]);

            string primaryKey = treeNode->values[0].name;
            transform(primaryKey.begin(), primaryKey.end(), primaryKey.begin(), ::toupper);

            // cout<<"header key variations: " << key << "\t"<< primaryKey<<endl;

            if(tableRow.count(treeNode->values[0].name) != 0){
                // cout<< "Evaluated: "<< treeNode->values[0].name<< " and retrieved: "<< tableRow[treeNode->values[0].name] <<endl;
                return tableRow[treeNode->values[0].name];
            }
            else if(tableRow.count(key) != 0){
                // cout<< "Evaluated: "<< treeNode->values[0].name<< " and retrieved(key): "<< tableRow[key] <<endl;
                return tableRow[key];
            }
            else{
                // cout<< "Evaluated: "<< treeNode->values[0].name<< " and retrieved(Pkey): "<< tableRow[primaryKey] <<endl;
                return tableRow[primaryKey];
            }
        }
    }

    float EvaluateArithmeticExpression(expression* treeNode, map<string, string> tableRow){
        if(!(treeNode->func == OP_NUMBER)){
            float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
            float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);
            switch(treeNode->func){
                case OP_PLUS:
                    return leftValue + rightValue;    
                case OP_BMINUS:
                    return leftValue - leftValue;
                case OP_TIMES:
                    return leftValue * rightValue;
                case OP_DIVIDE:
                    return leftValue / rightValue;
            }
        }   
        else{
            return treeNode->values[0].num;
        }
    }//this is called when current operator is an arithmetic op / is number

    bool EvaluateInequalityExpression(expression* treeNode, map<string, string> tableRow){
        switch(treeNode->func){
            case OP_AND:{
                bool leftValue = EvaluateInequalityExpression(treeNode->values[0].ep, tableRow);
                bool rightValue = EvaluateInequalityExpression(treeNode->values[1].ep, tableRow);
                if(leftValue && rightValue){
                    return true;
                }
                else{
                    return false;
                }
            }
            case OP_OR:{
                bool leftValue = EvaluateInequalityExpression(treeNode->values[0].ep, tableRow);
                bool rightValue = EvaluateInequalityExpression(treeNode->values[1].ep, tableRow);

                return leftValue || rightValue;
            }
            case OP_LEQ:{
                if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "string"){

                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                            float rightValue = stof(stringRightValue);

                            return leftValue <= rightValue;
                        } //this is most likely going to be the selection type, where both are floats
                        else{
                            cout<<"Invalid selection, cannot compare a string to a float" << endl;
                            //ERROR MESSAGE, CANNOT COMPARE FLOAT TO STRING LITERAL
                        }
                    }
                    else if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    } // the left value is a string and the right value is a string literal
                    else{
                        cout<<"Invalid selection, cannot do string <= string" << endl;
                    }


                }

                else if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "arithmetic"){
                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        return leftValue <= rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }

                else if(getEvaluationType(treeNode->values[0].ep) == "arithmetic" && getEvaluationType(treeNode->values[1].ep) == "string"){
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringRightValue[0])) || stringRightValue[0] == '-'){
                        float rightValue = stof(stringRightValue);
                        return leftValue <= rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }
                else{
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    return leftValue <= rightValue;
                }

                break;
            }
            case OP_LT:{
                if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "string"){

                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                            float rightValue = stof(stringRightValue);

                            return leftValue < rightValue;
                        } //this is most likely going to be the selection type, where both are floats
                        else{
                            cout<<"Invalid selection, cannot compare a string to a float" << endl;
                            //ERROR MESSAGE, CANNOT COMPARE FLOAT TO STRING LITERAL
                        }
                    }
                    else if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    } // the left value is a string and the right value is a string literal
                    else{
                        cout<<"Invalid selection, cannot do string < string" << endl;
                    }


                }

                else if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "arithmetic"){
                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        return leftValue < rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }

                else if(getEvaluationType(treeNode->values[0].ep) == "arithmetic" && getEvaluationType(treeNode->values[1].ep) == "string"){
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringRightValue[0])) || stringRightValue[0] == '-'){
                        float rightValue = stof(stringRightValue);
                        return leftValue < rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }
                else{
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    return leftValue < rightValue;
                }
                break;
            }
            case OP_GEQ:{
                if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "string"){

                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                            float rightValue = stof(stringRightValue);

                            return leftValue >=  rightValue;
                        } //this is most likely going to be the selection type, where both are floats
                        else{
                            cout<<"Invalid selection, cannot compare a string to a float" << endl;
                            //ERROR MESSAGE, CANNOT COMPARE FLOAT TO STRING LITERAL
                        }
                    }
                    else if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    } // the left value is a string and the right value is a string literal
                    else{
                        cout<<"Invalid selection, cannot do string >=  string" << endl;
                    }


                }

                else if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "arithmetic"){
                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        return leftValue >=  rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }

                else if(getEvaluationType(treeNode->values[0].ep) == "arithmetic" && getEvaluationType(treeNode->values[1].ep) == "string"){
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringRightValue[0])) || stringRightValue[0] == '-'){
                        float rightValue = stof(stringRightValue);
                        return leftValue >=  rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }
                else{
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    return leftValue >=  rightValue;
                }
                break;
            }
            case OP_GT:{
                if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "string"){

                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                            float rightValue = stof(stringRightValue);

                            return leftValue >  rightValue;
                        } //this is most likely going to be the selection type, where both are floats
                        else{
                            cout<<"Invalid selection, cannot compare a string to a float" << endl;
                            //ERROR MESSAGE, CANNOT COMPARE FLOAT TO STRING LITERAL
                        }
                    }
                    else if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    } // the left value is a string and the right value is a string literal
                    else{
                        cout<<"Invalid selection, cannot do string >  string" << endl;
                    }
                }

                else if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "arithmetic"){
                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        return leftValue >  rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }

                else if(getEvaluationType(treeNode->values[0].ep) == "arithmetic" && getEvaluationType(treeNode->values[1].ep) == "string"){
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringRightValue[0])) || stringRightValue[0] == '-'){
                        float rightValue = stof(stringRightValue);
                        return leftValue >  rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }
                else{
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    return leftValue >  rightValue;
                }
                break;
            }
            case OP_EQUAL:{
                if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "string"){

                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                            float rightValue = stof(stringRightValue);

                            return leftValue ==  rightValue;
                        } //this is most likely going to be the selection type, where both are floats
                        else{
                            cout<<"Invalid selection, cannot compare a string to a float" << endl;
                            //ERROR MESSAGE, CANNOT COMPARE FLOAT TO STRING LITERAL
                        }
                    }
                    else if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    } // the left value is a string and the right value is a string literal
                    else{
                        // cout<<"comparing two strings"<<endl;
                        // cout<<"left value: "<<stringLeftValue <<"\t" << "right value: "<<stringRightValue<<endl;
                        return stringLeftValue == stringRightValue;
                    }
                }

                else if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "arithmetic"){
                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        return leftValue ==  rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }

                else if(getEvaluationType(treeNode->values[0].ep) == "arithmetic" && getEvaluationType(treeNode->values[1].ep) == "string"){
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringRightValue[0])) || stringRightValue[0] == '-'){
                        float rightValue = stof(stringRightValue);
                        return leftValue ==  rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }
                else{
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    return leftValue ==  rightValue;
                }

                cout<< "Something went terribly wrong"<<endl;
                break;
            }
            case OP_NOTEQ:{
                if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "string"){

                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                            float rightValue = stof(stringRightValue);

                            return leftValue !=  rightValue;
                        } //this is most likely going to be the selection type, where both are floats
                        else{
                            cout<<"Invalid selection, cannot compare a string to a float" << endl;
                            //ERROR MESSAGE, CANNOT COMPARE FLOAT TO STRING LITERAL
                        }
                    }
                    else if((isdigit(stringLeftValue[1])) || stringLeftValue[1] == '-'){
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    } // the left value is a string and the right value is a string literal
                    else{
                        return stringLeftValue != stringRightValue;
                    }


                }

                else if(getEvaluationType(treeNode->values[0].ep) == "string" && getEvaluationType(treeNode->values[1].ep) == "arithmetic"){
                    string stringLeftValue = EvaluateStringExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringLeftValue[0])) || stringLeftValue[0] == '-'){
                        float leftValue = stof(stringLeftValue);
                        return leftValue !=  rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }

                else if(getEvaluationType(treeNode->values[0].ep) == "arithmetic" && getEvaluationType(treeNode->values[1].ep) == "string"){
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    string stringRightValue = EvaluateStringExpression(treeNode->values[1].ep, tableRow);

                    if((isdigit(stringRightValue[0])) || stringRightValue[0] == '-'){
                        float rightValue = stof(stringRightValue);
                        return leftValue !=  rightValue;
                    }
                    else{
                        cout<<"Invalid selection, cannot compare a string to a float"<<endl;
                    }
                }
                else{
                    float leftValue = EvaluateArithmeticExpression(treeNode->values[0].ep, tableRow);
                    float rightValue = EvaluateArithmeticExpression(treeNode->values[1].ep, tableRow);

                    return leftValue !=  rightValue;
                }
            }
                
        } 
    }//this is called when current operator is an inequality (always has a children)


    void PrintSubTrees(){
        cout<<"Printing SubTrees"<<endl;
        for(auto tree: subTrees){
            while(tree){
                if(tree->func == OP_NUMBER || tree->func == OP_STRING|| tree->func == OP_COLNAME){
                    cout<<"This Node's func is: ";
                    cout<<tree->func<<endl;
                    break;
                }
                cout<<"This Node's func is: ";
                cout<<tree->func<<endl;
                tree = tree->values[0].ep;
            }
            cout<<endl;
        }
    }

    void SplitSubTrees(expr e){
        // cout<<"Splitting subtrees"<<endl;
        expression *ep = (expression *)e;
        expression *subTreeRootNode = (expression *)e;
        expression *newSubTreeRootNode;
        while(ep){
            // cout<<"This subtree node's func is: "<< ep->func<<endl;
            switch(ep->func){ 
                case OP_NUMBER:
                case OP_STRING:
                case OP_COLNAME:
                    subTrees.push_back(subTreeRootNode);
                    goto doneParsingSubTrees;
                    break;

                case OP_AND:
                case OP_OR:
                    newSubTreeRootNode = ep->values[0].ep;
                    // cout<<"The new subtree root node's func is: "<< newSubTreeRootNode->func<<endl;
                    subTrees.push_back(ep->values[1].ep);
                    subTreeRootNode = newSubTreeRootNode;
            }

            // cout<<"Moving to the left node"<<endl<<endl;
            ep = ep->values[0].ep;
        }

        doneParsingSubTrees: 
        cout<< endl;
        //needed to break out of while loop to stop parsing the tree as there aren't any more nodes
        // cout<<"done parsing"<<endl<<endl;
        // PrintSubTrees();

    }//is passed in to the root of the selection statement

};