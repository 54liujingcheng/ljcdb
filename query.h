#include "system_management.h"
#include <optional>
#include <string_view>
#include <charconv>
#include <ranges>
#include <queue>
#include <functional>
#include <memory>
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
    COMPARE,IS_NULL,IS_NOT_NULL,IN_LIST,LIKE,SUB_IN,SUB_COMPARE
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
using Value = variant<int, double, string, bool, DATE, long long>;
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
struct BoundSelectRequest;
struct UnboundSelectRequest;
struct Condition{
    ConditionType type=ConditionType::COMPARE;
    Operand left{};
    CompareOp op=CompareOp::EQ;
    Operand right{};
    vector<Value> values{};
    string pattern{};
    shared_ptr<BoundSelectRequest> sub;
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
    shared_ptr<UnboundSelectRequest> sub;
};
struct UnboundInsertRequest{
    string FROM;
    vector<Row> rows;
};
struct BoundInsertRequest{
    int table_id;
    vector<Row> rows;
    int errorcode=0;//ok->0 dupilicate->1 foreign->2 primary->3 date->4 null->5 else->6
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
pair<Condition, int> ConditionBindingHelper(Table* ltb,Table* rtb, UnboundCondition& i, DbManager& dbm);
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
    int LIMIT = -1;
    int OFFSET = 0;
    bool star = 0;
};
struct BoundSelectRequest{
    vector<int> table_ids;
    vector<pair<optional<BoundColumn>,AggregateFunc>> col_items;
    vector<pair<BoundColumn,OrderBy>> ORDERBY;
    vector<BoundColumn> GROUPBY;
    vector<Condition> conditions;
    int LIMIT = -1;
    int OFFSET = 0;
    int errorcode = 0;
    bool star = 0;
};
BoundSelectRequest BindingSelectRequest(DbManager& dbm,UnboundSelectRequest& req);
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
struct Tuple{
    vector<Value> values;
    bool operator == (const Tuple& other) const {
        return other.values == values;
    }
    optional<ull> rid = nullopt;
};
namespace std {
    template <>
    struct hash<Tuple> {
        std::size_t operator()(const Tuple& p) const noexcept {
            std::size_t seed = 0;
            for(auto i:p.values){
                hash_combine(seed, i);
            }
            return seed;
        }
    };
}
struct LayoutEntry{
    string print_name;
    optional<BoundColumn> col;
    AggregateFunc agg = AggregateFunc::NONE;
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
unique_ptr<Operator> Planner(DbManager*, BoundSelectRequest&);
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
            layout.push_back({string(tb->name) + "." + sc.desc[i].name, tmp, AggregateFunc::NONE});
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
        output.rid = rid;
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
    Filter(DbManager* _dbm, unique_ptr<Operator> ch, vector<Condition>& vec): Operator(_dbm){
        child = move(ch);
        layout = child->layout;
        col_slot = child->col_slot;
        for(auto& i:vec){
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
        }
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
    BNLJ(DbManager* dbm, unique_ptr<Operator> _outer_child, unique_ptr<Operator> _inner_child, vector<Condition>& vec): Operator(dbm){
        outer_child = move(_outer_child);
        inner_child = move(_inner_child);
        layout = outer_child->layout;
        layout.insert(layout.end(),inner_child->layout.begin(),inner_child->layout.end());
        col_slot = outer_child->col_slot;
        outer_col = outer_child->layout.size();
        for(auto i:inner_child->col_slot){
            col_slot[i.first] = i.second + outer_col;
        }
        for(auto i:vec){
            ExeCondition tmp;
            tmp.op = i.op, tmp.pattern = i.pattern, tmp.type = i.type, tmp.values = i.values;
            ExeOperand& left = tmp.left,&right = tmp.right;
            left.is_val = i.left.is_val;
            right.is_val = i.right.is_val;
            if(left.is_val)left.val = i.left.val;
            else left.slot = col_slot.at(i.left.col);
            if(right.is_val)right.val = i.right.val;
            else right.slot = col_slot.at(i.right.col);
            conditions.push_back(tmp);
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
        if(total_len!=0)batch_size = 1024;
        else finished = 1;
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
                    goto label;
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
struct IndexScan : Operator{
    int table_id;
    Table* tb;
    Index* idx;
    SearchIter iter;
    char* buf;
    bool linclusive;
    bool rinclusive;
    char* l_key;
    char* r_key;
    IndexScan(DbManager* _dbm, Index* _idx, Row& lrow, Row& rrow, bool linc, bool rinc) : Operator(_dbm){
        table_id = _idx->table_id;
        idx = _idx;
        tb = dbm->id_table[table_id];
        Schema& sc = tb->mng->tbmeta.schema;
        for(int i=0;i<sc.col;i++){
            BoundColumn tmp = {table_id,i,sc.desc[i].type, sc.desc[i].nullable};
            layout.push_back({string(tb->name) + "." + sc.desc[i].name, tmp, AggregateFunc::NONE});
            col_slot[tmp] = i;
        }
        buf = new char[sc.recordsize];
        l_key = new char[idx->mng->keylen];
        r_key = new char[idx->mng->keylen];
        index_serialization(lrow, l_key, sc, idx);
        index_serialization(rrow, r_key, sc, idx);
        linclusive = linc;
        rinclusive = rinc;
    }
    ~IndexScan() {
        delete[] buf;
        delete[] l_key;
        delete[] r_key;
        Close();
    } 
    int Open(){
        opened = true;
        iter = idx->mng->SearchRange(l_key, r_key, linclusive, rinclusive);
        return 0;
    }
    void Close(){
        iter = SearchIter{};
        opened = false;
        return ;
    }
    int Next(Tuple& output){
        if(!opened)return -1;
        Schema& sc = tb->mng->tbmeta.schema;
        if(idx->mng->Valid(iter, 1)){
            tb->mng->LoadRecord(iter.rec_rid, buf);
            deserialization(buf,output.values , sc);
            output.rid = iter.rec_rid;
            idx->mng->Next(iter);
            return 0;
        }
        else return 1;
    }
};
struct SlotType{
    int type;
    int len;
    int offset;
    int nullable;
};
int tuple_serialization(Row& src, vector<char>& dest, vector<SlotType>& slots, int row_size, int offset);
int tuple_deserialization(vector<char>& src, Row& dest, vector<SlotType>& slots, int row_size, int offset);
struct Sort : Operator{
    unique_ptr<Operator> child;
    int row_size;
    vector<SlotType> slots;
    vector<char> data;
    vector<pair<int, OrderBy>> sortkeys;
    vector<int> rows;
    Tuple input;
    int cursor;
    int limit = -1;
    int Offset;
    Sort(DbManager* dbm, unique_ptr<Operator> ch, BoundSelectRequest& bsr, int _limit = -1, int _offset = 0):Operator(dbm){
        child = move(ch);
        layout = child->layout;
        col_slot = child->col_slot;
        for (const auto& [column, direction] : bsr.ORDERBY) {
            sortkeys.push_back({child->col_slot.at(column),direction});
        }
        int tmp=0;
        int col = layout.size(), nullen = (col+7)/8, offset = nullen;
        for(auto i:layout){
            if(i.agg == AggregateFunc::COUNT){
                slots.push_back({0,4,offset,0}), offset+=4;
            }
            else if(i.agg == AggregateFunc::AVG){
                slots.push_back({1,8,offset,1}), offset+=8;
            }
            else if(i.agg == AggregateFunc::SUM && i.col.value().type==0){
                slots.push_back({4,8,offset,1}), offset+=8;
            }
            else if(i.col){
                Table* tb = dbm->id_table[i.col.value().table_id];
                Schema& sc = tb->mng->tbmeta.schema;
                slots.push_back({i.col.value().type, sc.desc[i.col.value().cid].len, offset, i.col.value().nullable});
                offset += sc.desc[i.col.value().cid].len;
            }
        }
        row_size = offset;
        data.reserve(4*1024*1024);
        rows.reserve(1024*1024);
        cursor = 0;
        limit = _limit;
        Offset = _offset;
    }
    int OpenFullSort(){
        int tmpcursor = 0;
        cursor = 0;
        data.clear();
        rows.clear();
        int res= child->Open();
        if(res==1)return 1;
        if(res==-1){
            child->Close(); return -1;
        }
        while(1){
            int res = child->Next(input);
            if(res==1)break;
            if(res==-1){
                child->Close();
                return -1;
            }
            if(data.size() < (tmpcursor+1)*row_size)data.resize((tmpcursor+1)*row_size);
            if(tuple_serialization(input.values, data, slots, row_size, tmpcursor*row_size)==-1){
                child->Close();
                return -1;
            }
            rows.push_back(tmpcursor);
            tmpcursor++;
        }
        child->Close();
        sort(rows.begin(), rows.end(), [this](int lid, int rid){
            for(int i=0;i<this->sortkeys.size();i++){
                int type = this->slots[sortkeys[i].first].type;
                int offset = this->slots[sortkeys[i].first].offset;
                bool lisnull = KeyColumnIsNull(this->data.data()+lid*row_size, sortkeys[i].first);
                bool risnull = KeyColumnIsNull(this->data.data()+rid*row_size, sortkeys[i].first);
                if(lisnull && risnull)continue;
                else if(lisnull && !risnull){
                    if(sortkeys[i].second == OrderBy::ASC)return true;
                    else return false;
                }
                else if(!lisnull && risnull){
                    if(sortkeys[i].second == OrderBy::ASC)return false;
                    else return true;
                }
                if(type==0){
                    int l,r;
                    memcpy(&l, this->data.data()+lid*row_size+offset, 4);
                    memcpy(&r, this->data.data()+rid*row_size+offset, 4);
                    if(l==r)continue;
                    if(sortkeys[i].second == OrderBy::ASC){
                        return l<r;
                    }
                    else return l>r;
                }else if(type==1){
                    double l,r;
                    memcpy(&l, this->data.data()+lid*row_size+offset, 8);
                    memcpy(&r, this->data.data()+rid*row_size+offset, 8);
                    if(l==r)continue;
                    if(sortkeys[i].second == OrderBy::ASC){
                        return l<r;
                    }
                    else return l>r;
                }else if(type==3){
                    DATE l,r;
                    memcpy(&l.date, this->data.data()+lid*row_size+offset, 4);
                    memcpy(&r.date, this->data.data()+rid*row_size+offset, 4);
                    if(l==r)continue;
                    if(sortkeys[i].second == OrderBy::ASC){
                        return l<r;
                    }
                    else return l>r;
                }else if(type==2){
                    char* l=this->data.data()+lid*row_size+offset,*r=this->data.data()+rid*row_size+offset;
                    if(sortkeys[i].second == OrderBy::ASC){
                        if(memcmp(l, r, this->slots[sortkeys[i].first].len)<0)return true;
                        else if(memcmp(l, r, this->slots[sortkeys[i].first].len)>0)return false;
                    }
                    else{
                        if(memcmp(l, r, this->slots[sortkeys[i].first].len)<0)return false;
                        else if(memcmp(l, r, this->slots[sortkeys[i].first].len)>0)return true;
                    }
                }else if(type==4){
                    long long l,r;
                    memcpy(&l, this->data.data()+lid*row_size+offset, 8);
                    memcpy(&r, this->data.data()+rid*row_size+offset, 8);
                    if(l==r)continue;
                    if(sortkeys[i].second == OrderBy::ASC){
                        return l<r;
                    }
                    else return l>r;
                }
            }
            return false;
        }
        );
        opened = true;
        return 0;
    }
    int OpenTopK(){
        data.resize((limit+Offset+1)*row_size);
        rows.clear();
        int res= child->Open();
        if(res==1)return 1;
        if(res==-1){
            child->Close(); return -1;
        }
        if(limit == 0){
            opened = true;
            child->Close();
            return 0;
        }
        auto cmp = [this](int lid, int rid){
            for(int i=0;i<this->sortkeys.size();i++){
                int type = this->slots[sortkeys[i].first].type;
                int offset = this->slots[sortkeys[i].first].offset;
                bool lisnull = KeyColumnIsNull(this->data.data()+lid*row_size, sortkeys[i].first);
                bool risnull = KeyColumnIsNull(this->data.data()+rid*row_size, sortkeys[i].first);
                if(lisnull && risnull)continue;
                else if(lisnull && !risnull){
                    if(sortkeys[i].second == OrderBy::ASC)return true;
                    else return false;
                }
                else if(!lisnull && risnull){
                    if(sortkeys[i].second == OrderBy::ASC)return false;
                    else return true;
                }
                if(type==0){
                    int l,r;
                    memcpy(&l, this->data.data()+lid*row_size+offset, 4);
                    memcpy(&r, this->data.data()+rid*row_size+offset, 4);
                    if(l==r)continue;
                    if(sortkeys[i].second == OrderBy::ASC){
                        return l<r;
                    }
                    else return l>r;
                }else if(type==1){
                    double l,r;
                    memcpy(&l, this->data.data()+lid*row_size+offset, 8);
                    memcpy(&r, this->data.data()+rid*row_size+offset, 8);
                    if(l==r)continue;
                    if(sortkeys[i].second == OrderBy::ASC){
                        return l<r;
                    }
                    else return l>r;
                }else if(type==3){
                    DATE l,r;
                    memcpy(&l.date, this->data.data()+lid*row_size+offset, 4);
                    memcpy(&r.date, this->data.data()+rid*row_size+offset, 4);
                    if(l==r)continue;
                    if(sortkeys[i].second == OrderBy::ASC){
                        return l<r;
                    }
                    else return l>r;
                }else if(type==2){
                    char* l=this->data.data()+lid*row_size+offset,*r=this->data.data()+rid*row_size+offset;
                    if(sortkeys[i].second == OrderBy::ASC){
                        if(memcmp(l, r, this->slots[sortkeys[i].first].len)<0)return true;
                        else if(memcmp(l, r, this->slots[sortkeys[i].first].len)>0)return false;
                    }
                    else{
                        if(memcmp(l, r, this->slots[sortkeys[i].first].len)<0)return false;
                        else if(memcmp(l, r, this->slots[sortkeys[i].first].len)>0)return true;
                    }
                }else if(type==4){
                    long long l,r;
                    memcpy(&l, this->data.data()+lid*row_size+offset, 8);
                    memcpy(&r, this->data.data()+rid*row_size+offset, 8);
                    if(l==r)continue;
                    if(sortkeys[i].second == OrderBy::ASC){
                        return l<r;
                    }
                    else return l>r;
                }
            }
            return false;
        };
        priority_queue<int, vector<int>, decltype(cmp)> pq(cmp);
        int flag = 0;
        for(int i=0;i<limit+Offset;i++){
            int res = child->Next(input);
            if(res == 1){
                flag = 1;
                break;
            }
            if(res == -1){
                child->Close();
                return -1;
            }
            if(tuple_serialization(input.values,data , slots, row_size, i*row_size)!=0){
                child->Close();
                return -1;
            }
            pq.push(i);
        }
        if(!flag)
        while(1){
            int res = child->Next(input);
            if(res == 1){
                flag = 1;
                break;
            }
            if(res == -1){
                child->Close();
                return -1;
            }
            int top = pq.top();
            if(tuple_serialization(input.values,data , slots, row_size, (limit+Offset)*row_size)){
                child->Close();
                return -1;
            }
            if(cmp(top, limit+Offset) == false){
                pq.pop();
                memcpy(data.data()+top*row_size, data.data()+(limit+Offset)*row_size, row_size);
                pq.push(top);
            }
        }
        while(!pq.empty()){
            rows.push_back(pq.top());
            pq.pop();
        }
        sort(rows.begin(), rows.end(), cmp);
        opened = true;
        child->Close();
        cursor = 0;
        return 0;
    }
    int Open(){
        if(limit > -1)return OpenTopK();
        else return OpenFullSort();
    }
    int Next(Tuple& output){
        if(!opened)return -1;
        if(cursor>=rows.size())return 1;
        tuple_deserialization(data, output.values, slots, row_size, row_size*rows[cursor]);
        cursor++;
        return 0;
    }
    void Close(){
        cursor = 0;
        opened = false;
        rows.clear();
        data.clear();
    }
};
struct AggState{
    int cnt=0;
    long long int_sum=0;
    double double_sum=0.0;
    optional<Value> maxm=nullopt, minm=nullopt;
};
struct Agg{
    AggregateFunc func;
    int slot;
    int type;
};
struct Aggregate: Operator{
    unique_ptr<Operator> child;
    unordered_map<Tuple, vector<AggState>> gpkey_aggs;
    vector<int> gpslots;
    vector<Agg> aggs;
    int aggs_size;
    Tuple input;
    std::unordered_map<Tuple, vector<AggState>>::iterator cur;
    vector<pair<bool, int>> outref;//0->gpslots中的位置, 1->aggs&gpkey_aggs中的位置
    Aggregate(DbManager* dbm, unique_ptr<Operator> ch, BoundSelectRequest& bsr) : Operator(dbm) {
        child = move(ch);
        gpkey_aggs.clear();
        for(auto i:bsr.GROUPBY){
            gpslots.push_back(child->col_slot[i]);
        }
        int tmp=0;
        for(auto i:bsr.col_items){
            if(i.second != AggregateFunc::NONE){
                if(!i.first)aggs.push_back({AggregateFunc::COUNT, -1, 0});
                else aggs.push_back({i.second, child->col_slot[i.first.value()], i.first.value().type});
            }
            LayoutEntry I;
            if(!i.first){
                I.print_name = "COUNT(*)";
            }
            else{
                string print_name = child->layout[child->col_slot[i.first.value()]].print_name;
                if(i.second == AggregateFunc::AVG)I.print_name = "AVG(" + print_name + ")";
                else if(i.second == AggregateFunc::COUNT){
                    I.print_name = "COUNT(" + print_name + ")";
                }
                else if(i.second == AggregateFunc::MAX)I.print_name = "MAX(" + print_name + ")";
                else if(i.second == AggregateFunc::MIN)I.print_name = "MIN(" + print_name + ")";
                else if(i.second == AggregateFunc::SUM)I.print_name = "SUM(" + print_name + ")";
                else{
                    col_slot[i.first.value()] = tmp;
                    I.print_name = print_name;
                }
            }
            if(i.second == AggregateFunc::NONE){
                int group_index = -1;
                for(int j = 0;j < bsr.GROUPBY.size();++j){
                    if(bsr.GROUPBY[j] == i.first.value()){
                        group_index = j;
                        break;
                    }
                }
                if(group_index == -1){
                    aggs.push_back({AggregateFunc::NONE, child->col_slot[i.first.value()], i.first.value().type});
                    outref.push_back({1, aggs.size()-1});
                }
                else outref.push_back({0, group_index});
            }
            else outref.push_back({1, aggs.size()-1});
            I.agg = i.second;
            I.col = i.first;
            layout.push_back(I);
            tmp++;
        }
        aggs_size = aggs.size();
    }
    int Open(){
        int res = child->Open();
        if(res == -1){
            child->Close();
            return -1;
        }
        else if(res == 1){
            return 1;
        }
        if(gpslots.empty()){
            gpkey_aggs[Tuple{}] = vector<AggState>(aggs_size);
        }
        while(1){
            int res = child->Next(input);
            if(res == -1){
                child->Close();
                return -1;
            }
            if(res == 1){
                break;
            }
            Tuple tmp;
            for(auto i:gpslots)tmp.values.push_back(input.values[i]);
            if(!gpkey_aggs.count(tmp))for(int i=0;i<aggs_size;i++)gpkey_aggs[tmp].push_back({});
            vector<AggState>& state = gpkey_aggs[tmp];
            for(int i=0;i<aggs_size;i++){
                if(aggs[i].slot == -1){
                    state[i].cnt++;
                    continue;
                }
                int slot = aggs[i].slot,type = aggs[i].type;
                if(holds_alternative<bool>(input.values[slot]) && aggs[i].func != AggregateFunc::NONE)continue;
                if(aggs[i].func == AggregateFunc::AVG || aggs[i].func == AggregateFunc::SUM){
                    if(type == 0){
                        state[i].int_sum += get<int>(input.values[slot]);
                        state[i].cnt++;
                    }
                    else if(type == 1){
                        state[i].double_sum += get<double>(input.values[slot]);
                        state[i].cnt++;
                    }
                }
                else if(aggs[i].func == AggregateFunc::COUNT){
                    state[i].cnt++;
                }
                else if(aggs[i].func == AggregateFunc::MAX){
                    if(type == 0){
                        if(!state[i].maxm)state[i].maxm = get<int>(input.values[slot]);
                        else state[i].maxm = max(get<int>(state[i].maxm.value()), get<int>(input.values[slot]));
                    }
                    else if(type == 1){
                        if(!state[i].maxm)state[i].maxm = get<double>(input.values[slot]);
                        else state[i].maxm = max(get<double>(state[i].maxm.value()), get<double>(input.values[slot]));
                    }
                    else if(type == 2){
                        if(!state[i].maxm)state[i].maxm = get<string>(input.values[slot]);
                        else state[i].maxm = max(get<string>(state[i].maxm.value()), get<string>(input.values[slot]));
                    }
                    else if(type == 3){
                        if(!state[i].maxm)state[i].maxm = get<DATE>(input.values[slot]);
                        else state[i].maxm = max(get<DATE>(state[i].maxm.value()), get<DATE>(input.values[slot]));
                    }
                }
                else if(aggs[i].func == AggregateFunc::MIN){
                    if(type == 0){
                        if(!state[i].minm)state[i].minm = get<int>(input.values[slot]);
                        else state[i].minm = min(get<int>(state[i].minm.value()), get<int>(input.values[slot]));
                    }
                    else if(type == 1){
                        if(!state[i].minm)state[i].minm = get<double>(input.values[slot]);
                        else state[i].minm = min(get<double>(state[i].minm.value()), get<double>(input.values[slot]));
                    }
                    else if(type == 2){
                        if(!state[i].minm)state[i].minm = get<string>(input.values[slot]);
                        else state[i].minm = min(get<string>(state[i].minm.value()), get<string>(input.values[slot]));
                    }
                    else if(type == 3){
                        if(!state[i].minm)state[i].minm = get<DATE>(input.values[slot]);
                        else state[i].minm = min(get<DATE>(state[i].minm.value()), get<DATE>(input.values[slot]));
                    }
                }
                else if(aggs[i].func == AggregateFunc::NONE){
                    if(!state[i].maxm){
                        if(holds_alternative<bool>(input.values[slot])){
                            state[i].maxm = (bool)false;
                        }
                        else if(type == 0){
                            state[i].maxm = get<int>(input.values[slot]);
                        }
                        else if(type == 1){
                            state[i].maxm = get<double>(input.values[slot]);
                        }
                        else if(type == 2){
                            state[i].maxm = get<string>(input.values[slot]);
                        }
                        else if(type == 3){
                            state[i].maxm = get<DATE>(input.values[slot]);
                        }
                    }
                    else{
                        if(state[i].maxm!=input.values[slot]){
                            child->Close();
                            return -1;
                        }
                    }
                }
            }
        }
        child->Close();
        opened=1;
        cur = gpkey_aggs.begin();
        return 0;
    }
    void Close(){
        gpkey_aggs.clear();
        opened = 0;
    }
    int Next(Tuple& output){
        if(!opened)return -1;
        if(cur == gpkey_aggs.end())return 1;
        output.values.clear();
        output.values.resize(outref.size());
        auto&[gpkey, state] = *cur;
        for(int i=0;i<outref.size();i++){
            if(outref[i].first == 0){
                output.values[i] = gpkey.values[outref[i].second];
            }
            else{
                int slot = aggs[outref[i].second].slot,type = aggs[outref[i].second].type;
                AggregateFunc func = aggs[outref[i].second].func;
                AggState& states = state[outref[i].second];
                if(func == AggregateFunc::AVG){
                    if(states.cnt==0)output.values[i] = (bool)false;
                    else if(type == 0){
                        output.values[i] = (double)states.int_sum / states.cnt;
                    }
                    else if(type == 1){
                        output.values[i] = states.double_sum / states.cnt;
                    }
                }
                else if(func == AggregateFunc::COUNT){
                    output.values[i] = states.cnt;
                }
                else if(func == AggregateFunc::SUM){
                    if(states.cnt==0)output.values[i] = (bool)false;
                    else if(type == 0){
                        output.values[i] = (long long)states.int_sum;
                    }
                    else if(type == 1){
                        output.values[i] = states.double_sum;
                    }
                }
                else if(func == AggregateFunc::MAX){
                    if(!states.maxm)output.values[i] = (bool)false;
                    else output.values[i] = states.maxm.value();
                }
                else if(func == AggregateFunc::MIN){
                    if(!states.minm)output.values[i] = (bool)false;
                    else output.values[i] = states.minm.value();
                }
                else if(func == AggregateFunc::NONE){
                    if(!states.maxm)output.values[i] = (bool)false;
                    else output.values[i] = states.maxm.value();
                }
            }
        }
        cur++;
        return 0;
    }
};
struct INLJ: Operator{
    unique_ptr<Operator> outer_child;
    int inner_table_id;
    Table* inner_table;
    Index* inner_index;
    Tuple outer_tuple;
    Tuple inner_tuple;
    Row key;
    SearchIter iter;
    char* record_buf;
    char* key_buf;
    int outer_col;
    bool finished;
    vector<ExeCondition> conditions;
    vector<int> idxcid_outsid;
    INLJ(DbManager* dbm, unique_ptr<Operator> _outer_child, Index* index, vector<Condition>& vec): Operator(dbm){
        inner_index = index;
        inner_table_id = inner_index->table_id;
        inner_table = dbm->id_table[inner_table_id];
        outer_child = move(_outer_child);
        layout = outer_child->layout;
        Schema& sc = inner_table->mng->tbmeta.schema;
        col_slot = outer_child->col_slot;
        outer_col = outer_child->layout.size();
        idxcid_outsid.assign(index->cid.size(), -1);
        for(int i=0;i<sc.col;i++){
            BoundColumn tmp = {inner_table_id, i, sc.desc[i].type, sc.desc[i].nullable};
            string print_name = string(inner_table->name) +"."+ string(sc.desc[i].name);
            layout.push_back({print_name,tmp});
            col_slot[tmp] = i + outer_col;
        }
        for (int key_pos = 0;key_pos < inner_index->cid.size();++key_pos) {
            const int inner_cid = inner_index->cid[key_pos];
            for (const Condition& condition : vec) {
                if (condition.type != ConditionType::COMPARE ||
                    condition.op != CompareOp::EQ ||
                    condition.left.is_val ||
                    condition.right.is_val) {
                    continue;
                }
                const BoundColumn* outer_column = nullptr;
                if (condition.left.col.table_id == inner_table_id &&
                    condition.left.col.cid == inner_cid) {
                    outer_column = &condition.right.col;
                }
                else if (condition.right.col.table_id == inner_table_id &&
                        condition.right.col.cid == inner_cid) {
                    outer_column = &condition.left.col;
                }
                if (outer_column == nullptr) {
                    continue;
                }
                auto slot_it = outer_child->col_slot.find(*outer_column);
                if (slot_it == outer_child->col_slot.end()) {
                    continue;
                }
                idxcid_outsid[key_pos] = slot_it->second;
                break;
            }
        }
        for(auto i:vec){
            ExeCondition tmp;
            tmp.op = i.op, tmp.pattern = i.pattern, tmp.type = i.type, tmp.values = i.values;
            ExeOperand& left = tmp.left,&right = tmp.right;
            left.is_val = i.left.is_val;
            right.is_val = i.right.is_val;
            if(left.is_val)left.val = i.left.val;
            else left.slot = col_slot.at(i.left.col);
            if(right.is_val)right.val = i.right.val;
            else right.slot = col_slot.at(i.right.col);
            conditions.push_back(tmp);
        }
        finished = 0;
        record_buf = new char[sc.recordsize];
        key_buf = new char[inner_index->mng->keylen];
        key.reserve(inner_index->col);
        inner_tuple.values.reserve(sc.col);
    }
    ~INLJ(){
        delete[] key_buf;
        delete[] record_buf;
    }
    int Open(){
        iter = {};
        if(outer_child->Open()){
            opened = 0;
            return -1;
        }
        opened = 1;
        finished = 0;
        return 0;
    }
    int Next(Tuple& output){
        if(opened == false)return -1;
        if(finished)return 1;
        Schema& sc = inner_table->mng->tbmeta.schema;
        label:
        if(inner_index->mng->Valid(iter,1)){
            ull rid = iter.rec_rid;
            inner_index->mng->Next(iter);
            inner_table->mng->LoadRecord(rid, record_buf);
            deserialization(record_buf, inner_tuple.values, sc);
        }else{
            label2:
            int res = outer_child->Next(outer_tuple);
            if(res == -1)return -1;
            if(res == 1){
                finished = 1;
                return 1;
            }
            key.clear();
            for(auto i:idxcid_outsid){
                key.push_back(outer_tuple.values[i]);
                if(holds_alternative<bool>(outer_tuple.values[i])){
                    key.clear();
                    goto label2;
                }
            }
            if(index_serialization(key, key_buf, sc, inner_index)){
                return -1;
            }
            iter = inner_index->mng->SearchRange(key_buf, key_buf, 1, 1);
            goto label;
        }
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
                    goto label;
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
        output.values = outer_tuple.values;
        output.values.insert(output.values.end(), inner_tuple.values.begin(), inner_tuple.values.end());
        return 0;
    }
    void Close() {
        outer_child->Close();
        iter = {};
        key.clear();
        outer_tuple.values.clear();
        inner_tuple.values.clear();
        finished = false;
        opened = false;
    }
};
pair<int,int> InsertExecuter(DbManager& dbm, BoundInsertRequest& bir);
pair<int,int> DeleteExecuter(DbManager& dbm, BoundDeleteRequest& bdr);
pair<int,int> UpdateExecuter(DbManager& dbm, BoundUpdateRequest& bur);