#include "execute.h"
#include "query.h"
#include "third_party/antlr4-cpp-runtime-4.13.2-source/runtime/src/antlr4-runtime.h"
#include "parser/generated/SQLLexer.h"
#include "parser/generated/SQLParser.h"
#include <chrono>
#include <iomanip>
#include <format>
string errmsg[8] = {"OK", "duplicate", "foreign", "primary", "date", "null", "else"};
class StreamTable {
private:
    std::vector<std::string> headers_;
    std::vector<size_t> widths_;
    bool has_printed_header_ = false;
    bool has_data_rows_ = false;
    void print_repeat(const char* str, size_t count) const {
        for (size_t i = 0; i < count; ++i) {
            std::printf("%s", str);
        }
    }
    void print_divider(const char* left, const char* mid, const char* right) const {
        std::printf("%s", left);
        for (size_t i = 0; i < widths_.size(); ++i) {
            print_repeat("─", widths_[i] + 2);
            std::printf("%s", (i == widths_.size() - 1 ? right : mid));
        }
        std::printf("\n");
    }
    std::vector<std::string> wrap_text(const std::string& text, size_t width) const {
        std::vector<std::string> lines;
        if (text.empty()) {
            lines.push_back("");
            return lines;
        }
        size_t start = 0;
        while (start < text.length()) {
            lines.push_back(text.substr(start, width));
            start += width;
        }
        return lines;
    }
public:
    StreamTable(const std::vector<std::string>& headers, const std::vector<size_t>& widths)
        : headers_(headers), widths_(widths) {}
    void print_header() {
        print_divider("┌", "┬", "┐");
        std::printf("│");
        for (size_t i = 0; i < headers_.size(); ++i) {
            std::printf(" %-*s │", (int)widths_[i], headers_[i].c_str());
        }
        std::printf("\n");
        print_divider("├", "┼", "┤");
        has_printed_header_ = true;
    }
    void print_row(const std::vector<std::string>& row_data) {
        if (!has_printed_header_) {
            print_header();
        }
        if (has_data_rows_) {
            print_divider("├", "┼", "┤");
        }
        has_data_rows_ = true;
        std::vector<std::vector<std::string>> wrapped_columns;
        size_t max_lines = 1;
        for (size_t i = 0; i < row_data.size(); ++i) {
            auto lines = wrap_text(row_data[i], widths_[i]);
            max_lines = std::max(max_lines, lines.size());
            wrapped_columns.push_back(lines);
        }
        for (size_t line_idx = 0; line_idx < max_lines; ++line_idx) {
            std::printf("│");
            for (size_t col_idx = 0; col_idx < row_data.size(); ++col_idx) {
                std::string_view cell_text = "";
                if (line_idx < wrapped_columns[col_idx].size()) {
                    cell_text = wrapped_columns[col_idx][line_idx];
                }
                std::printf(" %-*s │", (int)widths_[col_idx], cell_text.data());
            }
            std::printf("\n");
        }
    }
    void finish() {
        if (!has_printed_header_) {
            print_header();
        }
        print_divider("└", "┴", "┘");
    }
};
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
constexpr long long dur(auto start, auto end){
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}
void runSQLJC(DbManager& dbm,string SQLJC, bool mode){//0->交互 1->批处理 other->静默
    vector<UnboundRequest> res = parseSQLJC(SQLJC);
    for(auto& i:res){
        auto start = std::chrono::high_resolution_clock::now();
        if(auto* simple = get_if<SimpleRequest>(&i)){
            if(simple->command == Simple::CREATE_DATABASE){
                int res = dbm.CreateDb(simple->name.value());
                auto end = std::chrono::high_resolution_clock::now();
                if(res == 0){
                    if(mode == 1)printf("@Time-consumed: %lldms\n", dur(start,end));
                    else if(mode == 0)printf("Succeed in creating the datebase.Time-consumed: %lldms\n",dur(start,end));
                }
                else if(res==-1){
                    if(mode == 1)printf("!ERROR\nelse\n@Time-consumed: %lldms\n", dur(start,end));
                    else if(mode == 0)printf("Fail in creating the datebase.Time-consumed: %lldms\n", dur(start,end));
                }
                else if(res==-2){
                    if(mode == 1)printf("@Time-consumed: %lldms\n", dur(start,end));
                    else if(mode == 0)printf("There's a datebase with same name.Time-consumed: %lldms\n",dur(start,end));
                }
            }
            else if(simple->command == Simple::DROP_DATABASE){
                int res = dbm.DropDb(simple->name.value());
                auto end = std::chrono::high_resolution_clock::now();
                if(res == 0){
                    if(mode == 1)printf("@Time-consumed: %lldms\n", dur(start,end));
                    else if(mode == 0)printf("Succeed in dropping the datebase.Time-consumed: %lldms\n",dur(start,end));
                }else{
                    if(mode == 1)printf("!ERROR\nelse\n@Time-consumed: %lldms\n", dur(start,end));
                    else if(mode == 0)printf("Fail in dropping the datebase.Time-consumed: %lldms\n", dur(start,end));
                }
            }
            else if(simple->command == Simple::USE_DATABASE){
                int res = dbm.UseDb(simple->name.value());
                auto end = std::chrono::high_resolution_clock::now();
                if(res == 0 || res == -2){
                    if(mode == 1)printf("@Time-consumed: %lldms\n", dur(start,end));
                    else if(mode == 0&&res == 0)printf("Switching into this datebase.Time-consumed: %lldms\n",dur(start,end));
                    else if(mode == 0&&res == -2)printf("You have been using this datebase so far.Time-consumed: %lldms\n",dur(start,end));
                }else if(res == -1){
                    if(mode == 1)printf("!ERROR\nelse\n@Time-consumed: %lldms\n", dur(start,end));
                    else if(mode == 0)printf("Fail in using the datebase.Time-consumed: %lldms\n", dur(start,end));
                }
            }
            else if(simple->command == Simple::DROP_TABLE){
                int res = dbm.DropTable(simple->name.value().data());
                auto end = std::chrono::high_resolution_clock::now();
                if(res){
                    if(mode == 1)printf("!ERROR\n%s\n@Time-consumed: %lldms\n", errmsg[res].c_str(), dur(start,end));
                    else if(mode == 0)printf("Fail in dropping the table: %s.Time-consumed: %lldms\n", errmsg[res].c_str(), dur(start,end));
                }
                else{
                    if(mode == 1)printf("@Time-consumed: %lldms\n", dur(start,end));
                    else if(mode == 0)printf("Succeed in dropping the table.Time-consumed: %lldms\n", dur(start,end));
                }
            }
            else if(simple->command == Simple::SHOW_DATABASES){
                vector<string> vec = dbm.ShowAllDb();
                if(mode == 1){
                    printf("DATABASES\n");
                    for(auto& i:vec){
                        printf("%s\n", i.c_str());
                    }
                    printf("@\n");
                }
                else{
                    printf("Databases:\n");
                    for(auto& i:vec){
                        printf("%s\n", i.c_str());
                    }
                }
            }
            else if(simple->command == Simple::SHOW_TABLES){
                if(mode == 1){
                    printf("TABLES\n");
                    for(auto& i:dbm.name_table){
                        printf("%s\n", i.first.c_str());
                    }
                    printf("@\n");
                }
                else{
                    printf("Tables:\n");
                    for(auto& i:dbm.name_table){
                        printf("%s\n", i.first.c_str());
                    }
                }
            }
            else if(simple->command == Simple::SHOW_INDEXES){
                if(mode == 1){
                    printf("INDEXES\n");
                    for(auto& i:dbm.id_idx){
                        printf("%s\n", i.second->file_name.c_str());
                    }
                    printf("@\n");
                }
                else{
                    printf("Indexes\n");
                    for(auto& i:dbm.id_idx){
                        printf("%s\n", i.second->file_name.c_str());
                    }
                }
            }
            else if(simple->command == Simple::DESCRIBE_TABLE){
                if(!dbm.name_table.count(simple->name.value())){
                    if(mode == 1)printf("!ERROR\nelse\n@\n");
                    else if(mode == 0)printf("The table does not exist.\n");
                    fflush(stdout);
                    goto cont;
                }
                printDESC(dbm, dbm.name_table[simple->name.value()], mode);
            }
            else if(simple->command == Simple::EMPTY){
                if(mode == 1)printf("@\n");
                else printf("\n");
            }
            fflush(stdout);
        }else if(auto* alter = get_if<AlterRequest>(&i)){
            string msg;
            int res = HandleAlter(dbm, get<AlterRequest>(i), msg);
            auto end = std::chrono::high_resolution_clock::now();
            if(res){
                if(mode == 1)printf("!ERROR\n%s\n@Time-consumed: %lldms\n", errmsg[res].c_str(), dur(start,end));
                else if(mode == 0)printf("Fail in %s: %s.Time-consumed: %lldms\n",msg.c_str(), errmsg[res].c_str(), dur(start,end));
            }
            else{
                if(mode == 1)printf("@Time-consumed: %lldms\n", dur(start,end));
                else if(mode == 0)printf("Succeed in %s.Time-consumed: %lldms\n",msg.c_str() ,dur(start,end));
            }
            fflush(stdout);
        }else if(auto* ctr = get_if<CreateTableRequest>(&i)){
            int errcode = 0;
            dbm.CreateTable(ctr->table_name.data(), ctr->schema, errcode);
            if(errcode){
                auto end = std::chrono::high_resolution_clock::now();
                if(mode == 1)printf("!ERROR\n%s\n@Time-consumed: %lldms\n", errmsg[errcode].c_str(), dur(start,end));
                else if(mode == 0)printf("Fail in creating the table: %s.Time-consumed: %lldms\n", errmsg[errcode].c_str(), dur(start,end));
                fflush(stdout);
                goto cont;
            }
            for(auto i:ctr->index_request){
                string msg;
                int res = HandleAlter(dbm, i, msg);
                if(res){
                    dbm.DropTable(ctr->table_name.data());
                    auto end = std::chrono::high_resolution_clock::now();
                    if(mode == 1)printf("!ERROR\n%s\n@Time-consumed: %lldms\n", errmsg[res].c_str(), dur(start,end));
                    else if(mode == 0)printf("Fail in %s during creating the table: %s.Time-consumed: %lldms\n",msg.c_str(), errmsg[res].c_str(), dur(start,end));
                    fflush(stdout);
                    goto cont;
                }
            }
            auto end = std::chrono::high_resolution_clock::now();
            if(mode==1)printf("@Time-consumed: %lldms\n", dur(start,end));
            else if(mode==0)printf("Succeed in creating the table.Time-consumed: %lldms\n", dur(start,end));
            fflush(stdout);
        }else if(auto* load = get_if<LoadRequest>(&i)){
            int res = LoadCSV(dbm, load->table_name, load->file_name);
            auto end = std::chrono::high_resolution_clock::now();
            if(mode==1)printf("rows\n%d\n@Time-consumed: %lldms\n",res, dur(start,end));
            else if(mode==0)printf("Load %d rows into %s.Time-consumed: %lldms\n", res, load->table_name.c_str(), dur(start,end));
            fflush(stdout);
        }else if(auto* uir = get_if<UnboundInsertRequest>(&i)){
            auto tmp = BindingInsertRequest(dbm, (*uir));
            if(tmp.errorcode != OK){
                auto end = std::chrono::high_resolution_clock::now();
                if(mode == 1)printf("!ERROR\n%s\n@Time-consumed: %lldms\n", errmsg[tmp.errorcode].c_str(), dur(start, end));
                else if(mode == 0)printf("Fail in binding the command: %s.Time-consumed: %lldms\n", errmsg[tmp.errorcode].c_str(), dur(start, end));
                fflush(stdout);
                goto cont;
            }
            auto res = InsertExecuter(dbm, tmp);
            auto end = std::chrono::high_resolution_clock::now();
            printDML(res, dur(start,end), mode, "in inserting columns");
            fflush(stdout);
        }else if(auto* udr = get_if<UnboundDeleteRequest>(&i)){
            auto tmp = BindingDeleteRequest(dbm, (*udr));
            if(tmp.errorcode != OK){
                auto end = std::chrono::high_resolution_clock::now();
                if(mode == 1)printf("!ERROR\n%s\n@Time-consumed: %lldms\n", errmsg[tmp.errorcode].c_str(), dur(start, end));
                else if(mode == 0)printf("Fail in binding the command: %s.Time-consumed: %lldms\n", errmsg[tmp.errorcode].c_str(), dur(start, end));
                fflush(stdout);
                goto cont;
            }
            auto res = DeleteExecuter(dbm, tmp);
            auto end = std::chrono::high_resolution_clock::now();
            printDML(res, dur(start,end), mode, "in deleting selected columns");
            fflush(stdout);
        }else if(auto* uur = get_if<UnboundUpdateRequest>(&i)){
            auto tmp = BindingUpdateRequest(dbm, (*uur));
            if(tmp.errorcode != OK){
                auto end = std::chrono::high_resolution_clock::now();
                if(mode == 1)printf("!ERROR\n%s\n@Time-consumed: %lldms\n", errmsg[tmp.errorcode].c_str(), dur(start, end));
                else if(mode == 0)printf("Fail in binding the command: %s.Time-consumed: %lldms\n", errmsg[tmp.errorcode].c_str(), dur(start, end));
                fflush(stdout);
                goto cont;
            }
            auto res = UpdateExecuter(dbm, tmp);
            auto end = std::chrono::high_resolution_clock::now();
            printDML(res, dur(start,end), mode, "in updating selected columns");
            fflush(stdout);
        }else if(auto* usr = get_if<UnboundSelectRequest>(&i)){
            auto tmp = BindingSelectRequest(dbm, (*usr));
            if(tmp.errorcode != OK){
                auto end = std::chrono::high_resolution_clock::now();
                if(mode == 1)printf("!ERROR\n%s\n@Time-consumed: %lldms\n", errmsg[tmp.errorcode].c_str(), dur(start, end));
                else if(mode == 0)printf("Fail in binding the command: %s.Time-consumed: %lldms\n", errmsg[tmp.errorcode].c_str(), dur(start, end));
                fflush(stdout);
                goto cont;
            }
            unique_ptr<Operator> root = move(Planner(&dbm, tmp));
            if(!root){
                if(mode == 1)printf("!ERROR\nelse\n@\n");
                else if(mode == 0)printf("Fail in processiing the query\n");
                fflush(stdout);
                goto cont;
            }
            if(!usr->star && root->layout.size() == usr->col_items.size()){
                for(size_t j = 0; j < usr->col_items.size(); ++j){
                    const auto& [column, agg] = usr->col_items[j];
                    const auto& [table_name, column_name] = column;
                    if(agg == AggregateFunc::COUNT && column_name.empty()){
                        root->layout[j].print_name = "COUNT(*)";
                        continue;
                    }
                    string name = table_name.empty()? column_name : table_name + "." + column_name;
                    if(agg == AggregateFunc::NONE)root->layout[j].print_name = name;
                    else if(agg == AggregateFunc::AVG)root->layout[j].print_name = "AVG(" + name + ")";
                    else if(agg == AggregateFunc::COUNT)root->layout[j].print_name = "COUNT(" + name + ")";
                    else if(agg == AggregateFunc::MAX)root->layout[j].print_name = "MAX(" + name + ")";
                    else if(agg == AggregateFunc::MIN)root->layout[j].print_name = "MIN(" + name + ")";
                    else if(agg == AggregateFunc::SUM)root->layout[j].print_name = "SUM(" + name + ")";
                }
            }
            int res = root->Open();
            if(res!=0){
                root->Close();
                if(mode == 1)printf("!ERROR\nelse\n@\n");
                else if(mode == 0)printf("Fail in processiing the query\n");
                fflush(stdout);
                goto cont;
            }
            Tuple output;
            unique_ptr<StreamTable> table;
            vector<string> head;
            vector<size_t> width;
            if(mode == 1){
                for(int i=0;i<root->layout.size()-1;i++){
                    printf("%s,", root->layout[i].print_name.c_str());
                }
                printf("%s\n", root->layout.back().print_name.c_str());
            }
            else if(mode == 0){
                for(int i=0;i<root->layout.size();i++){
                    head.push_back(root->layout[i].print_name);
                    int tp = 0;
                    if(root->layout[i].col){
                        tp = root->layout[i].col->type;
                        if(root->layout[i].agg == AggregateFunc::AVG)tp = 1;
                        if(root->layout[i].agg == AggregateFunc::COUNT)tp = 0;
                        if(root->layout[i].agg == AggregateFunc::SUM && tp == 0)tp=4;
                    }
                    int wide;
                    if(tp == 0)wide = 8;
                    else if(tp == 1)wide = 12;
                    else if(tp == 2)wide = 24;
                    else if(tp==3)wide = 10;
                    else if(tp == 4)wide = 14;
                    wide = max(wide, (int)root->layout[i].print_name.size());
                    width.push_back(wide);
                }
                table = make_unique<StreamTable>(head, width);
            }
            int rows = 0;
            while(1){
                int res = root->Next(output);
                if(res == -1){
                    root->Close();
                    if(mode == 1)printf("!ERROR\nelse\n@\n");
                    else if(mode == 0)printf("Fail in processiing the query\n");
                    fflush(stdout);
                    goto cont;
                }
                else if(res == 1){
                    break;
                }
                if(mode == 1){
                    for(int i = 0;i < output.values.size()-1;i++){
                        if(holds_alternative<bool>(output.values[i])){
                            printf("NULL,");
                        }
                        if(holds_alternative<int>(output.values[i])){
                            printf("%d,", get<int>(output.values[i]));
                        }
                        if(holds_alternative<double>(output.values[i])){
                            printf("%.2f,", get<double>(output.values[i]));
                        }
                        if(holds_alternative<string>(output.values[i])){
                            printf("%s,", get<string>(output.values[i]).c_str());
                        }
                        if(holds_alternative<DATE>(output.values[i])){
                            int yyyy = get<DATE>(output.values[i]).date / 10000;
                            int mm = (get<DATE>(output.values[i]).date / 100) % 100;
                            int dd = get<DATE>(output.values[i]).date % 100;
                            printf("%04d-%02d-%02d,",yyyy, mm, dd);
                        }
                        if(holds_alternative<long long>(output.values[i])){
                            printf("%lld,", get<long long>(output.values[i]));
                        }
                    }
                    if(holds_alternative<bool>(output.values.back())){
                        printf("NULL\n");
                    }
                    if(holds_alternative<int>(output.values.back())){
                        printf("%d\n", get<int>(output.values.back()));
                    }
                    if(holds_alternative<double>(output.values.back())){
                        printf("%.2f\n", get<double>(output.values.back()));
                    }
                    if(holds_alternative<string>(output.values.back())){
                        printf("%s\n", get<string>(output.values.back()).c_str());
                    }
                    if(holds_alternative<DATE>(output.values.back())){
                        int yyyy = get<DATE>(output.values.back()).date / 10000;
                        int mm = (get<DATE>(output.values.back()).date / 100) % 100;
                        int dd = get<DATE>(output.values.back()).date % 100;
                        printf("%04d-%02d-%02d\n",yyyy, mm, dd);
                    }
                    if(holds_alternative<long long>(output.values.back())){
                        printf("%lld\n", get<long long>(output.values.back()));
                    }
                }else if(mode == 0){
                    vector<string> out;
                    for(int i = 0;i < output.values.size();i++){
                        if(holds_alternative<bool>(output.values[i])){
                            out.push_back("NULL");
                        }
                        if(holds_alternative<int>(output.values[i])){
                            out.push_back(format("{}", get<int>(output.values[i])));
                        }
                        if(holds_alternative<double>(output.values[i])){
                            out.push_back(format("{:.2f}", get<double>(output.values[i])));
                        }
                        if(holds_alternative<string>(output.values[i])){
                            out.push_back(get<string>(output.values[i]));
                        }
                        if(holds_alternative<DATE>(output.values[i])){
                            int yyyy = get<DATE>(output.values[i]).date / 10000;
                            int mm = (get<DATE>(output.values[i]).date / 100) % 100;
                            int dd = get<DATE>(output.values[i]).date % 100;
                            out.push_back(format("{:04d}-{:02d}-{:02d}", yyyy, mm, dd));
                        }
                        if(holds_alternative<long long>(output.values[i])){
                            out.push_back(format("{}", get<long long>(output.values[i])));
                        }
                    }
                    table->print_row(out);
                }
                rows++;
            }
            root->Close();
            if(mode == 1){
                auto end = std::chrono::high_resolution_clock::now();
                printf("@Time-consumed: %lldms\n", dur(start, end));
            }
            else if(mode == 0){
                table->finish();
                auto end = std::chrono::high_resolution_clock::now();
                printf("Succeeding processing the query with %d rows.Time-consumed: %lldms\n",rows , dur(start, end));
            }
            fflush(stdout);
        }
        cont:
        ;
    }
}
void printDML(pair<int, int> output, long long duration, bool mode, const string& msg){
    if(mode == 1){
        if(output.second!=OK){
            printf("!ERROR\n%s\n@Time-consumed: %lldms\n", errmsg[output.second].c_str(), duration);
        }else{
            printf("rows\n%d\n@Time-consumed: %lldms\n", output.first, duration);
        }
    }else if(mode == 0){
        if(output.second!=OK){
            printf("Fail in %s: %s. Applied: %d.Time-consumed: %lldms\n",msg.c_str(), errmsg[output.second].c_str(), output.first, duration);
        }else{
            printf("Succeed in %s. Applied: %d.Time-consumed: %lldms\n",msg.c_str() ,output.first,duration);
        }
    }
}
int HandleAlter(DbManager& dbm, AlterRequest& req, string& msg){
    if(req.kind == Alter::ADD_INDEX){
        if(!dbm.name_table.count(req.table_name))return ERR_ELSE;
        Table* tb = dbm.name_table[req.table_name];
        vector<int> cid;
        for(auto& i:req.columns){
            if(!tb->name_cid.count(i))return ERR_ELSE;
            cid.push_back(tb->name_cid[i]);
        }
        int errcode=0;
        if(!req.constraint_name){
            req.constraint_name = format("idx_{}",dbm.next_idxid);
        }
        dbm.CreateIndex(tb->name, req.constraint_name.value().data(), cid.size(), cid, 0, 0, 1, errcode);
        if(errcode)return errcode;
        msg = "adding an index";
        return OK;
    }
    if(req.kind == Alter::ADD_PRIMARY_KEY){
        if(!dbm.name_table.count(req.table_name))return ERR_ELSE;
        Table* tb = dbm.name_table[req.table_name];
        vector<int> cid;
        for(auto& i:req.columns){
            if(!tb->name_cid.count(i))return ERR_ELSE;
            cid.push_back(tb->name_cid[i]);
        }
        if(!req.constraint_name)req.constraint_name = "pk_" + string(tb->name);
        int errcode=0;
        dbm.CreateIndex(tb->name, req.constraint_name.value().data(), cid.size(), cid, 1, 1, 0, errcode);
        if(errcode)return errcode;
        msg = "adding an primary key";
        return OK;
    }
    if(req.kind == Alter::ADD_FOREIGN_KEY){
        if(!dbm.name_table.count(req.table_name))return ERR_ELSE;
        Table* tb = dbm.name_table[req.table_name];
        vector<int> ccid;
        for(auto& i:req.columns){
            if(!tb->name_cid.count(i))return ERR_ELSE;
            ccid.push_back(tb->name_cid[i]);
        }
        char* _name = (req.constraint_name?req.constraint_name.value().data():nullptr);
        if(!dbm.name_table.count(req.referenced_table.value()))return ERR_ELSE;
        Table* patb = dbm.name_table[req.referenced_table.value()];
        if(patb->pkid == -1)return ERR_FORIEGN;
        Index* idx = dbm.id_idx[patb->pkid];
        int j=0;
        if(req.referenced_columns.size() != idx->col)return ERR_ELSE;
        for(auto& i:req.referenced_columns){
            if(!patb->name_cid.count(i) || patb->name_cid[i] != idx->cid[j++])return ERR_ELSE;
        }
        int errcode=0;
        dbm.CreateForeign(_name, patb->id, tb->id, ccid, errcode);
        if(errcode)return errcode;
        msg = "adding a foreign key";
        return OK;
    }
    if(req.kind == Alter::ADD_UNIQUE){
        if(!dbm.name_table.count(req.table_name))return ERR_ELSE;
        Table* tb = dbm.name_table[req.table_name];
        vector<int> cid;
        for(auto& i:req.columns){
            if(!tb->name_cid.count(i))return ERR_ELSE;
            cid.push_back(tb->name_cid[i]);
        }
        int errcode=0;
        if(!req.constraint_name){
            req.constraint_name = format("uni_{}",dbm.next_idxid);
        }
        dbm.CreateIndex(tb->name, req.constraint_name.value().data(), cid.size(), cid, 1, 0, 1, errcode);
        if(errcode)return errcode;
        msg = "adding an unique key";
        return OK;
    }
    if(req.kind == Alter::DROP_FOREIGN_KEY){
        if(!dbm.name_table.count(req.table_name))return ERR_ELSE;
        int res = dbm.DropForeign(req.table_name.data(), req.constraint_name.value().data());
        if(res)return res;
        msg = "dropping a foreign key";
        return OK;
    }
    if(req.kind == Alter::DROP_INDEX){
        if(!dbm.name_table.count(req.table_name))return ERR_ELSE;
        int res = dbm.DropIndex(req.table_name.data(), req.constraint_name.value().data());
        if(res)return res;
        msg = "dropping an index";
        return OK;
    }
    if(req.kind == Alter::DROP_PRIMARY_KEY){
        if(!dbm.name_table.count(req.table_name))return ERR_ELSE;
        Table* tb = dbm.name_table[req.table_name];
        if(tb->pkid == -1)return ERR_PRIMARY;
        char* name = dbm.id_idx[tb->pkid]->name;
        if(req.constraint_name && req.constraint_name.value() != string(name))return ERR_ELSE;
        int res = dbm.DropIndex(tb->name, name);
        if(res)return res;
        msg = "dropping a primary key";
        return OK;
    }
    return ERR_ELSE;
}
void printDESC(DbManager& dbm, Table* tb, bool mode){
    if(mode == 1){
        printf("Field,Type,Null,Default\n");
        Schema& sc = tb->mng->tbmeta.schema;
        for(int i=0;i<sc.col;i++){
            printf("%s,",sc.desc[i].name);
            if(sc.desc[i].type==0)printf("INT,");
            else if(sc.desc[i].type == 1)printf("FLOAT,");
            else if(sc.desc[i].type == 2)printf("VARCHAR(%d),",sc.desc[i].len);
            else if(sc.desc[i].type == 3)printf("DATE,");
            int flag=0;
            if(tb->pkid != -1)for(int j:dbm.id_idx[tb->pkid]->cid){
                if(j == i)flag = 1;
            }
            if(sc.desc[i].nullable && !flag)printf("YES,");
            else printf("NO,");
            if(!sc.desc[i].defalutable || sc.desc[i].default_offset == -1)printf("NULL\n");
            else{
                if(sc.desc[i].type==0){
                    int val;
                    memcpy(&val,sc.defval+sc.desc[i].default_offset,sc.desc[i].len);
                    printf("%d\n",val);
                }
                else if(sc.desc[i].type == 1){
                    double val;
                    memcpy(&val,sc.defval+sc.desc[i].default_offset,sc.desc[i].len);
                    printf("%.2f\n",val);
                }
                else if(sc.desc[i].type == 2){
                    memcpy(dbm.buf,sc.defval+sc.desc[i].default_offset,sc.desc[i].len);
                    dbm.buf[sc.desc[i].len] = '\0';
                    printf("%s\n",dbm.buf);
                }
                else if(sc.desc[i].type == 3){
                    DATE val;
                    memcpy(&val,sc.defval+sc.desc[i].default_offset,sc.desc[i].len);
                    printf("%04d-%02d-%02d\n",val.date/10000, (val.date/100)%100, val.date %100);
                }
            }
        }
        printf("\n");
        if(tb->pkid!=-1){
            printf("PRIMARY KEY (");
            vector<int>& res = dbm.id_idx[tb->pkid]->cid;
            for(int i=0;i<res.size()-1;i++){
                printf("%s,",sc.desc[res[i]].name);
            }
            printf("%s);\n",sc.desc[res[res.size()-1]].name);
        }
        for(auto i:dbm.id_outfk[tb->id]){
            printf("FOREIGN KEY (");
            vector<int>& res = dbm.id_idx[i->ch_idx_id]->cid;
            for(int j=0;j<res.size()-1;j++){
                printf("%s,",sc.desc[res[j]].name);
            }
            printf("%s) REFERENCES ",sc.desc[res[res.size()-1]].name);
            char* paname = dbm.id_table[i->pa_table_id]->name;
            Table* pa = dbm.id_table[i->pa_table_id];
            Schema& sc2 = pa->mng->tbmeta.schema;
            printf("%s(",paname);
            vector<int>& res2 = dbm.id_idx[i->pa_idx_id]->cid;
            for(int j=0;j<res2.size()-1;j++){
                printf("%s,",sc2.desc[res2[j]].name);
            }
            printf("%s);\n",sc2.desc[res2[res2.size()-1]].name);
        }
        vector<Index*> indexs, uniques;
        for(auto i:dbm.id_normidx[tb->id]){
            if(!i->implicit){
                if(i->uni)uniques.push_back(i);
                else indexs.push_back(i);
            }
        }
        for(auto i:uniques){
            printf("UNIQUE (");
            vector<int>& res = i->cid;
            for(int j=0;j<res.size()-1;j++){
                printf("%s,",sc.desc[res[j]].name);
            }
            printf("%s);\n",sc.desc[res[res.size()-1]].name);
        }
        for(auto i:indexs){
            printf("INDEX (");
            vector<int>& res = i->cid;
            for(int j=0;j<res.size()-1;j++){
                printf("%s,",sc.desc[res[j]].name);
            }
            printf("%s);\n",sc.desc[res[res.size()-1]].name);
        }
        printf("@\n");
    }else if(mode == 0){
        std::vector<std::string> headers = {"Field", "Type", "Null", "Default"};
        std::vector<size_t> widths = {15, 15, 6, 10};
        StreamTable table(headers, widths);
        Schema& sc = tb->mng->tbmeta.schema;
        for(int i=0;i<sc.col;i++){
            string field = string(sc.desc[i].name);
            string type;
            if(sc.desc[i].type==0)type = "INT";
            else if(sc.desc[i].type == 1)type = "FLOAT";
            else if(sc.desc[i].type == 2)type = format("VARCHAR({})", sc.desc[i].len);
            else if(sc.desc[i].type == 3)type = "DATE";
            int flag=0;
            if(tb->pkid != -1)for(int j:dbm.id_idx[tb->pkid]->cid){
                if(j == i)flag = 1;
            }
            string null;
            if(sc.desc[i].nullable && !flag)null = "Yes";
            else null = "No";
            string defval;
            if(!sc.desc[i].defalutable || sc.desc[i].default_offset == -1)defval = "NULL";
            else{
                if(sc.desc[i].type==0){
                    int val;
                    memcpy(&val,sc.defval+sc.desc[i].default_offset,sc.desc[i].len);
                    defval = format("{}", val);
                }
                else if(sc.desc[i].type == 1){
                    double val;
                    memcpy(&val,sc.defval+sc.desc[i].default_offset,sc.desc[i].len);
                    defval = format("{:.2f}", val);
                }
                else if(sc.desc[i].type == 2){
                    memcpy(dbm.buf,sc.defval+sc.desc[i].default_offset,sc.desc[i].len);
                    dbm.buf[sc.desc[i].len] = '\0';
                    defval = string(dbm.buf);
                }
                else if(sc.desc[i].type == 3){
                    DATE val;
                    memcpy(&val,sc.defval+sc.desc[i].default_offset,sc.desc[i].len);
                    defval = format("{:04d}-{:02d}-{:02d}",val.date/10000, (val.date/100)%100, val.date %100);
                }
            }
            table.print_row({field, type, null, defval});
        }
        table.finish();
        if(tb->pkid!=-1){
            printf("PRIMARY KEY (");
            vector<int>& res = dbm.id_idx[tb->pkid]->cid;
            for(int i=0;i<res.size()-1;i++){
                printf("%s,",sc.desc[res[i]].name);
            }
            printf("%s);\n",sc.desc[res[res.size()-1]].name);
        }
        for(auto i:dbm.id_normidx[tb->id]){
            if(!i->implicit){
                if(i->uni)printf("UNIQUE (");
                else printf("INDEX (");
                vector<int>& res = i->cid;
                for(int j=0;j<res.size()-1;j++){
                    printf("%s,",sc.desc[res[j]].name);
                }
                printf("%s);\n",sc.desc[res[res.size()-1]].name);
            }
        }
        for(auto i:dbm.id_outfk[tb->id]){
            printf("FOREIGN KEY (");
            vector<int>& res = dbm.id_idx[i->ch_idx_id]->cid;
            for(int j=0;j<res.size()-1;j++){
                printf("%s,",sc.desc[res[j]].name);
            }
            printf("%s) REFERENCES ",sc.desc[res[res.size()-1]].name);
            char* paname = dbm.id_table[i->pa_table_id]->name;
            Table* pa = dbm.id_table[i->pa_table_id];
            Schema& sc2 = pa->mng->tbmeta.schema;
            printf("%s(",paname);
            vector<int>& res2 = dbm.id_idx[i->pa_idx_id]->cid;
            for(int j=0;j<res2.size()-1;j++){
                printf("%s,",sc2.desc[res2[j]].name);
            }
            printf("%s);\n",sc2.desc[res2[res2.size()-1]].name);
        }
        printf("\n");
    }
}
