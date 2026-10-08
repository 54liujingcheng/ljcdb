#include "system_management.h"
#include <optional>
#include <string_view>
#include <charconv>
#pragma once
std::optional<DATE> parse_date_yyyy_mm_dd(std::string_view str);
constexpr bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}
constexpr bool is_valid_date(int year, int month, int day) {
    if (year <= 0 || month < 1 || month > 12 || day < 1) {
        return false;
    }
    constexpr int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max_days = days_in_month[month];
    if (month == 2 && is_leap_year(year)) {
        max_days = 29;
    }
    return day <= max_days;
}
bool is_valid_date_string(std::string_view str);
enum class ConditionType{
    COMPARE,IS_NULL,IS_NOT_NULL,IN_LIST,LIKE
};
enum class CompareOp{
    EQ, NE, LT, LE, GT, GE
};
enum class AggregateFunc{
    NONE, AVG, COUNT, MIN, MAX, SUM
};
enum class OrderBy{
    ASC, DESC
};
using Value = variant<int, double, string, bool, DATE>;
struct BoundColumn{
    int table_id=-1;//tables中的哪个表
    int cid=-1;//Schema中的哪一列
    int type=-1;
    int nullable=0;
    bool operator==(const BoundColumn& other) const {
        return table_id == other.table_id &&cid == other.cid;
    }
};
namespace std {
    template <>
    struct hash<BoundColumn> {
        size_t operator()(const BoundColumn& col) const noexcept {
            uint64_t key = (static_cast<uint64_t>(col.table_id) << 32) | 
                           (static_cast<uint32_t>(col.cid));
            return std::hash<uint64_t>{}(key);
        }
    };
}
struct Operand{
    bool is_val = true;
    BoundColumn col{};
    Value val{false};
};
struct Condition{
    ConditionType type=ConditionType::COMPARE;
    Operand left{};
    CompareOp op=CompareOp::EQ;
    Operand right{};
    vector<Value> values{};
    string pattern{};
};
struct UnboundOprand{
    bool is_val=true;
    pair<string,string> col_name{};
    Value val{false};
};
struct UnboundCondition{
    ConditionType type=ConditionType::COMPARE;
    UnboundOprand left{};
    CompareOp op = CompareOp::EQ;
    UnboundOprand right{};
    vector<Value> values{};
    string pattern{};
};
struct UnboundInsertRequest{
    string FROM;
    vector<Row> rows;
};
struct BoundInsertRequest{
    int table_id;
    vector<Row> rows;
    int errorcode=0;//ok->0 dupilicate->1 foreign->2 primary->3 date->4 else->5
};
BoundInsertRequest BindingInsertRequest(DbManager& dbm,UnboundInsertRequest& req);
struct UnboundDeleteRequest{
    string FROM;
    vector<UnboundCondition> conditions;
};
struct BoundDeleteRequest{
    int table_id;
    vector<Condition> conditions;
    int errorcode = 0;
};
pair<Condition, int> ConditionBindingHelper(Table* ltb,Table* rtb, UnboundCondition& i);
BoundDeleteRequest BindingDeleteRequest(DbManager& dbm,UnboundDeleteRequest& req);
struct UnboundUpdateRequest{
    string FROM;
    vector<pair<string,string>> col_names;
    vector<Value> values;
    vector<UnboundCondition> conditions;
};
struct BoundUpdateRequest{
    int table_id;
    vector<BoundColumn> col;
    vector<Value> values;
    vector<Condition> conditions;
    int errorcode = 0;
};
BoundUpdateRequest BindingUpdateRequest(DbManager& dbm,UnboundUpdateRequest& req);
struct UnboundSelectRequest{
    vector<string> FROM;
    vector<pair<pair<string,string>,AggregateFunc>> col_items;//两个都空、一个空一个是*表示COUNT(*)
    vector<pair<pair<string,string>,OrderBy>> ORDERBY;
    vector<pair<string,string>> GROUPBY;
    vector<UnboundCondition> conditions;
    int LIMIT;
    int OFFSET;
};
struct BoundSelectRequest{
    vector<int> table_ids;
    vector<pair<optional<BoundColumn>,AggregateFunc>> col_items;
    vector<pair<BoundColumn,OrderBy>> ORDERBY;
    vector<BoundColumn> GROUPBY;
    vector<Condition> conditions;
    int LIMIT;
    int OFFSET;
    int errorcode = 0;
};
BoundSelectRequest BindingSelectRequest(DbManager& dbm,UnboundSelectRequest& req);
struct Tuple{
    vector<Value> values;
};
struct LayoutEntry{
    string print_name;
    optional<BoundColumn> col;
};
struct Operator{
    vector<LayoutEntry> layout;
    unordered_map<BoundColumn, int> col_slot;
    DbManager* dbm = nullptr;
    bool opened = false;
    Operator(DbManager* _dbm):dbm(_dbm){;}
    virtual int Next(Tuple& output) = 0;
    virtual int Open() = 0;
    virtual void Close() = 0;
    virtual ~Operator() = default;
};
struct SeqScan : Operator{
    int table_id;
    Table* tb;
    ull rid;
    char* buf;
    SeqScan(DbManager* _dbm, int _tableid) : Operator(_dbm), table_id(_tableid) {
        tb = dbm->id_table[table_id];
        Schema& sc = tb->mng->tbmeta.schema;
        for(int i=0;i<sc.col;i++){
            BoundColumn tmp = {table_id,i,sc.desc[i].type, sc.desc[i].nullable};
            layout.push_back({string(tb->name) + "." + sc.desc[i].name, tmp});
            col_slot[tmp] = i;
        }
        buf = new char[sc.recordsize];
    }
    ~SeqScan() {
        delete[] buf;
        Close();
    } 
    int Open(){
        rid = tb->mng->FirstRecord();
        opened = true;
        return 0;
    }
    void Close(){
        rid = 0;
        opened = false;
        return ;
    }
    int Next(Tuple& output){
        if(opened == false)return -1;
        if(rid == 0)return 1;
        Schema& sc = tb->mng->tbmeta.schema;
        tb->mng->LoadRecord(rid, buf);
        deserialization(buf, output.values,sc);
        rid = tb->mng->NextRecord(rid);
        return 0;
    }
};
struct ExeOperand{
    bool is_val = true;
    int slot=-1;
    Value val{false};
};
struct ExeCondition{
    ConditionType type=ConditionType::COMPARE;
    ExeOperand left{};
    CompareOp op=CompareOp::EQ;
    ExeOperand right{};
    vector<Value> values{};
    string pattern{};
};
bool like(std::string_view text, std::string_view pattern);
struct Filter : Operator{
    unique_ptr<Operator> child;
    vector<ExeCondition> conditions;
    Filter(DbManager* _dbm, unique_ptr<Operator> ch, vector<ExeCondition> vec): Operator(_dbm){
        child = move(ch);
        layout = child->layout;
        col_slot = child->col_slot;
        /*for(auto i:vec){
            ExeCondition tmp;
            tmp.op = i.op, tmp.pattern = i.pattern, tmp.type = i.type, tmp.values = i.values;
            ExeOperand& left = tmp.left,&right = tmp.right;
            left.is_val = i.left.is_val;
            right.is_val = i.right.is_val;
            if(left.is_val)left.val = i.left.val;
            else left.slot = child->col_slot[i.left.col];
            if(right.is_val)right.val = i.right.val;
            else right.slot = child->col_slot[i.right.col];
            conditions.push_back(tmp);
        }*/
       conditions = move(vec);
    }
    ~Filter(){
        Close();
    }
    int Open(){
        if(child->Open()!=0)return -1;
        opened = true;
        return 0;
    }
    void Close(){
        opened = false;
        child->Close();
        return ;
    }
    int Next(Tuple& output){
        if(!opened)return -1;
        label:
        int res = child->Next(output);
        if(res == 1)return 1;
        if(res == -1)return -1;
        for(auto& i:conditions){
            if(i.type == ConditionType::COMPARE){
                Value *l,*r;
                if(i.left.is_val)l=&i.left.val;
                else l=&output.values[i.left.slot];
                if(i.right.is_val)r=&i.right.val;
                else r=&output.values[i.right.slot];
                if(holds_alternative<bool>(*l) || holds_alternative<bool>(*r))goto label;
                if(i.op == CompareOp::EQ && *l == *r || i.op == CompareOp::GE && *l >= *r 
                || i.op == CompareOp::GT && *l > *r || i.op == CompareOp::LE && *l <= *r
                || i.op == CompareOp::LT && *l < *r || i.op == CompareOp::NE && *l != *r)continue;
                else goto label;
            }
            if(i.type == ConditionType::IN_LIST){
                Value *l = &output.values[i.left.slot];
                if(holds_alternative<bool>(*l))goto label;
                if(find(i.values.begin(),i.values.end(),*l) != i.values.end())continue;
                else goto label;
            }
            if(i.type == ConditionType::IS_NULL){
                Value *l = &output.values[i.left.slot];
                if(holds_alternative<bool>(*l))continue;
                else goto label;
            }
            if(i.type == ConditionType::IS_NOT_NULL){
                Value *l = &output.values[i.left.slot];
                if(!holds_alternative<bool>(*l))continue;
                else goto label;
            }
            if(i.type == ConditionType::LIKE){
                Value *l = &output.values[i.left.slot];
                if(holds_alternative<bool>(*l))goto label;
                if(like(get<string>(*l),i.pattern))continue;
                else goto label;
            }
        }
        return 0;
    }
};
struct Project : Operator{
    unique_ptr<Operator> child;
    Tuple input_tuple;
    vector<int> input_slots;
    Project(DbManager* dbm, unique_ptr<Operator> ch, BoundSelectRequest& bsr):Operator(dbm){
        child = move(ch);
        int cnt=0;
        for(auto i:bsr.col_items){
            if(!i.first || i.second != AggregateFunc::NONE)continue;
            for(auto j:child->layout)
            if(j.col == i.first){
                layout.push_back(j);
                input_slots.push_back(child->col_slot.at(j.col.value()));
                col_slot[i.first.value()] = cnt;
                cnt++;
                break;
            }
        }
    }
    ~Project(){
        Close();
    }
    void Close(){
        opened = false;
        child->Close();
        return ;
    }
    int Open(){
        if(child->Open()!=0)return -1;
        opened = true;
        return 0;
    }
    int Next(Tuple& output){
        output.values.clear();
        if(!opened)return -1;
        int res = child->Next(input_tuple);
        if(res == 1)return 1;
        if(res == -1)return -1;
        for(auto i:input_slots){
            output.values.push_back(input_tuple.values[i]);
        }
        input_tuple.values.clear();
        return 0;
    }
};
struct Limit : Operator{
    int limit;
    int offset;
    int idx;
    unique_ptr<Operator> child;
    Limit(DbManager* dbm, unique_ptr<Operator> ch, BoundSelectRequest& bsr):Operator(dbm){
        child = move(ch);
        limit = bsr.LIMIT;
        offset = bsr.OFFSET;
        layout = child->layout;
        col_slot = child->col_slot;
    }
    ~Limit(){
        Close();
    }
    void Close(){
        opened = false;
        idx=0;
        child->Close();
        return ;
    }
    int Open(){
        if(child->Open()!=0)return -1;
        idx=0;
        opened = true;
        return 0;
    }
    int Next(Tuple& output){
        if(!opened)return -1;
        if(limit == 0)return 1;
        while(idx<offset){
            int res = child->Next(output);
            if(res == 1)return 1;
            if(res==-1)return -1;
            idx++;
        }
        if(idx<offset+limit){
            int res = child->Next(output);
            if(res == 1)return 1;
            if(res==-1)return -1;
            idx++;
            return 0;
        }
        return 1;
    }
};
struct BNLJ : Operator{
    unique_ptr<Operator> inner_child,outer_child;
    vector<Tuple> outer_batch;
    int batch_size;
    int outer_idx;
    int outer_col;
    int finished;
    Tuple outer_input;
    Tuple inner_tuple;
    vector<ExeCondition> conditions;
    BNLJ(DbManager* dbm, unique_ptr<Operator> _outer_child, unique_ptr<Operator> _inner_child, vector<ExeCondition> vec): Operator(dbm){
        conditions = move(vec);
        outer_child = move(_outer_child);
        inner_child = move(_inner_child);
        layout = outer_child->layout;
        layout.insert(layout.end(),inner_child->layout.begin(),inner_child->layout.end());
        col_slot = outer_child->col_slot;
        outer_col = outer_child->layout.size();
        for(auto i:inner_child->col_slot){
            col_slot[i.first] = i.second + outer_col;
        }
        outer_idx = 0;
        finished = 0;
        int total_len = 0;
        for(auto i:outer_child->layout){
            if(i.col){
                Table* tb = dbm->id_table[i.col.value().table_id];
                total_len += tb->mng->tbmeta.schema.desc[i.col.value().cid].len;
            }
        }
        if(total_len!=0)batch_size = 32*PAGE_SIZE / total_len;
        else finished = 1;
        #ifdef BATCH_SIZE
        batch_size = BATCH_SIZE
        #endif
        outer_batch.reserve(batch_size);
    }
    void Close(){
        opened = false;
        inner_child->Close();
        outer_child->Close();
        outer_batch.clear();
        outer_input.values.clear();
        outer_idx=0;
        finished = 1;
        return ;
    }
    int Open(){
        finished = 0;
        if(outer_child->Open()!=0)return -1;
        if(inner_child->Open()!=0){
            outer_child->Close();
            return -1;
        }
        outer_idx = 0;
        opened = true;
        for(int i=0;i<batch_size;i++){
            int res = outer_child->Next(outer_input);
            if(res == -1)return -1;
            if(res == 1)break;
            outer_batch.push_back(outer_input);
            outer_input.values.clear();
        }
        int res = inner_child->Next(inner_tuple);
        if(res == -1)return -1;
        else if(res == 1)finished = 1;
        return 0;
    }
    int Next(Tuple& output){
        if(!opened)return -1;
        if(finished)return 1;
        label:
        if(outer_idx == outer_batch.size()){
            int res = inner_child->Next(inner_tuple);
            if(res == -1)return -1;
            if(res == 1){
                inner_child->Close();
                if(inner_child->Open()!=0)return -1;
                int res = inner_child->Next(inner_tuple);
                if(res==-1)return -1;
                if(res==1)return 1;
                outer_batch.clear();
                for(int i=0;i<batch_size;i++){
                    int res = outer_child->Next(outer_input);
                    if(res == -1)return -1;
                    if(res == 1)break;
                    outer_batch.push_back(outer_input);
                    outer_input.values.clear();
                }
            }
            outer_idx = 0;
            if(outer_batch.size()==0)return 1;
        }
        Tuple& outer_tuple = outer_batch[outer_idx++];
        for(auto& i:conditions){
            if(i.type == ConditionType::COMPARE){
                Value *l,*r;
                if(i.left.is_val)l=&i.left.val;
                else if(i.left.slot<outer_col)l=&outer_tuple.values[i.left.slot];
                else l=&inner_tuple.values[i.left.slot-outer_col];
                if(i.right.is_val)r=&i.right.val;
                else if(i.right.slot<outer_col)r=&outer_tuple.values[i.right.slot];
                else r=&inner_tuple.values[i.right.slot-outer_col];
                if(holds_alternative<bool>(*l) || holds_alternative<bool>(*r)){
                    goto label;outer_idx++;
                }
                if(i.op == CompareOp::EQ && *l == *r || i.op == CompareOp::GE && *l >= *r 
                || i.op == CompareOp::GT && *l > *r || i.op == CompareOp::LE && *l <= *r
                || i.op == CompareOp::LT && *l < *r || i.op == CompareOp::NE && *l != *r)continue;
                else goto label;
            }
            if(i.type == ConditionType::IN_LIST){
                Value *l;
                if(i.left.slot<outer_col)l=&outer_tuple.values[i.left.slot];
                else l=&inner_tuple.values[i.left.slot-outer_col];
                if(holds_alternative<bool>(*l))goto label;
                if(find(i.values.begin(),i.values.end(),*l) != i.values.end())continue;
                else goto label;
            }
            if(i.type == ConditionType::IS_NULL){
                Value *l;
                if(i.left.slot<outer_col)l=&outer_tuple.values[i.left.slot];
                else l=&inner_tuple.values[i.left.slot-outer_col];
                if(holds_alternative<bool>(*l))continue;
                else goto label;
            }
            if(i.type == ConditionType::IS_NOT_NULL){
                Value *l;
                if(i.left.slot<outer_col)l=&outer_tuple.values[i.left.slot];
                else l=&inner_tuple.values[i.left.slot-outer_col];
                if(!holds_alternative<bool>(*l))continue;
                else goto label;
            }
            if(i.type == ConditionType::LIKE){
                Value *l;
                if(i.left.slot<outer_col)l=&outer_tuple.values[i.left.slot];
                else l=&inner_tuple.values[i.left.slot-outer_col];
                if(holds_alternative<bool>(*l))goto label;
                if(like(get<string>(*l),i.pattern))continue;
                else goto label;
            }
        }
        output = outer_tuple;
        output.values.insert(output.values.end(), inner_tuple.values.begin(), inner_tuple.values.end());
        return 0;
    }
};
