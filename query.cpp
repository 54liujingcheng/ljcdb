#include "query.h"
#include <compare>
#include <tuple>
#include <climits>
#include <cfloat>
std::optional<DATE> parse_date_yyyy_mm_dd(std::string_view str) {
    if (str.size() < 10) return std::nullopt;
    DATE d;
    int yyyy,mm,dd;
    auto res1 = std::from_chars(str.data(), str.data() + 4, yyyy);
    auto res2 = std::from_chars(str.data() + 5, str.data() + 7, mm);
    auto res3 = std::from_chars(str.data() + 8, str.data() + 10, dd);
    d = {yyyy*10000 + mm*100 + dd};
    if (res1.ec == std::errc{} && res2.ec == std::errc{} && res3.ec == std::errc{}) {
        return d;
    }
    return std::nullopt;
}
bool is_valid_date_string(std::string_view str) {
    if (str.size() != 10 || str[4] != '-' || str[7] != '-') {
        return false;
    }
    int year = 0, month = 0, day = 0;
    auto [p1, ec1] = std::from_chars(str.data(), str.data() + 4, year);
    auto [p2, ec2] = std::from_chars(str.data() + 5, str.data() + 7, month);
    auto [p3, ec3] = std::from_chars(str.data() + 8, str.data() + 10, day);
    if (ec1 != std::errc{} || p1 != str.data() + 4 ||
        ec2 != std::errc{} || p2 != str.data() + 7 ||
        ec3 != std::errc{} || p3 != str.data() + 10) {
        return false;
    }
    return is_valid_date(year, month, day);
}
BoundInsertRequest BindingInsertRequest(DbManager& dbm,UnboundInsertRequest& req){
    if(!dbm.name_table.count(req.FROM))return {0,req.rows,ERR_ELSE};
    int table_id = dbm.name_table[req.FROM]->id;
    Schema& sc = dbm.name_table[req.FROM]->mng->tbmeta.schema;
    vector<Row>& rows = req.rows;
    for(int i=0;i<rows.size();i++){
        if(rows[i].size() != sc.col)return BoundInsertRequest{table_id, req.rows, ERR_ELSE};
        for(int j=0;j<rows[i].size();j++){
            int type = sc.desc[j].type, nullable = sc.desc[j].nullable;
            if(holds_alternative<bool>(rows[i][j]) && !nullable){
                return BoundInsertRequest{table_id, req.rows, ERR_NULL};
            }
            if(holds_alternative<int>(rows[i][j]) && type != 0){
                return BoundInsertRequest{table_id, req.rows, ERR_ELSE};
            }
            if(holds_alternative<double>(rows[i][j]) && type != 1){
                return BoundInsertRequest{table_id, req.rows, ERR_ELSE};
            }
            if(holds_alternative<string>(rows[i][j]) && type != 2 && type != 3){
                return BoundInsertRequest{table_id, req.rows, ERR_ELSE};
            }
            if(holds_alternative<string>(rows[i][j]) && type == 3){
                if(!is_valid_date_string(get<string>(rows[i][j])))return BoundInsertRequest{table_id, req.rows, ERR_DATE};
                DATE date = parse_date_yyyy_mm_dd(get<string>(rows[i][j])).value();
                rows[i][j] = date;
            }
        }
    }
    return BoundInsertRequest{table_id, req.rows, 0};
}
static optional<int> BoundSelectOutputType(const BoundSelectRequest& request){
    if(request.errorcode != OK || request.col_items.size() != 1)
        return nullopt;
    const auto& [column, aggregate] = request.col_items.front();
    if(aggregate == AggregateFunc::COUNT)
        return 0;
    if(aggregate == AggregateFunc::AVG)
        return 1;
    if(!column)
        return nullopt;
    if(aggregate == AggregateFunc::SUM)
        return column->type == 0 ? 4 : (column->type == 1 ? 1 : -1);
    return column->type;
}
pair<Condition, int> ConditionBindingHelper(Table* ltb,Table* rtb, UnboundCondition& i, DbManager& dbm){
    Schema& lsc = ltb->mng->tbmeta.schema;
    Schema& rsc = rtb->mng->tbmeta.schema;
    int ltable_id = ltb->id, rtable_id = rtb->id;
    Condition tmp{};
    tmp.type = i.type, tmp.op = i.op;
    tmp.values = i.values, tmp.pattern = i.pattern;
    if(tmp.type == ConditionType::COMPARE){
        if(!i.left.is_val){
            if(!ltb->name_cid.count(i.left.col_name.second))return {tmp, ERR_ELSE};
            int cid = ltb->name_cid[i.left.col_name.second];
            BoundColumn ttmp = {ltable_id, cid, lsc.desc[cid].type, lsc.desc[cid].nullable};
            tmp.left = {i.left.is_val, ttmp, i.left.val};
        }else{
            BoundColumn ttmp = {0,0,0,0};
            tmp.left = {i.left.is_val, ttmp, i.left.val};
        }
        if(!i.right.is_val){
            if(!rtb->name_cid.count(i.right.col_name.second))return {tmp, ERR_ELSE};
            int cid = rtb->name_cid[i.right.col_name.second];
            BoundColumn ttmp = {rtable_id, cid, rsc.desc[cid].type, rsc.desc[cid].nullable};
            tmp.right = {i.right.is_val, ttmp, i.right.val};
        }else{
            BoundColumn ttmp = {0,0,0,0};
            tmp.right = {i.right.is_val, ttmp, i.right.val};
        }
        if(tmp.left.is_val && !tmp.right.is_val){
            if(tmp.right.col.type == 3){
                if(holds_alternative<string>(tmp.left.val) && is_valid_date_string(get<string>(tmp.left.val))){
                    DATE d = parse_date_yyyy_mm_dd(get<string>(tmp.left.val)).value();
                    tmp.left.val = d;
                }
                else return {tmp, ERR_DATE};
            }
            else if(holds_alternative<int>(tmp.left.val) && tmp.right.col.type != 0)return {tmp, ERR_ELSE};
            else if(holds_alternative<string>(tmp.left.val) && tmp.right.col.type != 2)return {tmp, ERR_ELSE};
            else if(holds_alternative<double>(tmp.left.val) && tmp.right.col.type != 1)return {tmp, ERR_ELSE};
        }
        else if(!tmp.left.is_val && tmp.right.is_val){
            if(tmp.left.col.type == 3){
                if(holds_alternative<string>(tmp.right.val) && is_valid_date_string(get<string>(tmp.right.val))){
                    DATE d = parse_date_yyyy_mm_dd(get<string>(tmp.right.val)).value();
                    tmp.right.val = d;
                }
                else return {tmp, ERR_DATE};
            }
            else if(holds_alternative<int>(tmp.right.val) && tmp.left.col.type != 0)return {tmp, ERR_ELSE};
            else if(holds_alternative<string>(tmp.right.val) && tmp.left.col.type != 2)return {tmp, ERR_ELSE};
            else if(holds_alternative<double>(tmp.right.val) && tmp.left.col.type != 1)return {tmp, ERR_ELSE};
        }
        else if(!tmp.left.is_val && !tmp.right.is_val){
            if(tmp.left.col.type != tmp.right.col.type)return {tmp, ERR_ELSE};
        }
        else return {tmp, ERR_ELSE};
    }
    if(tmp.type == ConditionType::IN_LIST){
        if(i.left.is_val)return {tmp, ERR_ELSE};
        if(!ltb->name_cid.count(i.left.col_name.second))return {tmp, ERR_ELSE};
        int cid = ltb->name_cid[i.left.col_name.second];
        BoundColumn ttmp = {ltable_id, cid, lsc.desc[cid].type, lsc.desc[cid].nullable};
        tmp.left = {i.left.is_val, ttmp, i.left.val};
        if(tmp.left.col.type == 3){
            for(auto& i:tmp.values){
                if(holds_alternative<string>(i) && is_valid_date_string(get<string>(i)))
                    i = parse_date_yyyy_mm_dd(get<string>(i)).value();
                else return {tmp, ERR_DATE};
            }
        }
        else{
            for(auto& i:tmp.values){
                if(holds_alternative<int>(i) && tmp.left.col.type != 0 || holds_alternative<double>(i) && tmp.left.col.type != 1 || holds_alternative<string>(i) && tmp.left.col.type != 2 )
                    return {tmp, ERR_ELSE};
            }
        }
    }
    if(tmp.type == ConditionType::IS_NOT_NULL || tmp.type == ConditionType::IS_NULL ||tmp.type == ConditionType::LIKE){
        if(i.left.is_val)return {tmp, ERR_ELSE};
        if(!ltb->name_cid.count(i.left.col_name.second))return {tmp, ERR_ELSE};
        int cid = ltb->name_cid[i.left.col_name.second];
        BoundColumn ttmp = {ltable_id, cid, lsc.desc[cid].type, lsc.desc[cid].nullable};
        tmp.left = {i.left.is_val, ttmp, i.left.val};
        if(tmp.type == ConditionType::LIKE && tmp.left.col.type != 2)return {tmp, ERR_ELSE};
    }
    if(tmp.type == ConditionType::SUB_COMPARE || tmp.type == ConditionType::SUB_IN){
        if(i.left.is_val || !i.sub)return {tmp, ERR_ELSE};
        if(!ltb->name_cid.count(i.left.col_name.second))return {tmp, ERR_ELSE};
        int cid = ltb->name_cid[i.left.col_name.second];
        BoundColumn ttmp = {ltable_id, cid, lsc.desc[cid].type, lsc.desc[cid].nullable};
        tmp.left = {i.left.is_val, ttmp, i.left.val};
        BoundSelectRequest sub = BindingSelectRequest(dbm, *i.sub);
        optional<int> output_type = BoundSelectOutputType(sub);
        if(!output_type || *output_type != tmp.left.col.type)return {tmp, ERR_ELSE};
        tmp.sub = make_shared<BoundSelectRequest>(move(sub));
    }
    return {tmp,0};
}
BoundDeleteRequest BindingDeleteRequest(DbManager& dbm,UnboundDeleteRequest& req){
    vector<Condition> conditions;
    if(!dbm.name_table.count(req.FROM))return BoundDeleteRequest{0, conditions, ERR_ELSE};
    Table* tb = dbm.name_table[req.FROM];
    Schema& sc = tb->mng->tbmeta.schema;
    int table_id = tb->id;
    for(auto& i:req.conditions){
        if(!i.left.is_val && !i.left.col_name.first.empty() && i.left.col_name.first != req.FROM )return {table_id, conditions, ERR_ELSE};
        if(i.type == ConditionType::COMPARE){
            if(!i.right.is_val && !i.right.col_name.first.empty() && i.right.col_name.first != req.FROM)return {table_id, conditions, ERR_ELSE};
        }
        pair<Condition,int> tmp = ConditionBindingHelper(tb,tb, i, dbm);
        if(tmp.second==0)conditions.push_back(tmp.first);
        else return {table_id, conditions, tmp.second};
    }
    return {table_id, conditions, 0};
}
BoundUpdateRequest BindingUpdateRequest(DbManager& dbm,UnboundUpdateRequest& req){
    vector<Condition> conditions;
    vector<BoundColumn> col;
    if(!dbm.name_table.count(req.FROM))return BoundUpdateRequest{0, col, req.values, conditions, ERR_ELSE};
    Table* tb = dbm.name_table[req.FROM];
    Schema& sc = tb->mng->tbmeta.schema;
    int table_id = tb->id;
    if(req.values.size() != req.col_names.size())return BoundUpdateRequest{0, col, req.values, conditions, ERR_ELSE};
    for(auto& i:req.col_names){
        if((!i.first.empty() && i.first!=req.FROM) || tb->name_cid.count(i.second)==0){
            return BoundUpdateRequest{0, col, req.values, conditions, ERR_ELSE};
        }
        BoundColumn tmp;
        int cid = tb->name_cid[i.second];
        tmp.nullable = sc.desc[cid].nullable, tmp.type = sc.desc[cid].type, tmp.cid = cid, tmp.table_id = table_id;
        col.push_back(tmp);
    }
    for(int i=0;i<req.values.size();i++){
        if(holds_alternative<bool>(req.values[i])){
            if(col[i].nullable)continue;
            else return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_NULL};
        }
        if(col[i].type == 3 ){
            if(holds_alternative<string>(req.values[i]) && is_valid_date_string(get<string>(req.values[i]))){
                req.values[i] = parse_date_yyyy_mm_dd(get<string>(req.values[i])).value();
            }
            else return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_DATE};
        }
        if(holds_alternative<int>(req.values[i]) && col[i].type!=0 || holds_alternative<double>(req.values[i]) && col[i].type!=1 || holds_alternative<string>(req.values[i]) && col[i].type!=2)return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_ELSE};
    }
    for(auto& i:req.conditions){
        if(!i.left.is_val && !i.left.col_name.first.empty() && i.left.col_name.first != req.FROM )return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_ELSE};
        if(i.type == ConditionType::COMPARE){
            if(!i.right.is_val && !i.right.col_name.first.empty() && i.right.col_name.first != req.FROM)return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_ELSE};
        }
        pair<Condition,int> tmp = ConditionBindingHelper(tb,tb, i, dbm);
        if(tmp.second==0)conditions.push_back(tmp.first);
        else return BoundUpdateRequest{tb->id, col, req.values, conditions, tmp.second};
    }
    return BoundUpdateRequest{tb->id, col, req.values, conditions, 0};
}
BoundSelectRequest BindingSelectRequest(DbManager& dbm,UnboundSelectRequest& req){
    vector<int> table_ids;
    vector<pair<optional<BoundColumn>,AggregateFunc>> col_items;
    vector<pair<BoundColumn,OrderBy>> ORDERBY;
    vector<BoundColumn> GROUPBY;
    vector<Condition> conditions;
    bool star = 0;
    bool withagg = false;
    if(req.FROM.empty() || req.col_items.empty())
        return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
    unordered_set<string> bound_tables;
    for(auto& i:req.FROM){
        if(!bound_tables.insert(i).second || dbm.name_table.count(i)==0)
            return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
        table_ids.push_back(dbm.name_table[i]->id);
    }
    if(req.col_items[0].first.first.empty() && req.col_items[0].first.second == "*"){
        star = 1;
        for(auto i:table_ids){
            Schema& sc = dbm.id_table[i]->mng->tbmeta.schema;
            for(int j=0;j<sc.col;j++){ 
                BoundColumn tmp = {i, j, sc.desc[j].type, sc.desc[j].nullable};
                col_items.push_back({tmp,AggregateFunc::NONE});
            }
        }
    }
    else
    for(auto& i:req.col_items){
        string& table_name = i.first.first, &col_name = i.first.second;
        AggregateFunc agg = i.second;
        if(agg!=AggregateFunc::NONE)withagg = true;
        if(table_name.empty()&&col_name.empty()){
            if(agg==AggregateFunc::COUNT)col_items.push_back({std::nullopt, agg});
            else return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
        }
        else if(table_name.empty()&&!col_name.empty()){
            int cnt=0,idx=0,cid=0;
            for(auto j:table_ids){
                Table* tb = dbm.id_table[j];
                if(tb->name_cid.count(col_name))idx=j,cid=tb->name_cid[col_name],cnt++;
            }
            if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            Schema& sc = dbm.id_table[idx]->mng->tbmeta.schema;
            BoundColumn tmp={idx,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            col_items.push_back({tmp,agg});
        }
        else if(!table_name.empty()&&!col_name.empty()){
            if(!dbm.name_table.count(table_name) )return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            int flag=0;
            for(int i=0;i<table_ids.size();i++){
                if(dbm.id_table[table_ids[i]]->name == table_name){
                    flag = 1;
                    break;
                }
            }
            if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            Table* tb = dbm.name_table[table_name];
            if(!tb->name_cid.count(col_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            int cid = tb->name_cid[col_name];
            Schema& sc = tb->mng->tbmeta.schema;
            BoundColumn tmp={tb->id,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            col_items.push_back({tmp,agg});
        }
        else return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
        if((agg == AggregateFunc::AVG || agg == AggregateFunc::SUM) && col_items.back().first->type != 0 && col_items.back().first->type != 1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
    }
    for(auto& i:req.ORDERBY){
        string& table_name = i.first.first, &col_name = i.first.second;
        OrderBy od = i.second;
        if(table_name.empty()&&col_name.empty()){
            return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
        }
        else if(table_name.empty()&&!col_name.empty()){
            int cnt=0,idx=0,cid=0;
            for(auto j:table_ids){
                Table* tb = dbm.id_table[j];
                if(tb->name_cid.count(col_name))idx=j,cid=tb->name_cid[col_name],cnt++;
            }
            if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            Schema& sc = dbm.id_table[idx]->mng->tbmeta.schema;
            BoundColumn tmp={idx,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            ORDERBY.push_back({tmp,od});
        }
        else if(!table_name.empty()&&!col_name.empty()){
            if(!dbm.name_table.count(table_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            int flag=0;
            for(int i=0;i<table_ids.size();i++){
                if(dbm.id_table[table_ids[i]]->name == table_name){
                    flag = 1;
                    break;
                }
            }
            if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            Table* tb = dbm.name_table[table_name];
            if(!tb->name_cid.count(col_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            int cid = tb->name_cid[col_name];
            Schema& sc = tb->mng->tbmeta.schema;
            BoundColumn tmp={tb->id,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            ORDERBY.push_back({tmp,od});
        }
        else return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
    }
    for(auto& i:req.GROUPBY){
        string &table_name = i.first, &col_name = i.second;
        if(table_name.empty()&&col_name.empty()){
            return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
        }
        else if(table_name.empty()&&!col_name.empty()){
            int cnt=0,idx=0,cid=0;
            for(auto j:table_ids){
                Table* tb = dbm.id_table[j];
                if(tb->name_cid.count(col_name))idx=j,cid=tb->name_cid[col_name],cnt++;
            }
            if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            Schema& sc = dbm.id_table[idx]->mng->tbmeta.schema;
            BoundColumn tmp={idx,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            GROUPBY.push_back(tmp);
        }
        else if(!table_name.empty()&&!col_name.empty()){
            if(!dbm.name_table.count(table_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            int flag=0;
            for(int i=0;i<table_ids.size();i++){
                if(dbm.id_table[table_ids[i]]->name == table_name){
                    flag = 1;
                    break;
                }
            }
            if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            Table* tb = dbm.name_table[table_name];
            if(!tb->name_cid.count(col_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
            int cid = tb->name_cid[col_name];
            Schema& sc = tb->mng->tbmeta.schema;
            BoundColumn tmp={tb->id,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            GROUPBY.push_back(tmp);
        }
        else return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
    }
    for(auto& i:req.conditions){
        if(i.type == ConditionType::COMPARE){
            if(!i.left.is_val&&!i.right.is_val){
                string &l_tname = i.left.col_name.first, &l_cname = i.left.col_name.second;
                string &r_tname = i.right.col_name.first, &r_cname = i.right.col_name.second;
                Table* l, *r;
                int lcid, rcid;
                if(l_tname.empty()){
                    int cnt=0, idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(l_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    l = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(l_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    l = dbm.name_table[l_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(l->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                }
                if(r_tname.empty()){
                    int cnt=0,idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(r_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    r = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(r_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    r = dbm.name_table[r_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(r->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                }
                if(!l->name_cid.count(l_cname) || !r->name_cid.count(r_cname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                pair<Condition,int> tmp = ConditionBindingHelper(l, r, i, dbm);
                if(tmp.second==0)conditions.push_back(tmp.first);
                else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, tmp.second, star};
            }
            else if(!i.left.is_val){
                string &l_tname = i.left.col_name.first, &l_cname = i.left.col_name.second;
                Table* l;
                int lcid;
                if(l_tname.empty()){
                    int cnt=0, idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(l_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    l = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(l_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    l = dbm.name_table[l_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(l->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                }
                if(!l->name_cid.count(l_cname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                lcid = l->name_cid.count(l_cname);
                pair<Condition,int> tmp = ConditionBindingHelper(l, l, i, dbm);
                if(tmp.second==0)conditions.push_back(tmp.first);
                else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, tmp.second, star};
            }
            else if(i.left.is_val&&!i.right.is_val){
                string &r_tname = i.right.col_name.first, &r_cname = i.right.col_name.second;
                Table* r;
                int rcid;
                if(r_tname.empty()){
                    int cnt=0, idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(r_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    r = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(r_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    r = dbm.name_table[r_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(r->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                }
                if(!r->name_cid.count(r_cname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                rcid = r->name_cid.count(r_cname);
                pair<Condition,int> tmp = ConditionBindingHelper(r, r, i, dbm);
                if(tmp.second==0)conditions.push_back(tmp.first);
                else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, tmp.second, star};
            }
            else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
        }
        else{
            if(!i.left.is_val){
                string &l_tname = i.left.col_name.first, &l_cname = i.left.col_name.second;
                Table* l;
                int lcid;
                if(l_tname.empty()){
                    int cnt=0, idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(l_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    l = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(l_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                    l = dbm.name_table[l_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(l->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                }
                if(!l->name_cid.count(l_cname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
                lcid = l->name_cid.count(l_cname);
                pair<Condition,int> tmp = ConditionBindingHelper(l, l, i, dbm);
                if(tmp.second==0)conditions.push_back(tmp.first);
                else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, tmp.second, star};
            }
            else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE, star};
        }
    }
    if(withagg || !GROUPBY.empty()){
        unordered_set<BoundColumn> determined(GROUPBY.begin(), GROUPBY.end());
        bool changed = true;
        while(changed){
            changed = false;
            for(const Condition& condition : conditions){
                if(condition.type != ConditionType::COMPARE || condition.op != CompareOp::EQ ||
                   condition.left.is_val || condition.right.is_val)
                    continue;
                if(determined.count(condition.left.col) && determined.insert(condition.right.col).second)
                    changed = true;
                if(determined.count(condition.right.col) && determined.insert(condition.left.col).second)
                    changed = true;
            }
            for(int table_id : table_ids){
                auto table_it = dbm.id_table.find(table_id);
                if(table_it == dbm.id_table.end() || table_it->second->pkid == -1)
                    continue;
                auto index_it = dbm.id_idx.find(table_it->second->pkid);
                if(index_it == dbm.id_idx.end())continue;
                bool primary_key_determined = true;
                for(int cid : index_it->second->cid){
                    if(!determined.count(BoundColumn{table_id, cid})){
                        primary_key_determined = false;
                        break;
                    }
                }
                if(!primary_key_determined)continue;
                Schema& schema = table_it->second->mng->tbmeta.schema;
                for(int cid = 0; cid < schema.col; ++cid){
                    if(determined.insert(BoundColumn{table_id, cid,
                       schema.desc[cid].type, schema.desc[cid].nullable}).second)
                        changed = true;
                }
            }
        }
        for(const auto& item : col_items){
            if(item.second == AggregateFunc::NONE &&
               (!item.first || !determined.count(*item.first)))
                return {table_ids, col_items, ORDERBY, GROUPBY, conditions,
                        req.LIMIT, req.OFFSET, ERR_ELSE, star};
        }
    }
    return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, 0, star};
}
inline bool like(std::string_view text, std::string_view pattern) {
    size_t t_idx = 0, p_idx = 0;
    size_t star_idx = std::string_view::npos;
    size_t match_idx = 0;
    size_t t_len = text.length(), p_len = pattern.length();
    while (t_idx < t_len) {
        if (p_idx <  p_len&& (pattern[p_idx] == '_' || pattern[p_idx] == text[t_idx])) {
            t_idx++;
            p_idx++;
        }
        else if (p_idx < p_len && pattern[p_idx] == '%') {
            star_idx = p_idx;
            match_idx = t_idx;
            p_idx++;
        }
        else if (star_idx != std::string_view::npos) {
            p_idx = star_idx + 1;
            match_idx++;
            t_idx = match_idx;
        }
        else {
            return false;
        }
    }
    while (p_idx < pattern.length() && pattern[p_idx] == '%') {
        p_idx++;
    }
    return p_idx == pattern.length();
}
unique_ptr<Operator> ScanMother(DbManager* dbm, vector<Condition>& conditions, int table_id);
static bool CanUseINLJ(Index* idx,int inner_table_id,const Operator& outer,const vector<Condition>& join_conditions) {
    for (int inner_cid : idx->cid) {
        bool found = false;
        for (const Condition& condition : join_conditions) {
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
            if (outer_column != nullptr &&
                outer.col_slot.find(*outer_column) != outer.col_slot.end()) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}
unique_ptr<Operator> Planner(DbManager* dbm, BoundSelectRequest& bsr){//记得有聚合的时候不用Project
    if(bsr.errorcode!=OK || bsr.table_ids.empty())return nullptr;
    unique_ptr<Operator> root = nullptr;
    bool has_agg=0;
    for(auto i:bsr.col_items){
        if(i.second!=AggregateFunc::NONE){
            has_agg=1;
            break;
        }
    }
    for(auto& i:bsr.conditions){
        if(i.type == ConditionType::SUB_COMPARE){
            if(!i.sub)return nullptr;
            unique_ptr<Operator> root = Planner(dbm, *(i.sub));
            Tuple input;
            if(root == nullptr || root->Open())return nullptr;
            int res = root->Next(input);
            if(res == -1 || res == 1){
                root->Close();
                return nullptr;
            }
            if(input.values.size() != 1){
                root->Close();
                return nullptr;
            }
            Tuple extra;
            res = root->Next(extra);
            if(res != 1){
                root->Close();
                return nullptr;
            }
            i.type = ConditionType::COMPARE;
            i.right.is_val = true;
            i.right.val = input.values[0];
            root->Close();
        }
        else if(i.type == ConditionType::SUB_IN){
            if(!i.sub)return nullptr;
            unique_ptr<Operator> root = Planner(dbm, *(i.sub));
            Tuple input;
            if(root == nullptr || root->Open())return nullptr;
            i.values.clear();
            while(1){
                int res = root->Next(input);
                if(res == -1){
                    root->Close();
                    return nullptr;
                }
                if(res == 1){
                    root->Close();
                    break;
                }
                if(input.values.size() != 1){
                    root->Close();
                    return nullptr;
                }
                i.values.push_back(input.values[0]);
            }
            i.type = ConditionType::IN_LIST;
            root->Close();
        }
    }
    if(bsr.table_ids.size() == 1){
        root = ScanMother(dbm, bsr.conditions, bsr.table_ids[0]);
        if(bsr.conditions.size()>0)root = make_unique<Filter>(dbm, move(root), bsr.conditions);
        bool flag=0;
        for(auto i:bsr.ORDERBY){
            bool flag2=0;
            for(auto j:bsr.col_items){
                if(j.first && i.first == j.first.value()){
                    flag2 = 1;
                    break;
                }
            }
            if(!flag2){
                flag=1;
                break;
            }
        }
        if(!flag){
            if(!bsr.GROUPBY.empty() || has_agg)root = make_unique<Aggregate>(dbm, move(root), bsr);
            if(!bsr.star && bsr.GROUPBY.empty() && !has_agg)root = make_unique<Project>(dbm, move(root), bsr);
            if(!bsr.ORDERBY.empty())root = make_unique<Sort>(dbm, move(root), bsr, bsr.LIMIT, bsr.OFFSET);
            if(bsr.LIMIT!=-1)root = make_unique<Limit>(dbm, move(root), bsr);
        }
        else{
            if(!bsr.GROUPBY.empty() || has_agg)root = make_unique<Aggregate>(dbm, move(root), bsr);
            if(!bsr.ORDERBY.empty())root = make_unique<Sort>(dbm, move(root), bsr, bsr.LIMIT, bsr.OFFSET);
            if(!bsr.star && bsr.GROUPBY.empty() && !has_agg)root = make_unique<Project>(dbm, move(root), bsr);
            if(bsr.LIMIT!=-1)root = make_unique<Limit>(dbm, move(root), bsr);
        }
    }
    else{
        unordered_set<int> tids;
        root = ScanMother(dbm, bsr.conditions, bsr.table_ids[bsr.table_ids.size()-1]);
        int i = bsr.table_ids[bsr.table_ids.size()-1];
        vector<Condition> tmp;
        for(auto& j:bsr.conditions){
            if(j.type == ConditionType::COMPARE){
                if((j.left.is_val || j.left.col.table_id == i) && (j.right.is_val || j.right.col.table_id == i)){
                    tmp.push_back(j);
                }
            }
            else{
                if(j.left.col.table_id == i)tmp.push_back(j);
            }
        }
        if(tmp.size()>0)root = make_unique<Filter>(dbm, move(root), tmp);
        tids.insert(i);
        for (int ii = bsr.table_ids.size()-2;ii >= 0;--ii) {
            int inner_table_id = bsr.table_ids[ii];
            vector<Condition> local_conditions;
            vector<Condition> join_conditions;
            for (auto& condition : bsr.conditions) {
                if (condition.type == ConditionType::COMPARE) {
                    if ((condition.left.is_val || condition.left.col.table_id == inner_table_id) &&
                        (condition.right.is_val || condition.right.col.table_id == inner_table_id)) {
                        local_conditions.push_back(condition);
                    }
                    else if (!condition.left.is_val &&!condition.right.is_val &&((condition.left.col.table_id == inner_table_id &&
                            tids.count(condition.right.col.table_id)) || (condition.right.col.table_id == inner_table_id && tids.count(condition.left.col.table_id)))) {
                        join_conditions.push_back(condition);
                    }
                }
                else {
                    if (!condition.left.is_val && condition.left.col.table_id == inner_table_id) {
                        local_conditions.push_back(condition);
                    }
                }
            }
            Table* inner_table = dbm->id_table.at(inner_table_id);
            Index* best_index = nullptr;
            int best_score = -1;
            auto consider_index = [&](Index* idx) {
                if (!CanUseINLJ(idx, inner_table_id, *root, join_conditions))
                    return;
                int score = idx->col;
                if (idx->uni)
                    score += 1000;
                if (score > best_score) {
                    best_score = score;
                    best_index = idx;
                }
            };
            if (inner_table->pkid != -1) {
                consider_index(dbm->id_idx.at(inner_table->pkid));
            }
            auto index_it = dbm->id_normidx.find(inner_table_id);
            if (index_it != dbm->id_normidx.end()) {
                for (Index* idx : index_it->second) {
                    consider_index(idx);
                }
            }
            if (best_index != nullptr) {
                vector<Condition> inlj_conditions = join_conditions;
                inlj_conditions.insert(inlj_conditions.end(),local_conditions.begin(),local_conditions.end());
                root = make_unique<INLJ>(dbm, move(root), best_index, inlj_conditions);
            }
            else {
                unique_ptr<Operator> inner_scan = ScanMother(dbm, bsr.conditions, inner_table_id);
                if (!local_conditions.empty()) {
                    inner_scan = make_unique<Filter>(dbm, move(inner_scan), local_conditions);
                }
                root = make_unique<BNLJ>(dbm, move(inner_scan), move(root), join_conditions);
            }
            tids.insert(inner_table_id);
        }
        bool flag=0;
        for(auto i:bsr.ORDERBY){
            bool flag2=0;
            for(auto j:bsr.col_items){
                if(j.first && i.first == j.first.value()){
                    flag2 = 1;
                    break;
                }
            }
            if(!flag2){
                flag=1;
                break;
            }
        }
        if(!flag){
            if(!bsr.GROUPBY.empty() || has_agg)root = make_unique<Aggregate>(dbm, move(root), bsr);
            if(!bsr.star && bsr.GROUPBY.empty() && !has_agg)root = make_unique<Project>(dbm, move(root), bsr);
            if(!bsr.ORDERBY.empty())root = make_unique<Sort>(dbm, move(root), bsr, bsr.LIMIT, bsr.OFFSET);
            if(bsr.LIMIT!=-1)root = make_unique<Limit>(dbm, move(root), bsr);
        }
        else{
            if(!bsr.GROUPBY.empty() || has_agg)root = make_unique<Aggregate>(dbm, move(root), bsr);
            if(!bsr.ORDERBY.empty())root = make_unique<Sort>(dbm, move(root), bsr, bsr.LIMIT, bsr.OFFSET);
            if(!bsr.star && bsr.GROUPBY.empty() && !has_agg)root = make_unique<Project>(dbm, move(root), bsr);
            if(bsr.LIMIT!=-1)root = make_unique<Limit>(dbm, move(root), bsr);
        }
    }
    return root;
}
struct LeftBound{
    Value val;
    bool inclusive;
    std::partial_ordering operator<=>(const LeftBound& other) const {
        if (auto cmp = val <=> other.val; cmp != 0) {
            return cmp;
        }
        return other.inclusive <=> inclusive;
    }

    bool operator==(const LeftBound& other) const {
        return val == other.val && inclusive== other.inclusive;
    }
};
struct RightBound{
    Value val;
    bool inclusive;
    auto operator<=>(const RightBound&) const = default;
};
struct ScanCandidate{
    Row l;
    Row r;
    bool linc;
    bool rinc;
    int idx;
    Index* index;
    int score;
};
unique_ptr<Operator> ScanMother(DbManager* dbm, vector<Condition>& conditions, int table_id){
    Table* tb = dbm->id_table[table_id];
    vector<Index*> index = dbm->id_normidx[table_id];
    if(tb->pkid!=-1)index.push_back(dbm->id_idx[tb->pkid]);
    unordered_map<int, LeftBound> cid_leftbound;
    unordered_map<int, RightBound> cid_rightbound;
    Schema& sc = tb->mng->tbmeta.schema;
    optional<ScanCandidate> candidate = nullopt;
    for(auto j:conditions){
        int flag = 0;
        if(j.type == ConditionType::COMPARE){
            if(j.left.is_val&&!j.right.is_val&&j.right.col.table_id == table_id){
                swap(j.left,j.right);
                if(j.op == CompareOp::GE)j.op = CompareOp::LE;
                else if(j.op == CompareOp::GT)j.op = CompareOp::LT;
                else if(j.op == CompareOp::LE)j.op = CompareOp::GE;
                else if(j.op == CompareOp::LT)j.op = CompareOp::GT;
                flag = 1;
            }
            if(!j.left.is_val&&j.right.is_val&&j.left.col.table_id == table_id){
                if(j.op==CompareOp::EQ){
                    int cid = j.left.col.cid;
                    bool inc = true;
                    Value& val = j.right.val;
                    cid_leftbound[cid] = (cid_leftbound.count(cid)?max(cid_leftbound[cid], LeftBound{val,inc}):LeftBound{val,inc});
                    cid_rightbound[cid] = (cid_rightbound.count(cid)?min(cid_rightbound[cid], RightBound{val,inc}):RightBound{val,inc});
                }else if(j.op == CompareOp::GE){
                    int cid = j.left.col.cid;
                    bool inc = true;
                    Value& val = j.right.val;
                    cid_leftbound[cid] = (cid_leftbound.count(cid)?max(cid_leftbound[cid], LeftBound{val,inc}):LeftBound{val,inc});
                }else if(j.op == CompareOp::GT){
                    int cid = j.left.col.cid;
                    bool inc = false;
                    Value& val = j.right.val;
                    cid_leftbound[cid] = (cid_leftbound.count(cid)?max(cid_leftbound[cid], LeftBound{val,inc}):LeftBound{val,inc});
                }else if(j.op == CompareOp::LE){
                    int cid = j.left.col.cid;
                    bool inc = true;
                    Value& val = j.right.val;
                    cid_rightbound[cid] = (cid_rightbound.count(cid)?min(cid_rightbound[cid], RightBound{val,inc}):RightBound{val,inc});
                }else if(j.op == CompareOp::LT){
                    int cid = j.left.col.cid;
                    bool inc = false;
                    Value& val = j.right.val;
                    cid_rightbound[cid] = (cid_rightbound.count(cid)?min(cid_rightbound[cid], RightBound{val,inc}):RightBound{val,inc});
                }
            }
        }
        if(flag){
            swap(j.left,j.right);
            if(j.op == CompareOp::GE)j.op = CompareOp::LE;
            else if(j.op == CompareOp::GT)j.op = CompareOp::LT;
            else if(j.op == CompareOp::LE)j.op = CompareOp::GE;
            else if(j.op == CompareOp::LT)j.op = CompareOp::GT;
        }
    }
    for(auto i : index){
        int idx=-1, flag=0;
        for(int j=0;j<i->col;j++){
            if(cid_leftbound.count(i->cid[j])&&cid_rightbound.count(i->cid[j])){
                LeftBound& l = cid_leftbound[i->cid[j]];
                RightBound& r = cid_rightbound[i->cid[j]];
                if(l.inclusive == r.inclusive && l.inclusive == 1 && l.val == r.val){
                    idx++;
                    continue;
                }
            }
            if(cid_leftbound.count(i->cid[j]) || cid_rightbound.count(i->cid[j])){
                idx++;
                flag=1;
            }
            break;
        }
        if(idx == -1)continue;
        else{
            Row l,r;
            for(int j=0;j<idx;j++){
                l.push_back(cid_leftbound[i->cid[j]].val),r.push_back(cid_rightbound[i->cid[j]].val);
            }
            bool linc=1,rinc=1;
            if(!cid_leftbound.count(i->cid[idx])){
                if(sc.desc[i->cid[idx]].nullable)l.push_back(false);
                else if(sc.desc[i->cid[idx]].type == 0)l.push_back(INT_MIN);
                else if(sc.desc[i->cid[idx]].type == 1)l.push_back(-DBL_MAX);
                else if(sc.desc[i->cid[idx]].type == 2)l.push_back("");
                else if(sc.desc[i->cid[idx]].type == 3)l.push_back(DATE{INT_MIN});
            }
            else l.push_back(cid_leftbound[i->cid[idx]].val),linc=cid_leftbound[i->cid[idx]].inclusive;
            if(!cid_rightbound.count(i->cid[idx])){
                if(sc.desc[i->cid[idx]].type == 0)r.push_back(INT_MAX);
                else if(sc.desc[i->cid[idx]].type == 1)r.push_back(DBL_MAX);
                else if(sc.desc[i->cid[idx]].type == 2){
                    std::string max_str(sc.desc[i->cid[idx]].len, static_cast<unsigned char>(0xFF));
                    r.push_back(max_str);
                }
                else if(sc.desc[i->cid[idx]].type == 3)r.push_back(DATE{INT_MAX});
            }
            else r.push_back(cid_rightbound[i->cid[idx]].val),rinc=cid_rightbound[i->cid[idx]].inclusive;
            for(int j=idx+1;j<i->col;j++){
                if(linc){
                    if(sc.desc[i->cid[j]].nullable)l.push_back(false);
                    else if(sc.desc[i->cid[j]].type == 0)l.push_back(INT_MIN);
                    else if(sc.desc[i->cid[j]].type == 1)l.push_back(-DBL_MAX);
                    else if(sc.desc[i->cid[j]].type == 2)l.push_back("");
                    else if(sc.desc[i->cid[j]].type == 3)l.push_back(DATE{INT_MIN});
                }
                else{
                    if(sc.desc[i->cid[j]].type == 0)l.push_back(INT_MAX);
                    else if(sc.desc[i->cid[j]].type == 1)l.push_back(DBL_MAX);
                    else if(sc.desc[i->cid[j]].type == 2){
                        std::string max_str(sc.desc[i->cid[j]].len, static_cast<char>(0xFF));
                        l.push_back(max_str);
                    }
                    else if(sc.desc[i->cid[j]].type == 3){
                        l.push_back(DATE{INT_MAX});
                    }
                }
                if(rinc){
                    if(sc.desc[i->cid[j]].type == 0)r.push_back(INT_MAX);
                    else if(sc.desc[i->cid[j]].type == 1)r.push_back(DBL_MAX);
                    else if(sc.desc[i->cid[j]].type == 2){
                        std::string max_str(sc.desc[i->cid[j]].len, static_cast<char>(0xFF));
                        r.push_back(max_str);
                    }
                    else if(sc.desc[i->cid[j]].type == 3){
                        r.push_back(DATE{INT_MAX});
                    }
                }
                else{
                    if(sc.desc[i->cid[j]].nullable)r.push_back(false);
                    else if(sc.desc[i->cid[j]].type == 0)r.push_back(INT_MIN);
                    else if(sc.desc[i->cid[j]].type == 1)r.push_back(-DBL_MAX);
                    else if(sc.desc[i->cid[j]].type == 2)r.push_back("");
                    else if(sc.desc[i->cid[j]].type == 3)r.push_back(DATE{INT_MIN});
                }
            }
            int score = 0;
            if(i->uni == 1)score++;
            if(!flag) score+=idx*2;
            else score+=idx*2-1;
            if(cid_leftbound.count(i->cid[idx]) && cid_rightbound.count(i->cid[idx]))score+=2;
            else score++;
            if(candidate==nullopt||candidate.value().score < score)candidate = ScanCandidate{l, r, linc, rinc, idx, i, score};
        }
    }
    if(candidate == nullopt)return make_unique<SeqScan>(dbm, table_id);
    return make_unique<IndexScan>(dbm, candidate.value().index, candidate.value().l, candidate.value().r, candidate.value().linc, candidate.value().rinc);
}
int tuple_deserialization(vector<char>& src, Row& dest, vector<SlotType>& slots, int row_size, int offset){
    int nullen = slots[0].offset, col = slots.size(), j = slots[0].offset;
    char* row = src.data() + offset;
    dest.clear();
    for(int i=0;i<col;i++){
        SlotType& desc = slots[i];
        if(row[(i>>3)] & (1u<<(i&7))){
            bool tmp=false;
            dest.push_back(tmp);
        }
        else{
            if(desc.type == 0){
                int tmp;
                memcpy(&tmp, row+j, desc.len);
                dest.push_back(tmp);
            }else if(desc.type == 1){
                double tmp;
                memcpy(&tmp, row+j, desc.len);
                dest.push_back(tmp);
            }else if(desc.type == 2){
                size_t len = strnlen(row + j, desc.len);
                string tmp(row + j, len);
                dest.push_back(tmp);
            }else if(desc.type == 3){
                DATE tmp;
                memcpy(&tmp, row+j, desc.len);
                dest.push_back(move(tmp));
            }
            else if(desc.type == 4){
                long long tmp;
                memcpy(&tmp, row+j, desc.len);
                dest.push_back(tmp);
            }
        }
        j += desc.len;
    }
    return 0;
}
int tuple_serialization(Row& src, vector<char>& dest, vector<SlotType>& slots, int row_size, int offset){
    memset(dest.data()+offset,0,row_size);
    int nullen = slots[0].offset, col = slots.size(), j = slots[0].offset;
    char* row = dest.data()+offset;
    for(int i=0;i<col;i++){
        SlotType& desc = slots[i];
        if(holds_alternative<bool>(src[i])){
            row[(i >> 3)] |= 1u << (i&7);
        }
        else{
            row[(i >> 3)] &= ~(1u << (i&7));
            if(desc.type == 0 && holds_alternative<int>(src[i])){
                memcpy(row+j,&(get<int>(src[i])),desc.len);
            }else if(desc.type == 1 && holds_alternative<double>(src[i])){
                memcpy(row+j,&(get<double>(src[i])),desc.len);
            }else if(desc.type == 2 && holds_alternative<string>(src[i])){
                string& tmp = get<string>(src[i]);
                if (tmp.size() > static_cast<size_t>(desc.len))
                    return -1;
                size_t tmplen = tmp.size();
                tmp.copy(row+j,tmplen,0);
                memset(row+j+tmplen,0,desc.len-tmplen);
            }else if(desc.type == 3 && holds_alternative<DATE>(src[i])){
                memcpy(row+j,&(get<DATE>(src[i])),desc.len);
            }
            else if(desc.type == 4 && holds_alternative<long long>(src[i])){
                memcpy(row+j,&(get<long long>(src[i])),desc.len);
            }
            else return -1;
        }
        j += desc.len;
    }
    return 0;
}
pair<int,int> InsertExecuter(DbManager& dbm, BoundInsertRequest& bir){
    if(bir.errorcode!=OK)return {0,bir.errorcode};
    int cnt=0;
    if(!dbm.id_table.count(bir.table_id))return {0,ERR_ELSE};
    Table* tb = dbm.id_table[bir.table_id];
    vector<ull> rids;
    for(int i=0;i<bir.rows.size();i++){
        ull res = dbm.InsertRow(tb, bir.rows[i]);
        if(res < 10){
            for(int j=cnt-1;j>=0;j--)dbm.DeleteApply(tb, rids[j]);
            return {0,res};
        }
        rids.push_back(res);
        cnt++;
    }
    return {cnt,0};
}
pair<int,int> DeleteExecuter(DbManager& dbm, BoundDeleteRequest& bdr){
    if(bdr.errorcode!=OK)return {0,bdr.errorcode};
    unique_ptr<Operator> root = nullptr;
    for(auto& i:bdr.conditions){
        if(i.type == ConditionType::SUB_COMPARE){
            if(!i.sub)return {0,ERR_ELSE};
            unique_ptr<Operator> root = Planner(&dbm, *(i.sub));
            Tuple input;
            if(root == nullptr || root->Open())return {0,ERR_ELSE};
            int res = root->Next(input);
            if(res == -1 || res == 1){
                root->Close();
                return {0,ERR_ELSE};
            }
            if(input.values.size() != 1){
                root->Close();
                return {0,ERR_ELSE};
            }
            Tuple extra;
            res = root->Next(extra);
            if(res != 1){
                root->Close();
                return {0,ERR_ELSE};
            }
            i.type = ConditionType::COMPARE;
            i.right.is_val = true;
            i.right.val = input.values[0];
            root->Close();
        }
        else if(i.type == ConditionType::SUB_IN){
            if(!i.sub)return {0,ERR_ELSE};
            unique_ptr<Operator> root = Planner(&dbm, *(i.sub));
            Tuple input;
            if(root == nullptr || root->Open())return {0,ERR_ELSE};
            i.values.clear();
            while(1){
                int res = root->Next(input);
                if(res == -1){
                    root->Close();
                    return {0,ERR_ELSE};
                }
                if(res == 1){
                    root->Close();
                    break;
                }
                if(input.values.size() != 1){
                    root->Close();
                    return {0,ERR_ELSE};
                }
                i.values.push_back(input.values[0]);
            }
            i.type = ConditionType::IN_LIST;
            root->Close();
        }
    }
    root = ScanMother(&dbm, bdr.conditions, bdr.table_id);
    
    if(bdr.conditions.size()>0)root = make_unique<Filter>(&dbm, move(root), bdr.conditions);
    vector<ull> rids;
    Tuple output;
    Table* tb = dbm.id_table[bdr.table_id];
    int cnt=0;
    if(root->Open()){
        root->Close();
        return {0,ERR_ELSE};
    }
    while(1){
        int res = root->Next(output);
        if(res == -1)return {0,ERR_ELSE};
        if(res == 1)break;
        if(output.rid)rids.push_back(output.rid.value());
    }
    root->Close();
    for(int i=0;i<rids.size();i++){
        int res = dbm.DeleteValid(tb, rids[i]);
        if(res != 0)return {0,res};
    }
    for(int i=0;i<rids.size();i++){
        int res = dbm.DeleteApply(tb, rids[i]);
        if(res!=0)return {cnt,res};
        cnt++;
    }
    return {cnt,OK};
}
pair<int,int> UpdateExecuter(DbManager& dbm, BoundUpdateRequest& bur){
    if(bur.errorcode!=OK)return {0,bur.errorcode};
    unique_ptr<Operator> root = nullptr;
    for(auto& i:bur.conditions){
        if(i.type == ConditionType::SUB_COMPARE){
            if(!i.sub)return {0,ERR_ELSE};
            unique_ptr<Operator> root = Planner(&dbm, *(i.sub));
            Tuple input;
            if(root == nullptr || root->Open())return {0,ERR_ELSE};
            int res = root->Next(input);
            if(res == -1 || res == 1){
                root->Close();
                return {0,ERR_ELSE};
            }
            if(input.values.size() != 1){
                root->Close();
                return {0,ERR_ELSE};
            }
            Tuple extra;
            res = root->Next(extra);
            if(res != 1){
                root->Close();
                return {0,ERR_ELSE};
            }
            i.type = ConditionType::COMPARE;
            i.right.is_val = true;
            i.right.val = input.values[0];
            root->Close();
        }
        else if(i.type == ConditionType::SUB_IN){
            if(!i.sub)return {0,ERR_ELSE};
            unique_ptr<Operator> root = Planner(&dbm, *(i.sub));
            Tuple input;
            if(root == nullptr || root->Open())return {0,ERR_ELSE};
            i.values.clear();
            while(1){
                int res = root->Next(input);
                if(res == -1){
                    root->Close();
                    return {0,ERR_ELSE};
                }
                if(res == 1){
                    root->Close();
                    break;
                }
                if(input.values.size() != 1){
                    root->Close();
                    return {0,ERR_ELSE};
                }
                i.values.push_back(input.values[0]);
            }
            i.type = ConditionType::IN_LIST;
            root->Close();
        }
    }
    root = ScanMother(&dbm, bur.conditions, bur.table_id);
    
    if(bur.conditions.size()>0)root = make_unique<Filter>(&dbm, move(root), bur.conditions);
    vector<ull> rids;
    Tuple output;
    Table* tb = dbm.id_table[bur.table_id];
    int cnt=0;
    if(root->Open())return {0,ERR_ELSE};
    while(1){
        int res = root->Next(output);
        if(res == -1)return {0,ERR_ELSE};
        if(res == 1)break;
        if(output.rid)rids.push_back(output.rid.value());
    }
    root->Close();
    vector<int> cid;
    Row& row = bur.values;
    for(auto& i:bur.col){
        cid.push_back(i.cid);
    }
    Schema& sc = tb->mng->tbmeta.schema;
    int recsize = sc.recordsize;
    for(int i=0;i<rids.size();i++){
        int res = dbm.UpdateValid(tb, rids[i], cid, row);
        if(res != 0)return {0,res};
        vector<Index*> indexes;
        if(tb->pkid!=-1)indexes.push_back(dbm.id_idx.at(tb->pkid));
        if(dbm.id_normidx.count(tb->id)){
            for(Index* index:dbm.id_normidx[tb->id])if(index->uni)indexes.push_back(index);
        }
    }
    for(int i=0;i<rids.size();i++){
        bool changed = false;
        int res = dbm.UpdateApply(tb, rids[i], cid, row, changed);
        if(res!=0){
            return {cnt, res};
        }
        if(changed)cnt++;
    }
    return {cnt,0};
}