#pragma once

#include <map>
#include <string>
#include <iostream>
#include <cstring>
#include <cctype>

#include <boost/filesystem.hpp>
#include <boost/any.hpp>
#include <boost/algorithm/string/split.hpp>

#include "createtableinfo.cpp"
#include "glibs/btree_map.h"

#include "insertrowinfo.cpp"

using namespace std;
// g++ -std=c++11 -lboost_system -lboost_filesystem indexinfo.cpp


class IndexInfo{
public:
  map<string, map<string, btree::btree_multimap<string, vector<string> > > > stringIndexTable;
  // outer most map key = table name with the values = list of possible idx files
  // middle map key = column name (idx file name) with the values = btree indices
  // inner most map key = column value with the values = index tuple within a specific table (basically the key of the outer most map)


  map<string, map<string, btree::btree_multimap<float,  vector<string> > > > floatIndexTable;
  //in this case the column value are floats(for the btree keys)

  static void CreateIndexFile(CreateTableInfo newTableToBeCreated){
    ofstream indexFileToBeCreated;
    vector<string> tempTableName;
    boost::split(tempTableName, newTableToBeCreated.tableName, boost::algorithm::is_any_of("."));
    //parses out the .tbl suffix in newTableToBeCreated's tableName

    for(auto colName : newTableToBeCreated.columnNames){
      if(isupper(colName[0])){
        indexFileToBeCreated.open("./data/" + tempTableName[0] + "." + colName + ".idx");
        indexFileToBeCreated.close();
      }//if the tobeconstructedtable has columns that are keys/primary keys, create an index file for each one of them
    }
  }//is only called when CREATE TABLE gets executed

/*
  template <typename T>
  void StoreIndexToBTree(btree::btree_multimap<T, vector<string> > newIndexBTree, vector<string> indexFileName, bool columnStringType){
    ifstream indexFile("./data/"+indexFileName[0] + "." + indexFileName[1] + ".idx", ios::in);
    string indexFileLine;
    vector<string> splitIndexFileLine;
    vector<string> indexFileTuple;

    while(getline(indexFile,indexFileLine)){
      indexFileTuple.clear();
      boost::split(splitIndexFileLine,indexFileLine, boost::algorithm::is_any_of("\t"));

      for(int i = 1; i< splitIndexFileLine.size(); i++){
        indexFileTuple.push_back(splitIndexFileLine[i]);
      }//start at 1 because the 0 is the column value; only want the tuple that corresponds to that column value

      if(columnStringType){
        newIndexBTree.insert(pair<string, vector<string> >(splitIndexFileLine[0], indexFileTuple));
      }
      else{
        newIndexBTree.insert(pair<float, vector<string> >(stof(splitIndexFileLine[0]), indexFileTuple));
      }

    } 

    if(columnStringType)
      this->stringIndexTable[indexFileName[0]][indexFileName[1]] = newIndexBTree;
    else
      this->floatIndexTable[indexFileName[0]][indexFileName[1]] = newIndexBTree;

    indexFile.close();
  } //template doesn't work, compiler doesn't know what exact type the first parameter of the btree_multimap
*/

  void StoreIndexToBTree(btree::btree_multimap<float, vector<string> > newFloatIndexTable, vector<string> indexFileName){
    ifstream indexFile("./data/"+indexFileName[0] + "." + indexFileName[1] + ".idx", ios::in);
    string indexFileLine;
    vector<string> splitIndexFileLine;
    vector<string> indexFileTuple;

    while(getline(indexFile,indexFileLine)){
      indexFileTuple.clear();
      boost::split(splitIndexFileLine,indexFileLine, boost::algorithm::is_any_of("\t"));

      for(int i = 1; i< splitIndexFileLine.size(); i++){
        indexFileTuple.push_back(splitIndexFileLine[i]);
      }//start at 1 because the 0 is the column value; only want the tuple that corresponds to that column value

      newFloatIndexTable.insert(pair<float, vector<string> >(stof(splitIndexFileLine[0]), indexFileTuple));
    } 

    this->floatIndexTable[indexFileName[0]][indexFileName[1]] = newFloatIndexTable;
    indexFile.close();
  }//for floats; stores idx file to main mem btree

  void StoreIndexToBTree(btree::btree_multimap<string, vector<string> > newStringIndexTable, vector<string> indexFileName){
    ifstream indexFile("./data/"+indexFileName[0] + "." + indexFileName[1] + ".idx", ios::in);
    string indexFileLine;
    vector<string> splitIndexFileLine;
    vector<string> indexFileTuple;

    while(getline(indexFile,indexFileLine)){
      indexFileTuple.clear();
      boost::split(splitIndexFileLine,indexFileLine, boost::algorithm::is_any_of("\t"));

      for(int i = 1; i< splitIndexFileLine.size(); i++){
        indexFileTuple.push_back(splitIndexFileLine[i]);
      }//start at 1 because the 0 is the column value; only want the tuple that corresponds to that column value

      newStringIndexTable.insert(pair<string, vector<string> >(splitIndexFileLine[0], indexFileTuple));
    } 
    this->stringIndexTable[indexFileName[0]][indexFileName[1]] = newStringIndexTable;
    indexFile.close();
  }//for strings


  void AddIndex(InsertRowInfo insertRowInfo, CreateTableInfo createTableInfo){

    vector<string> tableHeaders = createTableInfo.ParseTableHeader(insertRowInfo.tableName);
    vector<string> parsedTableName;

    boost::split(parsedTableName, insertRowInfo.tableName, boost::algorithm::is_any_of("."));
    
    int i = 0;
    for(auto column: tableHeaders){

      if(isupper(column[0])){
        string writeFile = "./data/"+parsedTableName[0] + "." + column + ".idx";
        ofstream indexFile(writeFile, ios::app);

        indexFile<< insertRowInfo.columnValues[i];

        for(int j = 0; j<insertRowInfo.columnValues.size(); j++){
          indexFile << "\t" << insertRowInfo.columnValues[j];
        }// insert tuple that corresponds to that column index value

        indexFile<<endl;
        indexFile.close();
      }

      i++;
    }

    LoadIndexFiles(); //shouldn't reload ALL the files, only the one we've accessed (need to optimize)
  }//adds index to idx file

  void LoadIndexFiles(){
    this->Clear();
    boost::filesystem::path directory("./data");
    boost::filesystem::directory_iterator iter(directory), end;

    for(;iter != end; iter++){
      if(iter->path().extension() == ".idx"){
        string indexFileName = iter->path().filename().string(); //tblname.colName.idx
        vector<string> splitFileName;
        boost::split(splitFileName,indexFileName,boost::algorithm::is_any_of(".")); //parse by '.'

        ifstream indexFile("./data/"+indexFileName, ios::in);
        string indexFileLine;

        if(getline(indexFile,indexFileLine)){ //if we're looking at a float index value
          if(isdigit(indexFileLine.c_str()[0]) || indexFileLine.c_str()[0] == '-'){
            btree::btree_multimap<float, vector<string> > newFloatIndexTable;
            indexFile.close();
            StoreIndexToBTree(newFloatIndexTable, splitFileName);
          }//if the key column value is a positive or negative number, store it into the int 
          else{//else we're looking at a string index value
            btree::btree_multimap<string, vector<string> > newStringIndexTable;
            indexFile.close();
            StoreIndexToBTree(newStringIndexTable, splitFileName);
          }
        }
      }
    }
  } //loads idx files for storing the index tuples into main mem btree

  void Clear(){
    stringIndexTable.clear();
    floatIndexTable.clear();
  }

  void PrintStringIndex(){
    for(auto& i:stringIndexTable){
      cout<<endl;
      cout<<"This String Index Table's name is : "<< i.first<<endl;
      cout<<endl;

      for(auto& j:i.second){
        cout<<"This is the key (column name - string): "<< j.first<<endl<<endl;
        for(auto& k:j.second){
          cout<<"This is the column's VALUE: " << k.first <<endl <<"This is the Index Tuple: "; 
          for(auto& l: k.second){
            cout<< l << ",";
          }
          cout<<endl;
        }
        cout<<endl;
      }
      cout<<endl;
    }
  }

  void PrintFloatIndex(){
    for(auto& i:floatIndexTable){
      cout<<endl;
      cout<<"This Float Index Table's name is : "<< i.first<<endl;
      cout<<endl;

      for(auto& j:i.second){
        cout<<"This is the key (column name - float): "<< j.first<<endl<<endl;
        for(auto& k:j.second){
          cout<<"This is the column's VALUE: " << k.first <<endl <<"This is the Index Tuple: "; 
          for(auto& l: k.second){
            cout<< l << ",";
          }
          cout<<endl;
        }
        cout<<endl;
      }
      cout<<endl;
    }
  }  

};