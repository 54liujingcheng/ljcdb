#include "execute.h"
#include "query.h"
#include "third_party/antlr4-cpp-runtime-4.13.2-source/runtime/src/antlr4-runtime.h"
#include "parser/generated/SQLLexer.h"
#include "parser/generated/SQLParser.h"
vector<UnboundRequest> parseSQLJC(std::string SQLJC) {
    antlr4::ANTLRInputStream sInputStream(SQLJC);
    SQLLexer iLexer(&sInputStream);
    antlr4::CommonTokenStream sTokenStream(&iLexer);
    SQLParser iParser(&sTokenStream);
    auto* iTree = iParser.program();
    SQLJCVisitor iVisitor;
    auto iRes = iVisitor.visit(iTree);
    return any_cast<vector<UnboundRequest>>(move(iRes));
}
void runSQLJC(DbManager& dbm,string SQLJC, bool mode){
    vector<UnboundRequest> res = parseSQLJC(SQLJC);
    for(auto& i:res){
        if(auto* simple = get_if<SimpleRequest>(&i)){

        }else if(auto* alter = get_if<AlterRequest>(&i)){

        }else if(auto* ctr = get_if<CreateTableRequest>(&i)){

        }else if(auto* load = get_if<LoadRequest>(&i)){

        }else if(auto* uir = get_if<UnboundInsertRequest>(&i)){
            auto tmp = BindingInsertRequest(dbm, (*uir));
            auto res = InsertExecuter(dbm, tmp);
        }else if(auto* udr = get_if<UnboundDeleteRequest>(&i)){
            auto tmp = BindingDeleteRequest(dbm, (*udr));
            auto res = DeleteExecuter(dbm, tmp);
        }else if(auto* uur = get_if<UnboundUpdateRequest>(&i)){
            
        }else if(auto* usr = get_if<UnboundSelectRequest>(&i)){

        }
    }
}