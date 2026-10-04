#include "index_management.h"
#include "record_management.h"
#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>
#include <variant>
#include <unordered_map>
#include <fstream>
#include <bitset>
#include <unordered_set>
#include <memory>
#include <variant>
#define MAX_TBL 16
#define MAX_IDX 64
#define MAX_FK 32
#pragma once
namespace fs = std::filesystem;
struct PairHash {
    std::size_t operator()(const std::pair<int, std::string>& p) const {
        std::size_t h1 = std::hash<int>{}(p.first);
        std::size_t h2 = std::hash<std::string>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};
typedef vector<variant<int,double,string,bool>> Row;
int serialization(Row src, char* dest, Schema& sc){
    memset(dest,0,sc.recordsize);
    int nullen = sc.nullen, col = sc.col, j = nullen;
    int nullbm = 0;
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
                size_t tmplen = tmp.size();
                tmp.copy(dest+j,tmplen,0);
                memset(dest+j+tmplen,0,desc.len-tmplen);
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
            }else{
                size_t len = strnlen(src + j, desc.len);
                string tmp(src + j, len);
                dest.push_back(tmp);
            }
        }
        j += desc.len;
    }
    return 0;
}
struct Table{
    int id;
    char name[MAX_STR];
    unique_ptr<RecordManager> mng;
    string file_name;
    int pkid=-1;
    //int Insert(Row row, )
};
struct Index{
    int id;
    char name[MAX_STR];
    int table_id;
    int col;
    vector<int> cid;
    int uni;
    int implicit;
    int type;
    unique_ptr<BPT> mng;
    string file_name;
};
struct Foreign{
    int id;
    char name[MAX_STR];
    int pa_table_id;
    int ch_table_id;
    int pa_idx_id;
    int ch_idx_id;
};
struct DbManager{
    Table table[MAX_TBL];
    Index idx[MAX_IDX];
    Foreign fk[MAX_FK];
    FileManager* fm;
    BufPageManager* idx_bpm,*rec_bpm;
    unordered_map<int,Table*> id_table;
    unordered_map<int, vector<Foreign*>> id_outfk;//当前表作为子表的外键
    unordered_map<int, vector<Foreign*>> id_infk;//当前表作为父表的外键
    unordered_map<int, vector<Index*>> id_normidx;//关联当前表的普通索引
    unordered_map<string,Table*> name_table;
    std::unordered_map<std::pair<int, std::string>, Index*, PairHash> idname_idx;
    std::unordered_map<int, Index*> id_idx;//表id到表
    unordered_set<int> opened_fid;
    bitset<16> tablebm;
    bitset<64> idxbm;
    bitset<32> fkbm;
    int table_num,idx_num,next_tableid,next_idxid;
    int fk_num,next_fkid;
    string dbname;
    char* buf;
    DbManager(){
        id_table.clear();
        id_outfk.clear();
        id_infk.clear();
        id_normidx.clear();
        name_table.clear();
        idname_idx.clear();
        id_idx.clear();
        //tid_name_idx.clear();
        //tid_name_fk.clear();
        table_num = idx_num = next_tableid = next_idxid = fk_num = next_fkid = 0;
        tablebm.reset();
        idxbm.reset();
        fkbm.reset();
        opened_fid.clear();
        fm = new FileManager();
        idx_bpm = new BufPageManager(fm);
        rec_bpm = new BufPageManager(fm);
        buf = new char[PAGE_SIZE];
        dbname.clear();
    }
    ~DbManager(){
        CloseDb();
    }
    bool OpenFile(const std::string& path, int& fid) {
        if (!fm->openFile(path.c_str(), fid))
            return false;
        opened_fid.insert(fid);
        return true;
    }
    bool CloseFile(int& fid) {
        if (fm->closeFile(fid))
            return false;
        opened_fid.erase(fid);
        return true;
    }
    int CreateDb(string& name){
        fs::path db = name;
        db = fs::path("data") / db ;
        if(fs::exists(db))return -2;
        fs::create_directories(db);
        fs::path catalog = db, index = db, foreign = db;
        catalog/=".catalog";
        index/=".index";
        foreign/=".foreign";
        FILE* f = std::fopen(catalog.c_str(),"w");
        if(f == nullptr)return -1;
        std::fprintf(f,"0 0");
        std::fclose(f);
        f = std::fopen(index.c_str(),"w");
        if(f == nullptr)return -1;
        std::fprintf(f,"0 0");
        std::fclose(f);
        f = std::fopen(foreign.c_str(),"w");
        if(f == nullptr)return -1;
        std::fprintf(f,"0 0");
        std::fclose(f);
        return 0;
    }
    int CloseDb(){
        if(dbname.empty())return 0;
        fs::path db = dbname;
        db = fs::path("data") / db ;
        fs::path catalog = db, index = db, foreign = db;
        catalog/=".catalog";
        index/=".index";
        foreign/=".foreign";
        FILE* f = std::fopen(catalog.c_str(),"w");
        if(f == nullptr)return -1;
        fprintf(f,"%d %d\n",table_num,next_tableid);
        for(int i=0;i<MAX_TBL;i++){
            if(tablebm.test(i))fprintf(f,"%d %s\n",table[i].id,table[i].name);
            table[i].mng.reset();
            table[i].pkid = -1;
        }
        fclose(f);
        f = std::fopen(index.c_str(),"w");
        if(f == nullptr)return -1;
        fprintf(f,"%d %d\n",idx_num,next_idxid);
        for(int i=0;i<MAX_IDX;i++){
            if(idxbm.test(i)){
                fprintf(f,"%d %s %d %d",idx[i].id,idx[i].name,idx[i].table_id,idx[i].col);
                for(int j=0;j<idx[i].col;j++){
                    fprintf(f," %d",idx[i].cid[j]);
                }
                fprintf(f," %d %d %d\n",idx[i].uni,idx[i].implicit,idx[i].type);
            }
            idx[i].cid.clear();
            idx[i].mng.reset();
        }
        fclose(f);
        f = std::fopen(foreign.c_str(),"w");
        if(f == nullptr)return -1;
        fprintf(f,"%d %d\n",fk_num,next_fkid);
        for(int i=0;i<MAX_FK;i++){
            if(fkbm.test(i)){
                fprintf(f,"%d %s %d %d %d %d\n",fk[i].id, fk[i].name, fk[i].pa_table_id, fk[i].ch_table_id, fk[i].pa_idx_id,fk[i].ch_idx_id);
            }
        }
        fclose(f);
        idx_bpm->close();
        rec_bpm->close();
        for(int i:opened_fid){
            fm->closeFile(i);
        }
        opened_fid.clear();
        id_table.clear();
        id_outfk.clear();
        id_infk.clear();
        id_normidx.clear();
        name_table.clear();
        idname_idx.clear();
        id_idx.clear();
        //name_idx.clear();
        //name_fk.clear();
        table_num = idx_num = next_tableid = next_idxid = fk_num = next_fkid = 0;
        tablebm.reset();
        idxbm.reset();
        fkbm.reset();
        dbname.clear();
        return 0;
    }
    int DropDb(string& name){
        if(name == dbname)CloseDb();
        fs::path db = name;
        db = fs::path("data") / db ;
        std::error_code ec;
        bool success = fs::remove_all(db, ec);
        if(ec)return -1;
        else if(success)return 0;
        else return -2;
    }
    int UseDb(string& name){
        if(name == dbname)return 0;
        fs::path db = name;
        db = fs::path("data") / db ;
        fs::path catalog = db, index = db, foreign = db;
        if(!fs::exists(db))return -1;
        catalog/=".catalog";
        index/=".index";
        foreign/=".foreign";
        if(!fs::exists(catalog) || !fs::exists(index) || !fs::exists(foreign))return -1;
        CloseDb();
        FILE* f = fopen(catalog.c_str(),"r");
        if(f==nullptr)return -1;
        fscanf(f,"%d %d",&table_num,&next_tableid);
        for(int i=0;i<table_num;i++){
            fscanf(f,"%d %s",&table[i].id,table[i].name);
            table[i].file_name = db / fs::path(table[i].name);
            table[i].file_name+="_" + to_string(table[i].id) + ".ljcdb";
            int fid;
            OpenFile(table[i].file_name.c_str(),fid);
            table[i].mng = std::make_unique <RecordManager>(fid, rec_bpm);
            name_table[table[i].name] = table+i;
            id_table[table[i].id] = table+i;
            tablebm.set(i,1);
        }
        fclose(f);
        f = fopen(index.c_str(),"r");
        if(f==nullptr)return -1;
        fscanf(f,"%d %d",&idx_num, &next_idxid);
        for(int i=0;i<idx_num;i++){
            fscanf(f,"%d %s %d %d",&idx[i].id, idx[i].name, &idx[i].table_id, &idx[i].col);
            Table* tb = id_table.at(idx[i].table_id);
            idx[i].file_name =(db / (string(tb->name) + "_" +to_string(idx[i].id) + ".index")).string();
            //name_idx[idx[i].name] = idx+i;
            for(int j=0;j<idx[i].col;j++){
                int tmp;
                fscanf(f,"%d",&tmp);
                idx[i].cid.push_back(tmp);
            }
            fscanf(f,"%d %d %d",&idx[i].uni,&idx[i].implicit,&idx[i].type);
            if(idx[i].type == 0)id_table[idx[i].table_id]->pkid = idx[i].id;
            else id_normidx[idx[i].table_id].push_back(idx+i);
            idname_idx[{idx[i].table_id,idx[i].name}] = idx+i;
            id_idx[idx[i].id] = idx+i;
            int fid;
            OpenFile(idx[i].file_name.c_str(),fid);
            idx[i].mng = std::make_unique<BPT>(fid,idx_bpm);
            idxbm.set(i,1);
            //name_idx[idx[i].name] = idx+i;
        }
        fclose(f);
        f = fopen(foreign.c_str(),"r");
        if(f == nullptr)return -1;
        fscanf(f,"%d %d",&fk_num,&next_fkid);
        for(int i=0;i<fk_num;i++){
            fscanf(f,"%d %s", &fk[i].id, fk[i].name);
            fscanf(f,"%d %d %d %d",&fk[i].pa_table_id, &fk[i].ch_table_id, &fk[i].pa_idx_id, &fk[i].ch_idx_id);
            id_outfk[fk[i].ch_table_id].push_back(fk+i);
            id_infk[fk[i].pa_table_id].push_back(fk+i);
            //name_fk[fk[i].name] = fk+i;
            fkbm.set(i);
        }
        fclose(f);
        dbname = name;
        return 0;
    }
    vector<string> ShowAllDb(){
        vector<string> res;
        res.clear();
        fs::path data("data/");
        if (!fs::exists(data) || !fs::is_directory(data)) {
            return res;
        }
        for (const auto& entry : fs::directory_iterator(data)) {
            if (entry.is_directory()) {
                res.push_back(entry.path().filename().string());
            }
        }
        return res;
    }
    Table* CreateTable(char* name, Schema& sc){
        Table* tb=nullptr;
        for(int i=0;i<MAX_TBL;i++){
            if(!tablebm.test(i)){
                tablebm.set(i);
                tb = &table[i];
                break;
            }
        }
        if(tb == 0)return 0;
        fs::path path = fs::path("data") / fs::path(dbname) / fs::path(string(name) + "_" + to_string(next_tableid) + ".ljcdb");
        int fid;
        FILE* f = fopen(path.c_str(),"w");
        if(f)fclose(f);
        OpenFile(path,fid);
        tb->id = next_tableid++;
        tb->file_name = path;
        tb->pkid = -1;
        strcpy(tb->name,name);
        tb->mng = make_unique<RecordManager>(fid, rec_bpm, sc);
        table_num++;
        id_table[tb->id] = tb;
        name_table[name] = tb;
        return tb;
    }
    void DropTable(char* name){
        Table* tb = name_table[name];
        fs::path path = tb->file_name;
        if(!fs::exists(path))return ;
        int id = tb -> id;
        while (id_infk.count(id) && !id_infk[id].empty()) {
            Foreign* f = id_infk[id].back();
            DropForeign(id_table.at(f->ch_table_id)->name, f->name);
        }
        while (id_outfk.count(id) && !id_outfk[id].empty()) {
            Foreign* f = id_outfk[id].back();
            DropForeign(tb->name, f->name);
        }
        while (id_normidx.count(id) && !id_normidx[id].empty()) {
            DropIndex(tb->name, id_normidx[id].back()->name);
        }
        if (tb->pkid != -1) {
            DropIndex(tb->name, id_idx.at(tb->pkid)->name);
        }
        rec_bpm -> flushAndInvalidateFile(tb->mng->fid);
        CloseFile(tb->mng->fid);
        tb->mng.reset();
        fs::remove(path);
        table_num --;
        tablebm.reset(tb-table);
        name_table.erase(name);
        id_table.erase(id);
    }
    Index* CreateIndex(char* tablename, char* indexname, int col, vector<int>& cid, int unique, int implicit, int type){
        if(!name_table.count(tablename) || col!=cid.size() || col<=0)return 0;
        Table* tb = name_table[tablename];
        Index* ix = nullptr;
        if(type==0){
            unique = 1;
            if(tb->pkid != -1)return 0;
        }
        vector<Index*> normidx = id_normidx[tb->id];
        for(int i=0;i<normidx.size();i++){
            if(strcpy(normidx[i]->name,indexname)==0)return 0;
        }
        for(int i=0;i<col;i++){
            if(cid[i]>=tb->mng->tbmeta.schema.col)return 0;
        }
        for(int i=0;i<MAX_IDX;i++){
            if(!idxbm.test(i)){
                idxbm.set(i);
                ix = &idx[i];
                break;
            }
        }
        if(!ix)return 0;
        fs::path path = fs::path("data") / fs::path(dbname) / fs::path(string(tablename) + "_" + to_string(next_idxid) + ".index");
        int fid;
        FILE* f = fopen(path.c_str(),"w");
        if(f)fclose(f);
        OpenFile(path,fid);
        ix->id = next_idxid++;
        ix->table_id = tb->id;
        ix->file_name = path;
        strcpy(ix->name, indexname);
        ix->col = col;
        ix->cid = cid;
        ix->uni = unique;
        ix->type = type;
        ix->implicit = implicit;
        idx_num++;
        idname_idx[{tb->id,indexname}] = ix;
        id_idx[ix->id] = ix;
        if(type == 0)tb->pkid = ix->id;
        else id_normidx[tb->id].push_back(ix);
        BPTMeta bptmeta;
        bptmeta.unique = unique;
        bptmeta.col = col;
        Schema& sc = tb->mng->tbmeta.schema;
        Col* desc = sc.desc;
        for(int i=0;i<col;i++){
            bptmeta.type[i] = desc[cid[i]].type;
            if(i>=1)bptmeta.offset[i] = bptmeta.offset[i-1] + desc[cid[i-1]].len;
            else bptmeta.offset[i] = (col+7)/8;
        }
        bptmeta.offset[col] = bptmeta.offset[col-1] + desc[cid[col-1]].len;
        ix->mng = make_unique<BPT>(fid,idx_bpm,bptmeta);
        int recsize = sc.recordsize;
        bool flag=0;
        for(ull rid = tb->mng->FirstRecord();rid !=0;rid = tb->mng->NextRecord(rid)){
            tb->mng->LoadRecord(rid,buf);
            int j=recsize+(cid.size()+7)/8;
            for(int i=0;i<cid.size();i++){
                if(KeyColumnIsNull(buf,cid[i]))SetKeyColumnNull(buf+recsize,i);
                else ClearKeyColumnNull(buf+recsize,i);
                int cid_ = cid[i], len = sc.desc[cid_].len;
                memcpy(buf+j,buf+sc.offset[cid_],len);
                j+=len;
            }
            if(ix->mng->InsertEntry(buf+recsize,rid) != 0){
                flag = 1;
            }
            if(type==0&&KeyHasNull(buf+recsize,col))flag=1;
        }
        if(flag)DropIndex(tablename,indexname),ix=0;
        return ix;
    }
    void DropIndex(char* tablename, char* indexname, bool fromdf=false){
        //外键哈希表在外键删除部分处理
        if(name_table.count(tablename) == 0)return ;
        Table* tb = name_table[tablename];
        if(idname_idx.count({tb->id, indexname}) == 0)return ;
        Index* ix = idname_idx[{tb->id, indexname}];
        if(!fromdf)
            for(int i=0;i<MAX_FK;i++){
                if(fkbm.test(i) && (fk[i].pa_idx_id == ix->id || fk[i].ch_idx_id == ix->id))
                    return ;
            }
        fs::path path = ix->file_name;
        if(!fs::exists(path))return ;
        idname_idx.erase({tb->id,indexname});
        id_idx.erase(ix->id);
        if(ix->type == 1){
            auto& vec = id_normidx[tb->id];
            for (auto it = vec.begin(); it != vec.end(); ++it) {
                if ((*it)->id == ix->id) {
                    vec.erase(it);
                    if(vec.empty())id_normidx.erase(tb->id);
                    break;
                }
            }
        }else if(ix->type == 0){
            tb->pkid = -1;
        }
        idx_bpm -> flushAndInvalidateFile(ix->mng->fid);
        CloseFile(ix->mng->fid);
        ix->mng.reset();
        fs::remove(path);
        idx_num --;
        idxbm.reset(ix-idx);
    }
    Foreign* CreateForeign(char* _name, int pa_tableid, int ch_tableid, vector<int>& ccid){
        string __name;
        Table* ptb = id_table[pa_tableid], *ctb = id_table[ch_tableid];
        Index* pix,*cix;
        Foreign* fkey = nullptr;
        char* pa_name = ptb->name, *ch_name = ctb->name;
        if(ptb->pkid == -1)return nullptr;
        pix = id_idx[ptb->pkid];
        if(ccid.size()!=pix->col)return nullptr;
        Schema& csc = ctb->mng->tbmeta.schema;
        Schema& psc = ptb->mng->tbmeta.schema;
        for(int i=0;i<pix->col;i++){
            Col& l = psc.desc[pix->cid[i]];
            Col& r = csc.desc[ccid[i]];
            if(l.type != r.type || l.len != r.len)return nullptr;
        }
        int childsize = csc.recordsize;
        for(ull rid = ctb->mng->FirstRecord();rid != 0;rid = ctb->mng->NextRecord(rid)){
            ctb->mng->LoadRecord(rid,buf);
            int j=childsize+(ccid.size()+7)/8;
            for(int i=0;i<ccid.size();i++){
                if(KeyColumnIsNull(buf,ccid[i]))SetKeyColumnNull(buf+childsize,i);
                else ClearKeyColumnNull(buf+childsize,i);
                int cid = ccid[i], len = csc.desc[cid].len;
                memcpy(buf+j,buf+csc.offset[cid],len);
                j+=len;
            }
            if(!KeyHasNull(buf+childsize,ccid.size())){
                if(pix->mng->Exist(buf+childsize)==0){
                    return nullptr;
                }
            }
        }
        for(int i=0;i<MAX_FK;i++){
            if(!fkbm.test(i)){
                fkbm.set(i);
                fkey = &fk[i];
                break;
            }
        }
        if(!fkey)return 0;
        int flag=0;
        for(int i=0;i<MAX_IDX;i++){
            if(idxbm.test(i)&&idx[i].table_id == ch_tableid&&idx[i].cid == ccid){
                cix = idx+i;
                flag=1;
                break;
            }
        }
        __name = "fk_" + to_string(next_fkid);
        if(!_name)strcpy(fkey->name,__name.c_str());
        else strcpy(fkey->name,_name);
        if(!flag){
            cix = CreateIndex(id_table[ch_tableid]->name, fkey->name, ccid.size(), ccid, 0, 1, 1);
        }
        fkey->id = next_fkid++;
        fk_num++;
        id_outfk[ctb->id].push_back(fkey);
        id_infk[ptb->id].push_back(fkey);
        fkey->pa_table_id = pa_tableid;
        fkey->ch_table_id = ch_tableid;
        fkey->pa_idx_id = ptb->pkid;
        fkey->ch_idx_id = cix->id;
        return fkey;
    }
    void DropForeign(char* table_name,char* foreign_name){
        Foreign* fkey=nullptr;
        Table* tb = name_table[table_name];
        int used=0;
        vector<Foreign*>& outfk = id_outfk[tb->id];
        for(int i=0;i<outfk.size();i++){
            if(strcmp(outfk[i]->name,foreign_name)==0){
                fkey = outfk[i];
                fkbm.reset(fkey-fk);
                break;
            }
        }
        if(!fkey)return ;
        auto& vec = id_outfk[fkey->ch_table_id];
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if ((*it)->id == fkey->id) {
                vec.erase(it);
                if(vec.empty())id_outfk.erase(fkey->ch_table_id);
                break;
            }
        }
        auto& vec2 = id_infk[fkey->pa_table_id];
        for (auto it = vec2.begin(); it != vec2.end(); ++it) {
            if ((*it)->id == fkey->id) {
                vec2.erase(it);
                if(vec2.empty())id_infk.erase(fkey->pa_table_id);
                break;
            }
        }
        fk_num--;
        if(id_idx[fkey->ch_idx_id]->implicit){
            for(int i=0;i<MAX_FK;i++){
                if(fkbm.test(i)&&fk[i].ch_idx_id == fkey->ch_idx_id){
                    used = 1;
                    break;
                }
            }
            if(!used){
                DropIndex(table_name, id_idx[fkey->ch_idx_id]->name, true);
            }
        }
    }
    void ReadRow(string table_name, ull rid, Row& row){
        Table* tb = name_table[table_name];
        tb->mng->LoadRecord(rid,buf);
        deserialization(buf,row,tb->mng->tbmeta.schema);
    }
    void IntoIndexKey(char* src, char* dest, Schema& sc, vector<int>& cid){
        int j=(cid.size()+7)/8;
        for(int i=0;i<cid.size();i++){
            if(KeyColumnIsNull(src,cid[i]))SetKeyColumnNull(dest,i);
            else ClearKeyColumnNull(dest,i);
            int cid_ = cid[i], len = sc.desc[cid_].len;
            memcpy(dest+j,src+sc.offset[cid_],len);
            j+=len;
        }
    }
    ull InsertRow(string table_name,Row& row){
        Table* tb = name_table[table_name];
        Schema sc = tb->mng->tbmeta.schema;
        int recsize = sc.recordsize,col = row.size();
        if(row.size()!=sc.col)return 0;
        serialization(row,buf,sc);
        for(int i=0;i<sc.col;i++){
            if(KeyColumnIsNull(buf,i)){
                if((bool)(sc.desc[i].nullable) == 0)return 1;
                continue;
            }
            if(sc.desc[i].type == 0 && !holds_alternative<int>(row[i])
            || sc.desc[i].type == 1 && !holds_alternative<double>(row[i])
            || sc.desc[i].type == 2 && !holds_alternative<string>(row[i])
                )return 2;
        }
        Index* primary ;
        if(tb->pkid != -1){
            primary = id_idx[tb->pkid];
            IntoIndexKey(buf,buf+recsize,sc,primary->cid);
            if(primary->mng->Exist(buf+recsize))return 3;
        }
        vector<Index*>& norm = id_normidx[tb->id];
        for(int i=0;i<norm.size();i++){
            Index* normidx = norm[i];
            IntoIndexKey(buf,buf+recsize,sc,normidx->cid);
            if(normidx->uni&&normidx->mng->Exist(buf+recsize))return 4;
        }
        vector<Foreign*>& localfk = id_outfk[tb->id];
        for(int i=0;i<localfk.size();i++){
            Foreign* fkey = localfk[i];
            Index* childidx = id_idx[fkey->ch_idx_id];
            IntoIndexKey(buf,buf+recsize,sc,childidx->cid);
            if(!KeyHasNull(buf+recsize, childidx->col) &&!childidx->mng->Exist(buf+recsize))return 5;
        }
        int rid = tb->mng->InsertRecord(buf);
        if(rid==0)return 6;
        if(tb->pkid != -1){
            IntoIndexKey(buf,buf+recsize,sc,primary->cid);
            primary->mng->InsertEntry(buf+recsize,rid);
        }
        norm = id_normidx[tb->id];
        for(int i=0;i<norm.size();i++){
            Index* normidx = norm[i];
            IntoIndexKey(buf,buf+recsize,sc,normidx->cid);
            normidx->mng->InsertEntry(buf+recsize,rid);
        }
        return rid;
    }
    vector<ull> InsertRows(string table_name,vector<Row>& vec){
        vector<ull> rids;
        for(int i=0;i<vec.size();i++){
            rids.push_back(InsertRow(table_name, vec[i]));
        }
        return rids;
    }
};
/*
文件格式：
/data/dbname
    .index
    .catalog
    table_name.ljcdb
    idx_name.index

.catalog
table_num next_id
id table_name

.index
index_num next_id
(type:0->pk 1->norm)
id name table_id col col_id[] uni implicit type

.foreign
fk_num next_id
id name pa_table_id ch_table_id pa_idx_id ch_idx_id
*/