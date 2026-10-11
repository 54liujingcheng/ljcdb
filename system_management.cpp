#include "system_management.h"
#include <charconv>
int serialization(Row& src, char* dest, Schema& sc){
    memset(dest,0,sc.recordsize);
    int nullen = sc.nullen, col = sc.col, j = nullen;
    for(int i=0;i<col;i++){
        Col& desc = sc.desc[i];
        if(holds_alternative<bool>(src[i])){
            dest[i >> 3] |= 1u << (i&7);
        }
        else{
            dest[i >> 3] &= ~(1u << (i&7));
            if(desc.type == 0 && holds_alternative<int>(src[i])){
                memcpy(dest+j,&(get<int>(src[i])),desc.len);
            }else if(desc.type == 1 && holds_alternative<double>(src[i])){
                memcpy(dest+j,&(get<double>(src[i])),desc.len);
            }else if(desc.type == 2 && holds_alternative<string>(src[i])){
                string tmp = get<string>(src[i]);
                if (tmp.size() > static_cast<size_t>(desc.len))
                    return -1;
                size_t tmplen = tmp.size();
                tmp.copy(dest+j,tmplen,0);
                memset(dest+j+tmplen,0,desc.len-tmplen);
            }else if(desc.type == 3 && holds_alternative<DATE>(src[i])){
                memcpy(dest+j,&(get<DATE>(src[i])),desc.len);
            }
            else return -1;
        }
        j += desc.len;
    }
    return 0;
}
int deserialization(char* src, Row& dest, Schema& sc){
    int nullen = sc.nullen, col = sc.col, j = nullen;
    dest.clear();
    for(int i=0;i<col;i++){
        Col& desc = sc.desc[i];
        if(src[i>>3] & (1u<<(i&7))){
            bool tmp=false;
            dest.push_back(tmp);
        }
        else{
            if(desc.type == 0){
                int tmp;
                memcpy(&tmp, src+j, desc.len);
                dest.push_back(tmp);
            }else if(desc.type == 1){
                double tmp;
                memcpy(&tmp, src+j, desc.len);
                dest.push_back(tmp);
            }else if(desc.type == 2){
                size_t len = strnlen(src + j, desc.len);
                string tmp(src + j, len);
                dest.push_back(tmp);
            }else if(desc.type == 3){
                DATE tmp;
                memcpy(&tmp, src+j, desc.len);
                dest.push_back(tmp);
            }
        }
        j += desc.len;
    }
    return 0;
}
int index_serialization(Row& src, char* dest, Schema& sc, Index* idx){
    memset(dest,0,idx->mng->keylen);
    int nullen = (idx->mng->meta.offset[0]), col = idx->col, j = nullen;
    for(int i=0;i<col;i++){
        Col& desc = sc.desc[idx->cid[i]];
        if(holds_alternative<bool>(src[i])){
            dest[i >> 3] |= 1u << (i&7);
        }
        else{
            dest[i >> 3] &= ~(1u << (i&7));
            if(desc.type == 0 && holds_alternative<int>(src[i])){
                memcpy(dest+j,&(get<int>(src[i])),desc.len);
            }else if(desc.type == 1 && holds_alternative<double>(src[i])){
                memcpy(dest+j,&(get<double>(src[i])),desc.len);
            }else if(desc.type == 2 && holds_alternative<string>(src[i])){
                string tmp = get<string>(src[i]);
                if (tmp.size() > static_cast<size_t>(desc.len))
                    return -1;
                size_t tmplen = tmp.size();
                tmp.copy(dest+j,tmplen,0);
                memset(dest+j+tmplen,0,desc.len-tmplen);
            }else if(desc.type == 3 && holds_alternative<DATE>(src[i])){
                memcpy(dest+j,&(get<DATE>(src[i])),desc.len);
            }
            else return -1;
        }
        j += desc.len;
    }
    return 0;
}
int index_deserialization(char* src, Row& dest, Schema& sc, Index* idx){
    int nullen = (idx->mng->meta.offset[0]), col = idx->col, j = nullen;
    dest.clear();
    for(int i=0;i<col;i++){
        Col& desc = sc.desc[idx->cid[i]];
        if(src[i>>3] & (1u<<(i&7))){
            bool tmp=false;
            dest.push_back(tmp);
        }
        else{
            if(desc.type == 0){
                int tmp;
                memcpy(&tmp, src+j, desc.len);
                dest.push_back(tmp);
            }else if(desc.type == 1){
                double tmp;
                memcpy(&tmp, src+j, desc.len);
                dest.push_back(tmp);
            }else if(desc.type == 2){
                size_t len = strnlen(src + j, desc.len);
                string tmp(src + j, len);
                dest.push_back(tmp);
            }else if(desc.type == 3){
                DATE tmp;
                memcpy(&tmp, src+j, desc.len);
                dest.push_back(tmp);
            }
        }
        j += desc.len;
    }
    return 0;
}
int LoadCSV(DbManager& dbm, string table_name, string load_path){
    FILE* f = fopen(load_path.c_str(), "rb");
    int cnt=0;
    if(!f)return 0;
    if(!dbm.name_table.count(table_name))return 0;
    Table* tb = dbm.name_table[table_name];
    Schema& sc = tb->mng->tbmeta.schema;
    Row row;
    row.reserve(sc.col);
    vector<Index*> indexes= dbm.id_normidx[tb->id];
    char buf[4096];
    if(tb->pkid != -1)indexes.push_back(dbm.id_idx[tb->pkid]);
    while(fgets(buf, sizeof(buf), f) != NULL){
        int cursor = 0;
        for(int i=0;i<sc.col;i++){
            int old_cursor = (i == 0?0:cursor+1);
            while(buf[cursor] != ',' && buf[cursor] != '\r' && buf[cursor] != '\n' && buf[cursor] != '\0'){
                    cursor++;
            }
            buf[cursor] = '\0';
            if(strcmp(buf+old_cursor, "NULL") == 0)row.push_back((bool)false);
            else if(sc.desc[i].type==0){
                int val;
                from_chars(buf+old_cursor, buf+cursor, val);
                row.push_back(val);
            }else if(sc.desc[i].type == 1){
                double val;
                from_chars(buf+old_cursor, buf+cursor, val);
                row.push_back(val);
            }else if(sc.desc[i].type == 2){
                string val(buf+old_cursor, buf+cursor);
                row.push_back(val);
            }else if(sc.desc[i].type == 3){
                DATE val{(buf[0+old_cursor]-'0')*10000000+(buf[1+old_cursor]-'0')*1000000+(buf[2+old_cursor]-'0')*100000+
                    (buf[3+old_cursor]-'0')*10000+(buf[5+old_cursor]-'0')*1000+(buf[6+old_cursor]-'0')*100+(buf[8+old_cursor]-'0')*10+buf[9+old_cursor]-'0'};
                row.push_back(val);
            }
        }
        dbm.InsertRow_raw(row, tb, sc, indexes);
        row.clear();
        cnt++;
    }
    fclose(f);
    return cnt;
}