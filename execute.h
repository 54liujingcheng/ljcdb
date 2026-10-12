#include "query.h"
#include "parse.h"
#include "system_management.h"
#pragma once
void runSQLJC(DbManager& dbm,string SQLJC, bool mode);
vector<UnboundRequest> parseSQLJC(std::string SQLJC);
void printDML(pair<int, int> output, long long duration, bool mode, const string& msg);
int HandleAlter(DbManager& dbm, AlterRequest& req, string& msg);
void printDESC(DbManager& dbm, Table* tb, bool mode);
