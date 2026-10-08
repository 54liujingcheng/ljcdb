#include "query.h"
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
                return BoundInsertRequest{table_id, req.rows, ERR_ELSE};
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
pair<Condition, int> ConditionBindingHelper(Table* ltb,Table* rtb, UnboundCondition& i){
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
            for(auto i:tmp.values){
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
    return {tmp,0};
}
BoundDeleteRequest BindingDeleteRequest(DbManager& dbm,UnboundDeleteRequest& req){
    vector<Condition> conditions;
    if(!dbm.name_table.count(req.FROM))return BoundDeleteRequest{0, conditions, ERR_ELSE};
    Table* tb = dbm.name_table[req.FROM];
    Schema& sc = tb->mng->tbmeta.schema;
    int table_id = tb->id;
    for(auto i:req.conditions){
        if(!i.left.is_val && !i.left.col_name.first.empty() && i.left.col_name.first != req.FROM )return {table_id, conditions, ERR_ELSE};
        if(i.type == ConditionType::COMPARE){
            if(!i.right.is_val && !i.right.col_name.first.empty() && i.right.col_name.first != req.FROM)return {table_id, conditions, ERR_ELSE};
        }
        pair<Condition,int> tmp = ConditionBindingHelper(tb,tb, i);
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
    for(auto i:req.col_names){
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
            else return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_ELSE};
        }
        if(col[i].type == 3 ){
            if(holds_alternative<string>(req.values[i]) && is_valid_date_string(get<string>(req.values[i]))){
                req.values[i] = parse_date_yyyy_mm_dd(get<string>(req.values[i])).value();
            }
            else return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_ELSE};
        }
        if(holds_alternative<int>(req.values[i]) && col[i].type!=0 || holds_alternative<double>(req.values[i]) && col[i].type!=1 || holds_alternative<string>(req.values[i]) && col[i].type!=2)return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_ELSE};
    }
    for(auto i:req.conditions){
        if(!i.left.is_val && !i.left.col_name.first.empty() && i.left.col_name.first != req.FROM )return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_ELSE};
        if(i.type == ConditionType::COMPARE){
            if(!i.right.is_val && !i.right.col_name.first.empty() && i.right.col_name.first != req.FROM)return BoundUpdateRequest{tb->id, col, req.values, conditions, ERR_ELSE};
        }
        pair<Condition,int> tmp = ConditionBindingHelper(tb,tb, i);
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
    bool withagg = false;
    for(auto i:req.FROM){
        if(dbm.name_table.count(i)==0)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
        table_ids.push_back(dbm.name_table[i]->id);
    }
    if(req.col_items[0].first.first.empty() && req.col_items[0].first.second == "*"){
        for(auto i:table_ids){
            Schema& sc = dbm.id_table[i]->mng->tbmeta.schema;
            for(int j=0;j<sc.col;j++){ 
                BoundColumn tmp = {i, j, sc.desc[j].type, sc.desc[j].nullable};
                col_items.push_back({tmp,AggregateFunc::NONE});
            }
        }
    }
    else
    for(auto i:req.col_items){
        string table_name = i.first.first, col_name = i.first.second;
        AggregateFunc agg = i.second;
        if(agg!=AggregateFunc::NONE)withagg = true;
        if(table_name.empty()&&col_name.empty()){
            if(agg==AggregateFunc::COUNT)col_items.push_back({std::nullopt, agg});
            else return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
        }
        else if(table_name.empty()&&!col_name.empty()){
            int cnt=0,idx=0,cid=0;
            for(auto j:table_ids){
                Table* tb = dbm.id_table[j];
                if(tb->name_cid.count(col_name))idx=j,cid=tb->name_cid[col_name],cnt++;
            }
            if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            Schema& sc = dbm.id_table[idx]->mng->tbmeta.schema;
            BoundColumn tmp={idx,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            col_items.push_back({tmp,agg});
        }
        else if(!table_name.empty()&&!col_name.empty()){
            if(!dbm.name_table.count(table_name) )return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            int flag=0;
            for(int i=0;i<table_ids.size();i++){
                if(dbm.id_table[table_ids[i]]->name == table_name){
                    flag = 1;
                    break;
                }
            }
            if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            Table* tb = dbm.name_table[table_name];
            if(!tb->name_cid.count(col_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            int cid = tb->name_cid[col_name];
            Schema& sc = tb->mng->tbmeta.schema;
            BoundColumn tmp={tb->id,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            col_items.push_back({tmp,agg});
        }
        else return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
        if((agg == AggregateFunc::AVG || agg == AggregateFunc::SUM) && col_items.back().first->type != 0 && col_items.back().first->type != 1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
    }
    for(auto i:req.ORDERBY){
        string table_name = i.first.first, col_name = i.first.second;
        OrderBy od = i.second;
        if(table_name.empty()&&col_name.empty()){
            return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
        }
        else if(table_name.empty()&&!col_name.empty()){
            int cnt=0,idx=0,cid=0;
            for(auto j:table_ids){
                Table* tb = dbm.id_table[j];
                if(tb->name_cid.count(col_name))idx=j,cid=tb->name_cid[col_name],cnt++;
            }
            if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            Schema& sc = dbm.id_table[idx]->mng->tbmeta.schema;
            BoundColumn tmp={idx,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            ORDERBY.push_back({tmp,od});
        }
        else if(!table_name.empty()&&!col_name.empty()){
            if(!dbm.name_table.count(table_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            int flag=0;
            for(int i=0;i<table_ids.size();i++){
                if(dbm.id_table[table_ids[i]]->name == table_name){
                    flag = 1;
                    break;
                }
            }
            if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            Table* tb = dbm.name_table[table_name];
            if(!tb->name_cid.count(col_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            int cid = tb->name_cid[col_name];
            Schema& sc = tb->mng->tbmeta.schema;
            BoundColumn tmp={tb->id,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            ORDERBY.push_back({tmp,od});
        }
        else return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
    }
    for(auto i:req.GROUPBY){
        string table_name = i.first, col_name = i.second;
        if(table_name.empty()&&col_name.empty()){
            return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
        }
        else if(table_name.empty()&&!col_name.empty()){
            int cnt=0,idx=0,cid=0;
            for(auto j:table_ids){
                Table* tb = dbm.id_table[j];
                if(tb->name_cid.count(col_name))idx=j,cid=tb->name_cid[col_name],cnt++;
            }
            if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            Schema& sc = dbm.id_table[idx]->mng->tbmeta.schema;
            BoundColumn tmp={idx,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            GROUPBY.push_back(tmp);
        }
        else if(!table_name.empty()&&!col_name.empty()){
            if(!dbm.name_table.count(table_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            int flag=0;
            for(int i=0;i<table_ids.size();i++){
                if(dbm.id_table[table_ids[i]]->name == table_name){
                    flag = 1;
                    break;
                }
            }
            if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            Table* tb = dbm.name_table[table_name];
            if(!tb->name_cid.count(col_name))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
            int cid = tb->name_cid[col_name];
            Schema& sc = tb->mng->tbmeta.schema;
            BoundColumn tmp={tb->id,cid,sc.desc[cid].type,sc.desc[cid].nullable};
            GROUPBY.push_back(tmp);
        }
        else return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
    }
    if(withagg || !req.GROUPBY.empty())
    for(int i=0;i<col_items.size();i++){
        if(col_items[i].first&&col_items[i].second == AggregateFunc::NONE){
            int flag=0;
            for(int j=0;j<GROUPBY.size();j++){
                if(GROUPBY[j].cid == col_items[i].first.value().cid && GROUPBY[j].table_id == col_items[i].first->table_id){
                    flag = 1;
                    break;
                }
            }
            if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
        }
    }
    for(auto i:req.conditions){
        if(i.type == ConditionType::COMPARE){
            if(!i.left.is_val&&!i.right.is_val){
                string l_tname = i.left.col_name.first, l_cname = i.left.col_name.second;
                string r_tname = i.right.col_name.first, r_cname = i.right.col_name.second;
                Table* l, *r;
                int lcid, rcid;
                if(l_tname.empty()){
                    int cnt=0, idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(l_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    l = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(l_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    l = dbm.name_table[l_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(l->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                }
                if(r_tname.empty()){
                    int cnt=0,idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(r_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    r = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(r_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    r = dbm.name_table[r_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(r->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                }
                if(!l->name_cid.count(l_cname) || !r->name_cid.count(r_cname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                pair<Condition,int> tmp = ConditionBindingHelper(l, r, i);
                if(tmp.second==0)conditions.push_back(tmp.first);
                else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, tmp.second};
            }
            else if(!i.left.is_val){
                string l_tname = i.left.col_name.first, l_cname = i.left.col_name.second;
                Table* l;
                int lcid;
                if(l_tname.empty()){
                    int cnt=0, idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(l_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    l = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(l_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    l = dbm.name_table[l_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(l->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                }
                if(!l->name_cid.count(l_cname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                lcid = l->name_cid.count(l_cname);
                pair<Condition,int> tmp = ConditionBindingHelper(l, l, i);
                if(tmp.second==0)conditions.push_back(tmp.first);
                else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, tmp.second};
            }
            else if(i.left.is_val&&!i.right.is_val){
                string r_tname = i.right.col_name.first, r_cname = i.right.col_name.second;
                Table* r;
                int rcid;
                if(r_tname.empty()){
                    int cnt=0, idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(r_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    r = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(r_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    r = dbm.name_table[r_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(r->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                }
                if(!r->name_cid.count(r_cname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                rcid = r->name_cid.count(r_cname);
                pair<Condition,int> tmp = ConditionBindingHelper(r, r, i);
                if(tmp.second==0)conditions.push_back(tmp.first);
                else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, tmp.second};
            }
            else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
        }
        else{
            if(!i.left.is_val){
                string l_tname = i.left.col_name.first, l_cname = i.left.col_name.second;
                Table* l;
                int lcid;
                if(l_tname.empty()){
                    int cnt=0, idx=0;
                    for(auto j:table_ids){
                        if(dbm.id_table[j]->name_cid.count(l_cname))cnt++, idx = j;
                    }
                    if(cnt==0||cnt>1)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    l = dbm.id_table[idx];
                }
                else{
                    if(!dbm.name_table.count(l_tname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                    l = dbm.name_table[l_tname];
                    int flag=0;
                    for(auto j:table_ids){
                        if(l->id == j){
                            flag=1;break;
                        }
                    }
                    if(!flag)return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                }
                if(!l->name_cid.count(l_cname))return {table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
                lcid = l->name_cid.count(l_cname);
                pair<Condition,int> tmp = ConditionBindingHelper(l, l, i);
                if(tmp.second==0)conditions.push_back(tmp.first);
                else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, tmp.second};
            }
            else return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, ERR_ELSE};
        }
    }
    return BoundSelectRequest{table_ids, col_items, ORDERBY, GROUPBY, conditions, req.LIMIT, req.OFFSET, 0};
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
unique_ptr<Operator> Planner(BoundSelectRequest& bsr){

}