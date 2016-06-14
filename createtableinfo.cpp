#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <string>
#include <map>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

#include "expression.h"
#include "HW4-expr.h"

using namespace std;

class CreateTableInfo{
public:
  map<string, vector<string> > tableInfo;
  string tableName; //temp placeholder
  vector<string> columnNames;
  stack<string> rlistStack;

  void PrintTableInfo(){
    for(auto& i: tableInfo){
      cout<< "This Table's name is: " << i.first << " and its headers are: ";
      for(auto& j: i.second){
        cout<< j << " , " ;
      }
      cout<<endl;
    }
    //also print out the number of tables 
  }

  vector<string> ParseTableHeader(string table){
    ifstream tableFile("./data/" + table, ios::in);
    string firstTableLine;
    vector<string> tableHeader;

    getline(tableFile,firstTableLine);

    boost::split(tableHeader, firstTableLine, boost::is_any_of("\t"));

    tableFile.close();

    return tableHeader;
  }


  void LoadTables(){
    ifstream tableIn;

    tableInfo.clear();

    boost::filesystem::path directory("./data");
    if(!boost::filesystem::exists(directory)){
      if(!boost::filesystem::create_directories( directory )){
        cout<<"Cannot create database directory"<<endl;
        return;
      }
    }

    boost::filesystem::directory_iterator iter(directory), end;

    for(;iter != end; iter++){
      if(iter->path().extension() == ".tbl"){
        string tableFileName = iter->path().filename().string();
        vector<string> headers = ParseTableHeader(tableFileName);
        tableInfo[tableFileName] = headers;
      }//if the extension is .tbl, read it in
    }
    tableIn.close();
  }




  void Parse(expr e){
    expression *ep = (expression *)e;
    string tempString(ep->values[0].ep->values[0].name);
    this->tableName = tempString + ".tbl"; //settablename
    // temp placeholder that'll be used to in the map is set to the table name attached to the root

    while(ep){
      if(ep->func == OP_RLIST){
        if(ep->values[0].ep->values[1].num == 1 ){
          ep->values[0].ep->values[0].ep->values[0].name[0] = toupper(ep->values[0].ep->values[0].ep->values[0].name[0]);
        }//if we see a key, convert the first letter to uppercase
        else if(ep->values[0].ep->values[1].num == 3){
          boost::algorithm::to_upper(ep->values[0].ep->values[0].ep->values[0].name);
        }//if we see a primary key, convert the whole thing to uppercase
        
        this->rlistStack.push(ep->values[0].ep->values[0].ep->values[0].name);
      }
      ep = ep->values[1].ep;
    }//traverses to the right of the root in order to store RLIST data

    this->SetColumnNames();
  }//sets the columnName vec and tableName for mapping in Execute()

  void SetColumnNames(){
    while(!this->rlistStack.empty()){
      this->columnNames.push_back(this->rlistStack.top());
      this->rlistStack.pop();
    }
  }//called in parse; empties rliststack and stores the data into this obj's columnnames in the correct order
  //didn't want to deal w/ a reverse vec iter

  void Execute(){
    boost::filesystem::path path("./data");
    if(!boost::filesystem::exists(path)){
      if(!boost::filesystem::create_directories( path )){
        cout<<"Cannot create database directory"<<endl;
        return;
      }
    }
    if(boost::filesystem::exists("./data/" + this->tableName)){
      cout<<"Table already exists"<<endl;
      return;
    }
    else{
      ofstream tableFile;
      tableFile.open("./data/" + this->tableName);
      
      if(!tableFile.is_open()){
        cout<<"Cannot create table"<<endl;
        return;
      }

      for(int i = 0; i< this->columnNames.size()-1; i++){
        tableFile<< this->columnNames[i] << "\t";
      }//delimit the tbl files by tabs (for the headers in this case)
      tableFile << this->columnNames[this->columnNames.size() - 1]<<endl;

      tableFile.close();

      cout<<"CREATE TABLE command executed"<<endl;
    }

    this->tableInfo[this->tableName] = columnNames;
    //clear should be called after indexfile is created.
  }

  void Clear(){
    this->tableName.clear();
    this->columnNames.clear();
  }


};
