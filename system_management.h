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
#include <compare>
#define MAX_TBL 16
#define MAX_IDX 64
#define MAX_FK 32
#pragma once
namespace fs = std::filesystem;
enum Errortype{
    OK, ERR_DUPULICATE, ERR_FORIEGN, ERR_PRIMARY, ERR_DATE, ERR_NULL, ERR_ELSE
};
struct DATE{
    int date;
    auto operator<=>(const DATE&) const = default;
};
namespace std {
    template <>
    struct hash<DATE> {
        std::size_t operator()(const DATE& p) const noexcept {
            return std::hash<int>{}(p.date);
        }
    };
}
struct PairHash {
    std::size_t operator()(const std::pair<int, std::string>& p) const {
        std::size_t h1 = std::hash<int>{}(p.first);
        std::size_t h2 = std::hash<std::string>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};
typedef vector<variant<int,double,string,bool,DATE,long long>> Row;
int serialization(Row& src, char* dest, Schema& sc);
int deserialization(char* src, Row& dest, Schema& sc);
struct Table{
    int id;
    char name[MAX_STR];
    unique_ptr<RecordManager> mng;
    string file_name;
    int pkid=-1;
    unordered_map <string, int> name_cid;
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
int index_serialization(Row& src, char* dest, Schema& sc, Index* idx);
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
        rec_bpm = idx_bpm;
        buf = new char[PAGE_SIZE];
        dbname.clear();
    }
    ~DbManager(){
        CloseDb();
        delete[] buf;
        delete idx_bpm;
        delete fm;
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
            table[i].name_cid.clear();
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
            Schema& sc = table[i].mng->tbmeta.schema;
            for(int j=0;j<sc.col;j++){
                table[i].name_cid[sc.desc[j].name] = j;
            }
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
        if(dbname.empty() || name_table.count(name))return 0;
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
        tb->name_cid.clear();
        for(int i=0;i<sc.col;i++){
            tb->name_cid[sc.desc[i].name] = i;
        }
        return tb;
    }
    void DropTable(char* name){
        if(!name_table.count(name))return ;
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
        tb->name_cid.clear();
    }
    Index* CreateIndex(char* tablename, char* indexname, int col, vector<int>& cid, int unique, int implicit, int type){
        if(!name_table.count(tablename) || col!=cid.size() || col<=0)return 0;
        Table* tb = name_table[tablename];
        if(idname_idx.count({tb->id, indexname}))return 0;
        Index* ix = nullptr;
        if(type==0){
            unique = 1;
            if(tb->pkid != -1)return 0;
        }
        vector<Index*> normidx = id_normidx[tb->id];
        for(int i=0;i<normidx.size();i++){
            if(strcmp(normidx[i]->name,indexname)==0)return 0;
            if(normidx[i]->type == 1&&normidx[i]->implicit == 0&&normidx[i]->cid == cid)return 0;
        }
        for(int i=0;i<col;i++){
            if(cid[i]>=tb->mng->tbmeta.schema.col || cid[i]<0)return 0;
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
            if(type==0&&KeyHasNull(buf+recsize,col)){
                flag=1;
                break;
            }
            if(ix->mng->InsertEntry(buf+recsize,rid) != 0){
                flag = 1;
                break;
            }
        }
        if(flag)DropIndex(tablename,indexname),ix=0;
        return ix;
    }
    void DropIndex(char* tablename, char* indexname){
        //外键哈希表在外键删除部分处理
        if(name_table.count(tablename) == 0)return ;
        Table* tb = name_table[tablename];
        if(idname_idx.count({tb->id, indexname}) == 0)return ;
        Index* ix = idname_idx[{tb->id, indexname}];
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
        if(!id_table.count(pa_tableid) || !id_table.count(ch_tableid))return 0;
        if(_name && id_outfk.count(ch_tableid))
            for(auto i:id_outfk[ch_tableid]){
                if(strcmp(_name, i->name)==0)return 0;
            }
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
            if(ccid[i]<0 || ccid[i] >= csc.col)return 0;
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
        int iii;
        for(int i=0;i<MAX_FK;i++){
            if(!fkbm.test(i)){
                iii=i;
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
            if(cix == nullptr)return 0;
        }
        fkbm.set(iii);
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
        if(!name_table.count(table_name))return ;
        Table* tb = name_table[table_name];
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
        if(id_idx[fkey->ch_idx_id]->implicit == 1&& id_idx[fkey->ch_idx_id]->uni == 0&& id_idx[fkey->ch_idx_id]->type == 1){
                DropIndex(table_name, id_idx[fkey->ch_idx_id]->name);
        }
    }
    Row ReadRow(string table_name, ull rid){
        Row row;
        if(!name_table.count(table_name))return row;
        Table* tb = name_table[table_name];
        if(!tb->mng->isValid(rid))return row;
        tb->mng->LoadRecord(rid,buf);
        deserialization(buf,row,tb->mng->tbmeta.schema);
        return row;
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
        if(!name_table.count(table_name))return ERR_ELSE;
        Table* tb = name_table[table_name];
        Schema& sc = tb->mng->tbmeta.schema;
        vector<Index*> indexes;
        int recsize = sc.recordsize,col = row.size();
        if(row.size()!=sc.col)return ERR_ELSE;
        if(serialization(row,buf,sc) == -1)return ERR_ELSE;
        for(int i=0;i<sc.col;i++){
            if(KeyColumnIsNull(buf,i)){
                if((bool)(sc.desc[i].nullable) == 0)return ERR_NULL;
                continue;
            }
            if(sc.desc[i].type == 0 && !holds_alternative<int>(row[i])
            || sc.desc[i].type == 1 && !holds_alternative<double>(row[i])
            || sc.desc[i].type == 2 && !holds_alternative<string>(row[i])
            || sc.desc[i].type == 3 && !holds_alternative<DATE>(row[i])
                )return ERR_ELSE;
        }
        Index* primary ;
        if(tb->pkid != -1){
            primary = id_idx[tb->pkid];
            IntoIndexKey(buf,buf+recsize,sc,primary->cid);
            indexes.push_back(primary);
            if(KeyHasNull(buf+recsize, primary->col))return ERR_PRIMARY;
            if(primary->mng->Exist(buf+recsize))return ERR_PRIMARY;
        }
        vector<Index*>& norm = id_normidx[tb->id];
        for(int i=0;i<norm.size();i++){
            Index* normidx = norm[i];
            indexes.push_back(norm[i]);
            IntoIndexKey(buf,buf+recsize,sc,normidx->cid);
            if(normidx->uni&&!KeyHasNull(buf+recsize,normidx->col)&&normidx->mng->Exist(buf+recsize))return ERR_DUPULICATE;
        }
        vector<Foreign*>& localfk = id_outfk[tb->id];
        for(int i=0;i<localfk.size();i++){
            Foreign* fkey = localfk[i];
            Index* childidx = id_idx[fkey->ch_idx_id];
            IntoIndexKey(buf,buf+recsize,sc,childidx->cid);
            Index* parentidx = id_idx.at(fkey->pa_idx_id);
            if (!KeyHasNull(buf+recsize, childidx->col) &&!parentidx->mng->Exist(buf+recsize))return ERR_FORIEGN;
        }
        ull rid = tb->mng->InsertRecord(buf);
        if(rid<=0)return 9;
        vector<Index*> insertedIndexes;
        for (Index* index : indexes) {
            IntoIndexKey(buf, buf + recsize, sc, index->cid);
            if (index->mng->InsertEntry(buf + recsize, rid) != 0) {
                for (auto it = insertedIndexes.rbegin();
                    it != insertedIndexes.rend(); ++it) {
                    IntoIndexKey(buf, buf + recsize, sc, (*it)->cid);
                    (*it)->mng->DeleteEntry(buf + recsize, rid);
                }
                tb->mng->EraseRecord(rid);
                return ERR_DUPULICATE;
            }
            insertedIndexes.push_back(index);
        }
        return rid;
    }
    int DeleteRow(string table_name, ull rid){
        if(!name_table.count(table_name))return ERR_ELSE;
        Table* tb = name_table[table_name];
        Schema& sc = tb->mng->tbmeta.schema;
        vector<Index*> indexes;
        int recsize = sc.recordsize;
        if(!tb->mng->isValid(rid))return ERR_ELSE;
        if(id_infk.count(tb->id) && !id_infk[tb->id].empty()){
            vector<Foreign*>& in = id_infk[tb->id];
            for(auto fk:in){
                Index* chidx = id_idx[fk->ch_idx_id];
                Index* paidx = id_idx[fk->pa_idx_id];
                tb->mng->LoadRecord(rid,buf);
                IntoIndexKey(buf,buf+recsize,sc,paidx->cid);
                if(chidx->mng->Exist(buf+recsize))return ERR_FORIEGN;
            }
        }
        vector<Index*> deletedindexes;
        if(id_normidx.count(tb->id) && !id_normidx[tb->id].empty()){
            vector<Index*>& norm = id_normidx[tb->id];
            for(auto idx:norm){
                tb->mng->LoadRecord(rid,buf);
                IntoIndexKey(buf,buf+recsize,sc,idx->cid);
                indexes.push_back(idx);
            }
        }
        if(tb->pkid != -1){
            Index* primary = id_idx[tb->pkid];
            tb->mng->LoadRecord(rid,buf);
            IntoIndexKey(buf,buf+recsize,sc,primary->cid);
            indexes.push_back(primary);
        }
        for (Index* index : indexes) {
            IntoIndexKey(buf, buf + recsize, sc, index->cid);
            if (index->mng->DeleteEntry(buf + recsize, rid) != 0) {
                for (auto it = deletedindexes.rbegin();
                    it != deletedindexes.rend(); ++it) {
                    IntoIndexKey(buf, buf + recsize, sc, (*it)->cid);
                    (*it)->mng->InsertEntry(buf + recsize, rid);
                }
                return ERR_ELSE;
            }
            deletedindexes.push_back(index);
        }
        tb->mng->EraseRecord(rid);
        return OK;
    }
    #ifdef TEST
    int DeleteRows(string table_name, vector<ull>& vec){
        if(!name_table.count(table_name))return 1;
        Table* tb = name_table[table_name];
        Schema& sc = tb->mng->tbmeta.schema;
        int recsize = sc.recordsize;
        for(auto rid:vec){
            if(!tb->mng->isValid(rid))return 3;
        }
        if(id_infk.count(tb->id) && !id_infk[tb->id].empty()){
            vector<Foreign*>& in = id_infk[tb->id];
            for(auto fk:in){
                for(auto rid:vec){
                    Index* chidx = id_idx[fk->ch_idx_id];
                    Index* paidx = id_idx[fk->pa_idx_id];
                    tb->mng->LoadRecord(rid,buf);
                    IntoIndexKey(buf,buf+recsize,sc,paidx->cid);
                    if(chidx->mng->Exist(buf+recsize))return 2;
                }
            }
        }
        if(id_normidx.count(tb->id) && !id_normidx[tb->id].empty()){
            vector<Index*>& norm = id_normidx[tb->id];
            for(auto idx:norm){
                for(auto rid:vec){
                    tb->mng->LoadRecord(rid,buf);
                    IntoIndexKey(buf,buf+recsize,sc,idx->cid);
                    idx->mng->DeleteEntry(buf+recsize,rid);
                }
            }
        }
        if(tb->pkid != -1){
            Index* primary = id_idx[tb->pkid];
            for(auto rid:vec){
                tb->mng->LoadRecord(rid,buf);
                IntoIndexKey(buf,buf+recsize,sc,primary->cid);
                primary->mng->DeleteEntry(buf+recsize,rid);
            }
        }
        for(auto rid:vec){
            tb->mng->EraseRecord(rid);
        }
        return 0;
    }
    #endif
    int UpdateRow(string table_name, ull rid, vector<int>& cid, Row& row, bool& changed){
        changed = false;
        if(!name_table.count(table_name))return ERR_ELSE;
        Table* tb = name_table[table_name];
        Schema& sc = tb->mng->tbmeta.schema;
        if (!tb->mng->isValid(rid))return ERR_ELSE;
        if (cid.size() != row.size())return ERR_ELSE;
        for(size_t i=0;i<cid.size();i++){
            if(cid[i]<0 || cid[i]>=sc.col)return ERR_ELSE;
        }
        int recsize = sc.recordsize;
        char* oldRecord = buf, *newRecord = buf+recsize;
        char* oldKey = buf+2*recsize, *newKey = buf+3*recsize;
        tb->mng->LoadRecord(rid, oldRecord);
        Row newRow;
        deserialization(oldRecord,newRow,sc);
        for(size_t i=0;i<cid.size();i++){
            newRow[cid[i]]=row[i];
        }
        if(serialization(newRow, newRecord, sc))return ERR_ELSE;
        for (int i = 0; i < sc.col; i++) {
            if (KeyColumnIsNull(newRecord, i) && !sc.desc[i].nullable) {
                return ERR_NULL;
            }
        }
        if (memcmp(oldRecord, newRecord, recsize) == 0) {
            return OK;
        }
        vector<Index*> indexes;
        if(tb->pkid!=-1)indexes.push_back(id_idx.at(tb->pkid));
        if(id_normidx.count(tb->id)){
            for(Index* index:id_normidx[tb->id])indexes.push_back(index);
        }
        for(Index* index:indexes){
            IntoIndexKey(oldRecord,oldKey,sc,index->cid);
            IntoIndexKey(newRecord,newKey,sc,index->cid);
            if(index->type==0 && KeyHasNull(newKey,index->col))return ERR_PRIMARY;
            if(index->mng->cmp(oldKey,newKey,0,0)==0)continue;
            if(index->type==0 && index->mng->Exist(newKey))return ERR_DUPULICATE;
            if(index->type!=0 && index->uni &&
               !KeyHasNull(newKey,index->col) && index->mng->Exist(newKey))return ERR_DUPULICATE;
        }
        if(id_outfk.count(tb->id)){
            for(Foreign* foreign:id_outfk[tb->id]){
                Index* childIndex = id_idx.at(foreign->ch_idx_id);
                Index* parentIndex = id_idx.at(foreign->pa_idx_id);
                IntoIndexKey(oldRecord,oldKey,sc,childIndex->cid);
                IntoIndexKey(newRecord,newKey,sc,childIndex->cid);
                if(childIndex->mng->cmp(oldKey,newKey,0,0)==0 ||
                   KeyHasNull(newKey,childIndex->col))continue;
                if(parentIndex->mng->Exist(newKey))continue;
                if(foreign->pa_table_id==tb->id){
                    IntoIndexKey(newRecord,oldKey,sc,parentIndex->cid);
                    if(parentIndex->mng->cmp(oldKey,newKey,0,0)==0)continue;
                }
                return ERR_FORIEGN;
            }
        }
        if(id_infk.count(tb->id)){
            for(Foreign* foreign:id_infk[tb->id]){
                Index* parentIndex = id_idx.at(foreign->pa_idx_id);
                Index* childIndex = id_idx.at(foreign->ch_idx_id);
                IntoIndexKey(oldRecord,oldKey,sc,parentIndex->cid);
                IntoIndexKey(newRecord,newKey,sc,parentIndex->cid);
                if(parentIndex->mng->cmp(oldKey,newKey,0,0)==0)continue;
                SearchIter iter = childIndex->mng->SearchRange(oldKey,oldKey,1,1);
                while(childIndex->mng->Valid(iter)){
                    bool remainsReferenced = true;
                    if(foreign->ch_table_id==tb->id && iter.rec_rid==rid){
                        IntoIndexKey(newRecord,newKey,sc,childIndex->cid);
                        remainsReferenced = !KeyHasNull(newKey,childIndex->col) &&
                            childIndex->mng->cmp(newKey,oldKey,0,0)==0;
                    }
                    if(remainsReferenced)return ERR_FORIEGN;
                    if(!childIndex->mng->Next(iter))break;
                }
            }
        }
        vector<Index*> updatedIndexes;
        for(Index* index:indexes){
            IntoIndexKey(oldRecord,oldKey,sc,index->cid);
            IntoIndexKey(newRecord,newKey,sc,index->cid);
            if(index->mng->cmp(oldKey,newKey,0,0)==0)continue;
            if(index->mng->InsertEntry(newKey,rid)!=0 ||
               index->mng->DeleteEntry(oldKey,rid)!=0){
                index->mng->DeleteEntry(newKey,rid);
                for(auto it=updatedIndexes.rbegin();it!=updatedIndexes.rend();++it){
                    IntoIndexKey(oldRecord,oldKey,sc,(*it)->cid);
                    IntoIndexKey(newRecord,newKey,sc,(*it)->cid);
                    (*it)->mng->InsertEntry(oldKey,rid);
                    (*it)->mng->DeleteEntry(newKey,rid);
                }
                return ERR_ELSE;
            }
            updatedIndexes.push_back(index);
        }
        tb->mng->UpdataRecord(rid,newRecord);
        changed = true;
        return OK;
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
