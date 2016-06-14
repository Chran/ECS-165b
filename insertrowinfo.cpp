#pragma once

#include <string>
#include <iostream>
#include <vector>

#include "expression.h"
#include "createtableinfo.cpp"
#include "indexinfo.cpp"

using namespace std;
// g++ -std=c++11 -lboost_system -lboost_filesystem insertinfo.cpp

class InsertRowInfo{
public:
  string tableName;
  vector <string> columnValues;

  void SetTableName(string inputTableName){
    tableName = inputTableName + ".tbl";
  }

  //reverse the RList
  void CorrectColumns(){
    reverse(columnValues.begin(), columnValues.end());
  }

  void Clear(){
    columnValues.clear();
  } 

  void Parse(expr e){
    expression *ep = (expression *)e;

    SetTableName(ep->values[0].ep->values[0].data);

    while(ep){
      if(ep->func == OP_RLIST){
        // rlistStack.push(ep->values[0].ep->values[0].ep->values[0].data);
        switch(ep->values[0].ep->func){
          case OP_NUMBER:
            columnValues.push_back(to_string(ep->values[0].ep->values[0].num));
            break;
          case OP_STRING:
            columnValues.push_back(ep->values[0].ep->values[0].data);
            break;
          case OP_NULL:
            columnValues.push_back("NULL");
            break; 
          default:
            cout<<"Something went wrong with reading in values for INSERT command"<<endl;
            break;         
        }
      }
      ep = ep->values[1].ep;
    }//traverses to the right of the root in order to store RLIST data

    CorrectColumns();
  }

  void Execute(map<string, vector<string> > tableInfo){
    if(columnValues.size() < tableInfo[tableName].size()){
      cout<< "Too few values being inserted"<<endl;
      return;
    }
    else if(columnValues.size() > tableInfo[tableName].size()){
      cout<< "Too many values being inserted"<<endl;
      return;
    }

    if(tableInfo.count(tableName) == 0){
      cout<<"Table not found"<<endl;
      return;
    }
    
    ofstream tableFileOut("./data/" + tableName, ios::app);

    for(int i = 0; i< columnValues.size() - 1; i++){
      tableFileOut << columnValues[i] << "\t";
    }
    tableFileOut << columnValues[columnValues.size() - 1] <<endl;
    tableFileOut.close();

    cout<<"INSERT VALUES command executed"<<endl;

    //count number of lines -1 with while get line until eof is reached. That is the index
  }

};