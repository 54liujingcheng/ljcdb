#include "parser/generated/SQLBaseVisitor.h"
#include "query.h"
#include <limits>
#include <stdexcept>
#pragma once
enum class Simple {
    CREATE_DATABASE,
    DROP_DATABASE,
    USE_DATABASE,
    DROP_TABLE,
    DESCRIBE_TABLE,
    SHOW_DATABASES,
    SHOW_TABLES,
    SHOW_INDEXES,
    EMPTY
};
struct SimpleRequest {
    Simple command;
    optional<string> name;
};
enum class Alter {
    ADD_INDEX,
    DROP_INDEX,
    ADD_PRIMARY_KEY,
    DROP_PRIMARY_KEY,
    ADD_FOREIGN_KEY,
    DROP_FOREIGN_KEY,
    ADD_UNIQUE
};
struct AlterRequest {
    Alter kind;
    string table_name;
    optional<string> constraint_name;
    vector<string> columns;
    optional<string> referenced_table;
    vector<string> referenced_columns;
};
struct LoadRequest{
    string file_name;
    string table_name;
};
struct CreateTableRequest{
    string table_name;
    Schema schema;
    vector<AlterRequest> index_request;
};
typedef variant<UnboundDeleteRequest, UnboundInsertRequest, UnboundUpdateRequest, UnboundSelectRequest,
SimpleRequest, AlterRequest, LoadRequest, CreateTableRequest> UnboundRequest;
class SQLJCVisitor : public SQLBaseVisitor {
    static long long parseInteger(std::string_view text, const char* what) {
        long long value = 0;
        auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if(ec != std::errc{} || end != text.data() + text.size())
            throw std::runtime_error(std::string("invalid ") + what);
        return value;
    }
    static double parseDouble(std::string_view text) {
        double value = 0;
        auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if(ec != std::errc{} || end != text.data() + text.size())
            throw std::runtime_error("invalid floating-point literal");
        return value;
    }
    static bool containsErrorNode(antlr4::tree::ParseTree* node) {
        if(dynamic_cast<antlr4::tree::ErrorNode*>(node) != nullptr)
            return true;
        if(auto* context = dynamic_cast<antlr4::ParserRuleContext*>(node);
           context != nullptr && context->exception)
            return true;
        for(auto* child : node->children){
            if(containsErrorNode(child))return true;
        }
        return false;
    }
public:
    AggregateFunc parseAggregator(SQLParser::AggregatorContext *ctx) {
        if(ctx->Count())return AggregateFunc::COUNT;
        else if(ctx->Average())return AggregateFunc::AVG;
        else if(ctx->Max())return AggregateFunc::MAX;
        else if(ctx->Min())return AggregateFunc::MIN;
        else if(ctx->Sum())return AggregateFunc::SUM;
        throw std::runtime_error("invalid aggregate function");
    }
    CompareOp parseOperator_(SQLParser::Operator_Context *ctx) {
        if(ctx->EqualOrAssign())return CompareOp::EQ;
        else if(ctx->Greater())return CompareOp::GT;
        else if(ctx->GreaterEqual())return CompareOp::GE;
        else if(ctx->Less())return CompareOp::LT;
        else if(ctx->LessEqual())return CompareOp::LE;
        else if(ctx->NotEqual())return CompareOp::NE;
        throw std::runtime_error("invalid comparison operator");
    }
    pair<string, string> parseColumn(SQLParser::ColumnContext *ctx){
        if(ctx->Identifier().size()==1)return {"",ctx->Identifier(0)->getText()};
        if(ctx->Identifier().size()==2)return {ctx->Identifier(0)->getText(), ctx->Identifier(1)->getText()};
        throw std::runtime_error("invalid column reference");
    }
    vector<pair<string, string>> parseColumn_list(SQLParser::Column_listContext *ctx){
        vector<pair<string, string>> vec;
        for(auto& i:ctx->column()){
            vec.push_back(parseColumn(i));
        }
        return vec;
    }
    vector<string> parseIdentifiers(SQLParser::IdentifiersContext *ctx){
        vector<string> vec;
        for(auto& i:ctx->Identifier()){
            vec.push_back(i->getText());
        }
        return vec;
    }
    Value parseValue(SQLParser::ValueContext *ctx){
        if(ctx->Float()){
            string s = ctx->Float()->getText();
            return parseDouble(s);
        }
        else if(ctx->String()){
            string s = ctx->String()->getText();
            return s.substr(1,s.size()-2);
        }
        else if(ctx->Null()){
            return (bool)(false);
        }
        else if(ctx->signed_integer()){
            string s = ctx->signed_integer()->getText();
            long long value = parseInteger(s, "integer literal");
            if(value < std::numeric_limits<int>::min() ||
               value > std::numeric_limits<int>::max())
                throw std::runtime_error("integer literal out of range");
            return static_cast<int>(value);
        }
        throw std::runtime_error("invalid value");
    }
    OrderBy parseOrder(SQLParser::OrderContext *ctx){
        if (ctx->getText() == "ASC")
            return OrderBy::ASC;
        else if (ctx->getText() == "DESC")
            return OrderBy::DESC;
        throw std::runtime_error("invalid order direction");
    }
    pair<int, int> parseType_(SQLParser::Type_Context *ctx){
        if(ctx->getText() == "INT")return {0,sizeof(int)};
        if(ctx->getText() == "FLOAT" || ctx->getText() == "DOUBLE")return {1,sizeof(double)};
        if(ctx->getText().starts_with("VARCHAR") || ctx->getText().starts_with("CHAR")){
            string s = ctx->UnsignedInteger()->getText();
            long long value = parseInteger(s, "string length");
            if(value <= 0 || value > 2048)
                throw std::runtime_error("string length out of range");
            return {2,static_cast<int>(value)};
        }
        if(ctx->getText() == "DATE"){
            return {3,sizeof(DATE)};
        }
        throw std::runtime_error("invalid column type");
    }
    vector<Value> parseValue_list(SQLParser::Value_listContext *ctx){
        vector<Value> vec;
        for(auto i : ctx->value()){
            vec.push_back(parseValue(i));
        }
        return vec;
    }
    vector<Row> parseValue_lists(SQLParser::Value_listsContext *ctx){
        vector<Row> vec;
        for(auto i:ctx->value_list()){
            vec.push_back(parseValue_list(i));
        }
        return vec;
    }
    UnboundOprand parseExpression(SQLParser::ExpressionContext *ctx){
        UnboundOprand res;
        if(ctx->value()){
            res.is_val = 1;
            res.val = parseValue(ctx->value());
            return res;
        }
        if(ctx->column()){
            res.is_val = 0;
            res.col_name = parseColumn(ctx->column());
            return res;
        }
        throw std::runtime_error("invalid expression");
    }
    pair<pair<string, string>, AggregateFunc>parseSelector(SQLParser::SelectorContext *ctx){
        pair<string, string> col;
        AggregateFunc agg;
        if(ctx->aggregator()){
            agg = parseAggregator(ctx->aggregator());
            col = parseColumn(ctx->column());
            return {col, agg};
        }
        if(ctx->column()){
            col = parseColumn(ctx->column());
            agg = AggregateFunc::NONE;
            return {col, agg};
        }
        if(ctx->Count()){
            return {{"",""},AggregateFunc::COUNT};
        }
        throw std::runtime_error("invalid selector");
    }
    vector<pair<pair<string, string>, AggregateFunc>> parseSelectors(SQLParser::SelectorsContext *ctx){
        vector<pair<pair<string, string>, AggregateFunc>> vec;
        if(ctx->selector().empty())return {{{"", "*"}, AggregateFunc::NONE}};
        for(auto i : ctx->selector()){
            vec.push_back(parseSelector(i));
        }
        return vec;
    }
    pair<pair<string,string>, OrderBy> parseOrder_item(SQLParser::Order_itemContext *ctx){
        if(ctx->order())return {parseColumn(ctx->column()), parseOrder(ctx->order())};
        else return {parseColumn(ctx->column()), OrderBy::ASC};
    }
    pair<pair<string, string>, Value> parseAssignment(SQLParser::AssignmentContext *ctx) {
        return {parseColumn(ctx->column()), parseValue(ctx->value())};
    }
    vector<pair<pair<string, string>, Value>> parseSet_clause(SQLParser::Set_clauseContext *ctx) {
        vector<pair<pair<string, string>, Value>> vec;
        for(auto i :ctx->assignment()){
            vec.push_back(parseAssignment(i));
        }
        return vec;
    }
    vector<pair<string,string>> parseGroup_by_part(SQLParser::Group_by_partContext *ctx){
        return parseColumn_list(ctx->column_list());
    }
    vector<pair<pair<string,string>, OrderBy>> parseOrder_by_part(SQLParser::Order_by_partContext *ctx){
        vector<pair<pair<string,string>, OrderBy>> vec;
        for(auto i:ctx->order_item()){
            vec.push_back(parseOrder_item(i));
        }
        return vec;
    }
    pair<int, int> parseLimit_part(SQLParser::Limit_partContext *ctx){
        auto parseLimit = [](long long value) {
            if(value < 0 || value > std::numeric_limits<int>::max())
                throw std::runtime_error("LIMIT/OFFSET out of range");
            return static_cast<int>(value);
        };
        if(ctx->UnsignedInteger().size()==1){
            int value = parseLimit(parseInteger(ctx->UnsignedInteger(0)->getText(), "LIMIT"));
            return {value,0};
        }
        if(ctx->UnsignedInteger().size()==2){
            int value = parseLimit(parseInteger(ctx->UnsignedInteger(0)->getText(), "LIMIT"));
            int offset = parseLimit(parseInteger(ctx->UnsignedInteger(1)->getText(), "OFFSET"));
            if(static_cast<long long>(value) + offset > std::numeric_limits<int>::max())
                throw std::runtime_error("LIMIT plus OFFSET is out of range");
            return {value,offset};
        }
        throw std::runtime_error("invalid LIMIT clause");
    }
    virtual std::any visitWhere_operator_expression(SQLParser::Where_operator_expressionContext *ctx) override {
        UnboundCondition ubc;
        ubc.type = ConditionType::COMPARE;
        ubc.op = parseOperator_(ctx->operator_());
        if(ctx->column())ubc.left = UnboundOprand{0,parseColumn(ctx->column())};
        ubc.right = parseExpression(ctx->expression());
        return ubc;
    }
    virtual std::any visitWhere_value_operator_column(SQLParser::Where_value_operator_columnContext *ctx) override {
        UnboundCondition ubc;
        ubc.type = ConditionType::COMPARE;
        ubc.op = parseOperator_(ctx->operator_());
        if(ctx->column())ubc.right = UnboundOprand{0,parseColumn(ctx->column())};
        ubc.left = UnboundOprand{1,{"", ""},parseValue(ctx->value())};
        return ubc;
    }
    std::any visitWhere_null(SQLParser::Where_nullContext* ctx) {
        UnboundCondition condition;
        condition.left.is_val = false;
        condition.left.col_name = parseColumn(ctx->column());
        bool hasNot = false;
        for (auto* child : ctx->children) {
            if (child->getText() == "NOT") {
                hasNot = true;
                break;
            }
        }
        condition.type = hasNot? ConditionType::IS_NOT_NULL: ConditionType::IS_NULL;
        return condition;
    }
    virtual std::any visitWhere_in_list(SQLParser::Where_in_listContext *ctx) override {
        UnboundCondition ubc;
        ubc.type = ConditionType::IN_LIST;
        ubc.left = UnboundOprand{0,parseColumn(ctx->column())};
        ubc.values = parseValue_list(ctx->value_list());
        return ubc;
    }
    virtual std::any visitWhere_like_string(SQLParser::Where_like_stringContext *ctx) override {
        UnboundCondition ubc;
        ubc.type = ConditionType::LIKE;
        ubc.left = UnboundOprand{0,parseColumn(ctx->column())};
        string s = ctx->String()->getText();
        ubc.pattern = s.substr(1,s.size()-2);
        return ubc;
    }
    virtual std::any visitWhere_and_clause(SQLParser::Where_and_clauseContext *ctx) override {
        vector<UnboundCondition> vec;
        for(auto i:ctx->where_clause()){
            vec.push_back(any_cast<UnboundCondition>(visit(i)));
        }
        return vec;
    }
    virtual std::any visitWhere_in_select(SQLParser::Where_in_selectContext *ctx) override {
        UnboundCondition ubc;
        ubc.type = ConditionType::SUB_IN;
        ubc.left = UnboundOprand{0,parseColumn(ctx->column())};
        ubc.sub = make_shared<UnboundSelectRequest>(parseSelect(ctx->select_table()));
        return ubc;
    }
    virtual std::any visitWhere_operator_select(SQLParser::Where_operator_selectContext *ctx) override {
        UnboundCondition ubc;
        ubc.type = ConditionType::SUB_COMPARE;
        ubc.left = UnboundOprand{0,parseColumn(ctx->column())};
        ubc.op = parseOperator_(ctx->operator_());
        ubc.sub = make_shared<UnboundSelectRequest>(parseSelect(ctx->select_table()));
        return ubc;
    }
    UnboundSelectRequest parseSelect(SQLParser::Select_tableContext *ctx) {
        UnboundSelectRequest usr;
        usr.col_items = parseSelectors(ctx->selectors());
        if(usr.col_items == vector<pair<pair<string, string>, AggregateFunc>>{{{"", "*"}, AggregateFunc::NONE}})usr.star = 1;
        usr.FROM = parseIdentifiers(ctx->identifiers());
        if(ctx->where_part())usr.conditions = any_cast<vector<UnboundCondition>>(visit(ctx->where_part()->where_and_clause()));
        if(ctx->group_by_part())usr.GROUPBY = parseGroup_by_part(ctx->group_by_part());
        if(ctx->order_by_part())usr.ORDERBY = parseOrder_by_part(ctx->order_by_part());
        if(ctx->limit_part()){
            auto limit = parseLimit_part(ctx->limit_part());
            usr.LIMIT = limit.first;
            usr.OFFSET = limit.second;
        }
        else usr.LIMIT = -1, usr.OFFSET = 0;
        return usr;
    }
    virtual std::any visitSelect_table_(SQLParser::Select_table_Context *ctx) override {
        UnboundRequest res = parseSelect(ctx->select_table());
        return res;
    }
    virtual std::any visitInsert_into_table(SQLParser::Insert_into_tableContext *ctx) override {
        UnboundInsertRequest uir;
        uir.FROM = ctx->Identifier()->getText();
        uir.rows = parseValue_lists(ctx->value_lists());
        return UnboundRequest{move(uir)};
    }
    virtual std::any visitDelete_from_table(SQLParser::Delete_from_tableContext *ctx) override {
        UnboundDeleteRequest udr;
        udr.FROM = ctx->Identifier()->getText();
        if(ctx->where_and_clause())
            udr.conditions = any_cast<vector<UnboundCondition>>(visitWhere_and_clause(ctx->where_and_clause()));
        return UnboundRequest{move(udr)};
    }
    virtual std::any visitUpdate_table(SQLParser::Update_tableContext *ctx) override {
        UnboundUpdateRequest uur;
        uur.FROM = ctx->Identifier()->getText();
        if(ctx->where_and_clause())
            uur.conditions = any_cast<vector<UnboundCondition>>(visitWhere_and_clause(ctx->where_and_clause()));
        for(auto& i : parseSet_clause(ctx->set_clause())){
            uur.col_names.push_back(i.first);
            uur.values.push_back(i.second);
        }
        return UnboundRequest{move(uur)};
    }
    virtual std::any visitCreate_db(SQLParser::Create_dbContext *ctx) override {
        return UnboundRequest(SimpleRequest{Simple::CREATE_DATABASE,ctx->Identifier()->getText()});
    }
    virtual std::any visitDrop_db(SQLParser::Drop_dbContext *ctx) override {
        return UnboundRequest(SimpleRequest{Simple::DROP_DATABASE,ctx->Identifier()->getText()});
    }
    virtual std::any visitShow_dbs(SQLParser::Show_dbsContext *ctx) override {
        return UnboundRequest(SimpleRequest{Simple::SHOW_DATABASES});
    }
    virtual std::any visitUse_db(SQLParser::Use_dbContext *ctx) override {
        return UnboundRequest(SimpleRequest{Simple::USE_DATABASE,ctx->Identifier()->getText()});
    }
    virtual std::any visitShow_tables(SQLParser::Show_tablesContext *ctx) override {
        return UnboundRequest(SimpleRequest{Simple::SHOW_TABLES});
    }
    virtual std::any visitShow_indexes(SQLParser::Show_indexesContext *ctx) override {
        return UnboundRequest(SimpleRequest{Simple::SHOW_INDEXES});
    }
    virtual std::any visitCreate_table(SQLParser::Create_tableContext *ctx) override {
        CreateTableRequest ctr;
        Schema sc{};
        int col=0;
        int def_offset = 0;
        int offset;
        ctr.table_name = ctx->Identifier()->getText();
        if(ctr.table_name.empty() || ctr.table_name.size() >= MAX_STR)
            throw std::runtime_error("table name is too long");
        for(auto i:ctx->field_list()->field()){
            if(dynamic_cast<SQLParser::Normal_fieldContext*>(i) != nullptr){
                col++;
            }
        }
        if(col <= 0 || col > 32)
            throw std::runtime_error("invalid number of columns");
        sc.col = col;
        sc.nullen = (col+7)/8;
        offset = sc.nullen;
        col = 0;
        for(auto i:ctx->field_list()->field()){
            if(auto* normal = dynamic_cast<SQLParser::Normal_fieldContext*>(i)){
                string column_name = normal->Identifier()->getText();
                if(column_name.empty() || column_name.size() >= MAX_STR)
                    throw std::runtime_error("column name is too long");
                std::memcpy(sc.desc[col].name, column_name.c_str(), column_name.size() + 1);
                auto type = parseType_(normal->type_());
                sc.desc[col].type = type.first;
                sc.desc[col].len = type.second;
                bool not_null = false;
                for(auto* child : normal->children){
                    if(child->getText() == "NOT"){
                        not_null = true;
                        break;
                    }
                }
                sc.desc[col].nullable = !not_null;
                if(normal->value()){
                    sc.desc[col].defalutable = 1;
                    sc.desc[col].default_offset = def_offset;
                    Value def = parseValue(normal->value());
                    if(holds_alternative<bool>(def)){
                        if(!sc.desc[col].nullable)
                            throw std::runtime_error("NULL default on NOT NULL column");
                        sc.desc[col].default_offset = -1;
                    }
                    else if(sc.desc[col].type==0){
                        int* value = get_if<int>(&def);
                        if(value == nullptr)throw std::runtime_error("default value type mismatch");
                        if(def_offset > static_cast<int>(sizeof(sc.defval)) - sc.desc[col].len)
                            throw std::runtime_error("default values are too large");
                        memcpy(sc.defval+def_offset, value, sc.desc[col].len);
                        def_offset += sc.desc[col].len;
                    }
                    else if(sc.desc[col].type==1){
                        double* value = get_if<double>(&def);
                        if(value == nullptr)throw std::runtime_error("default value type mismatch");
                        if(def_offset > static_cast<int>(sizeof(sc.defval)) - sc.desc[col].len)
                            throw std::runtime_error("default values are too large");
                        memcpy(sc.defval+def_offset, value, sc.desc[col].len);
                        def_offset += sc.desc[col].len;
                    }
                    else if(sc.desc[col].type==2){
                        string* value = get_if<string>(&def);
                        if(value == nullptr || value->size() > static_cast<size_t>(sc.desc[col].len))
                            throw std::runtime_error("default string is too long or has wrong type");
                        if(def_offset > static_cast<int>(sizeof(sc.defval)) - sc.desc[col].len)
                            throw std::runtime_error("default values are too large");
                        memset(sc.defval+def_offset, 0, sc.desc[col].len);
                        memcpy(sc.defval+def_offset, value->data(), value->size());
                        def_offset += sc.desc[col].len;
                    }
                    else if(sc.desc[col].type==3){
                        string* value = get_if<string>(&def);
                        if(value == nullptr || !is_valid_date_string(*value))
                            throw std::runtime_error("invalid DATE default");
                        auto date = parse_date_yyyy_mm_dd(*value);
                        if(!date)throw std::runtime_error("invalid DATE default");
                        if(def_offset > static_cast<int>(sizeof(sc.defval)) - sc.desc[col].len)
                            throw std::runtime_error("default values are too large");
                        memcpy(sc.defval+def_offset, &date->date, sc.desc[col].len);
                        def_offset += sc.desc[col].len;
                    }
                }
                else sc.desc[col].defalutable = 0, sc.desc[col].default_offset = -1;
                sc.offset[col] = offset;
                offset += sc.desc[col].len;
                col++;
            }
            if(auto* pk = dynamic_cast<SQLParser::Primary_key_fieldContext*>(i)){
                AlterRequest req;
                req.kind = Alter::ADD_PRIMARY_KEY;
                req.table_name = ctr.table_name;
                if(pk->Identifier())req.constraint_name = pk->Identifier()->getText();
                for(auto j : pk->identifiers()->Identifier()) req.columns.push_back(j->getText());
                ctr.index_request.push_back(req);
            }
            if(auto* fk = dynamic_cast<SQLParser::Foreign_key_fieldContext*>(i)){
                AlterRequest req;
                req.kind = Alter::ADD_FOREIGN_KEY;
                req.table_name = ctr.table_name;
                if(fk->Identifier().size()==1){
                    req.referenced_table = fk->Identifier(0)->getText();
                }
                else{
                    req.constraint_name = fk->Identifier(0)->getText();
                    req.referenced_table = fk->Identifier(1)->getText();
                }
                for(auto j:fk->identifiers(0)->Identifier()) req.columns.push_back(j->getText());
                for(auto j:fk->identifiers(1)->Identifier()) req.referenced_columns.push_back(j->getText());
                ctr.index_request.push_back(req);
            }
            if(auto* uni = dynamic_cast<SQLParser::Unique_fieldContext*>(i)){
                AlterRequest req;
                req.kind = Alter::ADD_UNIQUE;
                req.table_name = ctr.table_name;
                if(uni->Identifier())req.constraint_name = uni->Identifier()->getText();
                for(auto j : uni->identifiers()->Identifier()) req.columns.push_back(j->getText());
                ctr.index_request.push_back(req);
            }
        }
        sc.offset[col] = offset;
        sc.recordsize = offset;
        sc.col = col;
        sc.nullen = (col+7)/8;
        if(!IsValidSchema(sc))
            throw std::runtime_error("invalid table schema");
        ctr.schema = move(sc);
        return UnboundRequest{move(ctr)};
    }
    virtual std::any visitDrop_table(SQLParser::Drop_tableContext *ctx) override {
        return UnboundRequest(SimpleRequest{Simple::DROP_TABLE, ctx->Identifier()->getText()});
    }
    virtual std::any visitDescribe_table(SQLParser::Describe_tableContext *ctx) override {
        return UnboundRequest(SimpleRequest{Simple::DESCRIBE_TABLE, ctx->Identifier()->getText()});
    }
    virtual std::any visitLoad_table(SQLParser::Load_tableContext *ctx) override {
        string s = ctx->String(0)->getText();
        LoadRequest req = {s.substr(1,s.size()-2) ,ctx->Identifier()->getText()};
        return UnboundRequest{move(req)};
    }
    
    virtual std::any visitAlter_add_index(SQLParser::Alter_add_indexContext *ctx) override {
        AlterRequest req;
        req.kind = Alter::ADD_INDEX;
        req.table_name = ctx->Identifier(0)->getText();
        if(ctx->Identifier().size()>1)req.constraint_name = ctx->Identifier(1)->getText();
        for(auto j : ctx->identifiers()->Identifier()) req.columns.push_back(j->getText());
        return UnboundRequest{move(req)};
    }
    virtual std::any visitAlter_drop_index(SQLParser::Alter_drop_indexContext *ctx) override {
        AlterRequest req;
        req.kind = Alter::DROP_INDEX;
        req.table_name = ctx->Identifier(0)->getText();
        req.constraint_name = ctx->Identifier(1)->getText();
        return UnboundRequest{move(req)};
    }
    virtual std::any visitAlter_table_drop_pk(SQLParser::Alter_table_drop_pkContext *ctx) override {
        AlterRequest req;
        req.kind = Alter::DROP_PRIMARY_KEY;
        req.table_name = ctx->Identifier(0)->getText();
        if(ctx->Identifier().size()>1)req.constraint_name = ctx->Identifier(1)->getText();
        return UnboundRequest{move(req)};
    }
    virtual std::any visitAlter_table_drop_foreign_key(SQLParser::Alter_table_drop_foreign_keyContext *ctx) override {
        AlterRequest req;
        req.kind = Alter::DROP_FOREIGN_KEY;
        req.table_name = ctx->Identifier(0)->getText();
        req.constraint_name = ctx->Identifier(1)->getText();
        return UnboundRequest{move(req)};
    }
    virtual std::any visitAlter_table_add_pk(SQLParser::Alter_table_add_pkContext *ctx) override {
        AlterRequest req;
        req.kind = Alter::ADD_PRIMARY_KEY;
        req.table_name = ctx->Identifier(0)->getText();
        if(ctx->Identifier().size()>1)req.constraint_name = ctx->Identifier(1)->getText();
        for(auto j : ctx->identifiers()->Identifier()) req.columns.push_back(j->getText());
        return UnboundRequest{move(req)};
    }
    virtual std::any visitAlter_table_add_foreign_key(SQLParser::Alter_table_add_foreign_keyContext *ctx) override {
        AlterRequest req;
        req.kind = Alter::ADD_FOREIGN_KEY;
        req.table_name = ctx->Identifier(0)->getText();
        if(ctx->Identifier().size()>2){
            req.constraint_name = ctx->Identifier(1)->getText();
            req.referenced_table = ctx->Identifier(2)->getText();
        }
        else{
            req.referenced_table = ctx->Identifier(1)->getText();
        }
        for(auto j : ctx->identifiers(0)->Identifier()) req.columns.push_back(j->getText());
        for(auto j : ctx->identifiers(1)->Identifier()) req.referenced_columns.push_back(j->getText());
        return UnboundRequest{move(req)};
    }
    virtual std::any visitAlter_table_add_unique(SQLParser::Alter_table_add_uniqueContext *ctx) override {
        AlterRequest req;
        req.kind = Alter::ADD_UNIQUE;
        req.table_name = ctx->Identifier(0)->getText();
        if(ctx->Identifier().size()>1)req.constraint_name = ctx->Identifier(1)->getText();
        for(auto j : ctx->identifiers()->Identifier()) req.columns.push_back(j->getText());
        return UnboundRequest{move(req)};
    }
    std::any visitStatement(SQLParser::StatementContext* ctx) override{
        if (ctx->db_statement()) {
            return visit(ctx->db_statement());
        }
        if (ctx->table_statement()) {
            return visit(ctx->table_statement());
        }
        if (ctx->alter_statement()) {
            return visit(ctx->alter_statement());
        }
        if (ctx->Null()) {
            return UnboundRequest{SimpleRequest{Simple::EMPTY, nullopt}};
        }
        throw std::runtime_error("invalid statement");
    }
    std::any visitProgram(SQLParser::ProgramContext* ctx) override{
        if(containsErrorNode(ctx))
            throw std::runtime_error("SQL syntax error");
        vector<UnboundRequest> requests;
        requests.reserve(ctx->statement().size());
        for (auto* statement : ctx->statement()) {
            std::any result = visit(statement);
            requests.push_back(std::any_cast<UnboundRequest>(std::move(result)));
        }
        return requests;
    }
};
