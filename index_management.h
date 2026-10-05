#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"
#include <cstring>
#include <cmath>
#include <cstdint>
#include <iostream>
#pragma once
#ifndef PID
#define PID(x) ((int)(x>>32))
#endif
#ifndef SID
#define SID(x) ((int)(x&0xFFFFFFFF))
#endif
#ifndef RID
#define RID(x,y) (((ull)(x)<<32)|((ull)(y)))
#endif
#ifndef RIDMIN
#define RIDMIN 0ULL
#endif
#ifndef RIDMAX
#define RIDMAX UINT64_MAX
#endif
struct TreeKey{
    char* key;
    ull rid;
};
struct InternalKey{
    TreeKey key;
    int cpid;
};
#define BPTMETA (8*4+4*64)
#define PAGEMETA 16
struct BPTMeta{
    int root=1;//root pid
    int lhead=1;
    int ltail=1;//leaf链表头尾
    int freehead=0;
    int page = 0;
    int unique;
    int col;
    int type[32];
    int offset[33];
    //int keylen = offset[col] sizeof索引长度
};
struct PageMeta{
    int type;//叶子，内部节点，空表
    int count;
    int prev;
    int succ;//空表当后继用
};
struct SearchIter{
    int curpid=0; // 0 means the scan has ended.
    int cursid=0;
    char* key=nullptr;
    char* r=nullptr;
    bool linclusive=1;
    bool rinclusive=1;
    ull rec_rid=0;
};
#define INTENTRY(b,sid) ((char*)(b)+((sid)*offset_inter+PAGEMETA))
#define LEAFENTRY(b,sid) ((char*)(b)+((sid)*offset_leaf+PAGEMETA))
inline bool KeyColumnIsNull(const char* key, int col)  {
    const auto* bitmap =
        reinterpret_cast<const std::uint8_t*>(key);

    return (bitmap[col >> 3] &
            (std::uint8_t(1) << (col & 7))) != 0;
}

inline bool KeyHasNull(const char* key, int col)  {
    for (int i = 0; i < col; ++i) {
        if (KeyColumnIsNull(key, i))
            return true;
    }
    return false;
}
inline void SetKeyColumnNull(char* key, int col) {
    auto* bitmap =
        reinterpret_cast<std::uint8_t*>(key);
    bitmap[col >> 3] |=
        std::uint8_t(1) << (col & 7);
}
inline void ClearKeyColumnNull(char* key, int col) {
    auto* bitmap =
        reinterpret_cast<std::uint8_t*>(key);

    bitmap[col >> 3] &=
        ~(std::uint8_t(1) << (col & 7));
}
struct BPT{
    BPTMeta meta;
    BufPageManager* bpm;
    int fid;
    int offset_inter,offset_leaf,keylen,nullen;
    int L;
    int I;
    int Lmin;
    int Imin;
    int INTCPID(BufType b,int sid){
        int pid;
        memcpy(&pid,INTENTRY(b,sid)+keylen+8,sizeof(pid));
        return pid;
    }
    ull LEAFRID(BufType b,int sid){
        ull rid;
        memcpy(&rid,LEAFENTRY(b,sid)+keylen,sizeof(rid));
        return rid;
    }
    ull INTRID(BufType b,int sid){
        ull rid;
        memcpy(&rid,INTENTRY(b,sid)+keylen,sizeof(rid));
        return rid;
    }
    int AllocPage(int type){
        if(meta.freehead == 0){
            int index;
            meta.page += 1;
            BufType b = bpm->allocPage(fid,meta.page,index,false);
            memset(b,0,PAGE_SIZE);
            b[0] = type;
            bpm->markDirty(index);
            b = bpm->getPage(fid,0,index);
            b[4] = meta.page;
            bpm->markDirty(index);
            return meta.page;
        }
        else{
            int index,res=meta.freehead;
            BufType b = bpm->getPage(fid,res, index);
            meta.freehead = b[3];
            memset(b,0,PAGE_SIZE);
            b[0]=type;
            bpm->markDirty(index);
            b = bpm->getPage(fid,0,index);
            b[3] = meta.freehead;
            bpm->markDirty(index);
            return res;
        }
    }
    void FreePage(int pid){
        int index;
        BufType b = bpm->getPage(fid,pid, index);
        b[0] = 2;
        b[3] = meta.freehead;
        bpm->markDirty(index);
        b = bpm->getPage(fid,0,index);
        b[3] = pid;
        meta.freehead = pid;
        //meta.page--;
        b[4] = meta.page;
        bpm->markDirty(index);
        return ;
    }
    #ifdef TEST_OLD
    FileManager* fm;
    BPT(char* path){
        fm = new FileManager();
        bpm = new BufPageManager(fm);
        fm->openFile(path,fid);
        int index;
        BufType metapage = bpm->getPage(fid,0,index);
        memcpy(static_cast<void*>(&meta),metapage,sizeof(meta));
        keylen = meta.offset[meta.col];
        nullen = (meta.col+7)/8;
        offset_inter = keylen+12;
        offset_leaf = keylen+8;
        #ifndef TEST
        L = (PAGE_SIZE - PAGEMETA) / (keylen + 8);
        I = (PAGE_SIZE - PAGEMETA) / (keylen + 12);
        #else
        L = TEST_L;
        I = TEST_I;
        #endif
        Lmin = (L+1)/2, Imin = (I+1)/2;
    }
    BPT(char* path,BPTMeta& meta2){
        fm = new FileManager();
        bpm = new BufPageManager(fm);
        fm->createFile(path);
        fm->openFile(path,fid);
        int index;
        BufType metapage = bpm->allocPage(fid,0,index,false);
        memset(metapage,0,PAGE_SIZE);
        memcpy(metapage,&meta2,BPTMETA);
        meta = meta2;
        bpm->markDirty(index);
        keylen = meta.offset[meta.col];
        nullen = (meta.col+7)/8;
        offset_inter = keylen+12;
        offset_leaf = keylen+8;
        #ifndef TEST
        L = (PAGE_SIZE - PAGEMETA) / (keylen + 8);
        I = (PAGE_SIZE - PAGEMETA) / (keylen + 12);
        #else
        L = TEST_L;
        I = TEST_I;
        #endif
        Lmin = (L+1)/2, Imin = (I+1)/2;
        AllocPage(0);
    }
    #endif
    BPT(int fid, BufPageManager* _bpm){
        bpm = _bpm;
        this->fid = fid;
        int index;
        BufType metapage = bpm->getPage(fid,0,index);
        memcpy(static_cast<void*>(&meta),metapage,sizeof(meta));
        keylen = meta.offset[meta.col];
        nullen = (meta.col+7)/8;
        offset_inter = keylen+12;
        offset_leaf = keylen+8;
        #ifndef TEST
        L = (PAGE_SIZE - PAGEMETA) / (keylen + 8);
        I = (PAGE_SIZE - PAGEMETA) / (keylen + 12);
        #else
        L = TEST_L;
        I = TEST_I;
        #endif
        Lmin = (L+1)/2, Imin = (I+1)/2;
    }
    BPT(int fid, BufPageManager* _bpm,BPTMeta& meta2){
        this->fid = fid;
        bpm = _bpm;
        int index;
        BufType metapage = bpm->allocPage(fid,0,index,false);
        memset(metapage,0,PAGE_SIZE);
        memcpy(metapage,&meta2,BPTMETA);
        meta = meta2;
        bpm->markDirty(index);
        keylen = meta.offset[meta.col];
        nullen = (meta.col+7)/8;
        offset_inter = keylen+12;
        offset_leaf = keylen+8;
        #ifndef TEST
        L = (PAGE_SIZE - PAGEMETA) / (keylen + 8);
        I = (PAGE_SIZE - PAGEMETA) / (keylen + 12);
        #else
        L = TEST_L;
        I = TEST_I;
        #endif
        Lmin = (L+1)/2, Imin = (I+1)/2;
        AllocPage(0);
    }
    int cmp(char* l,char* r,ull lrid,ull rrid){
        //l<r -1 = 0 > 1
        for(int i=0;i<meta.col;i++){
            bool lnull =
                static_cast<unsigned char>(l[i >> 3]) &
                (1u << (i & 7));
            bool rnull =
                static_cast<unsigned char>(r[i >> 3]) &
                (1u << (i & 7));
            if (lnull != rnull)
                return lnull ? -1 : 1;
            if (lnull) {
                continue;
            }
            //比较时NULL比所有非NULL值小
            int idx=meta.offset[i];
            if(meta.type[i]==0){//int
                int ll,rr;
                memcpy(&ll,l+idx,sizeof(ll));
                memcpy(&rr,r+idx,sizeof(rr));
                if(ll<rr)return -1;else if(ll>rr)return 1;
            }
            else if(meta.type[i]==1){//float
                double ll,rr;
                memcpy(&ll,l+idx,sizeof(ll));
                memcpy(&rr,r+idx,sizeof(rr));
                if(ll<rr)return -1;else if(ll>rr)return 1;
            }
            else{//varchar
                for(int j=idx;j<meta.offset[i+1];j++){
                    if((uchar)l[j]<(uchar)r[j])return -1;
                    else if((uchar)l[j]>(uchar)r[j])return 1;
                }
            }
        }
        if(lrid<rrid)return -1;else if(lrid>rrid)return 1;
        return 0;
    }
    int _path[128][2];//索引页pid 索引页条目idx
    bool _precise;
    int Search(char* k,ull rid){
        int pid=meta.root,cnt=0,index;
        memset(_path,0,sizeof(_path));
        _precise = false;
        while(1){
            BufType b = bpm->getPage(fid,pid,index);
            _path[cnt][0]=pid;
            if(b[0]==0){
                int l=-1,r=b[1];
                while(l+1<r){
                    int mid=(l+r)/2;
                    ull midrid = LEAFRID(b,mid);
                    int tmp = cmp(LEAFENTRY(b,mid),k,midrid,rid);
                    if(tmp==-1){
                        l = mid;
                    }else{
                        r = mid;
                        if(tmp==0)_precise = true;
                    }
                }
                _path[cnt][1]=r;
                return cnt;
            }else if(b[0]==1){
                int l=-1,r=b[1];
                while(l+1<r){
                    int mid=(l+r)/2;
                    ull midrid = INTRID(b,mid);
                    if(cmp(INTENTRY(b,mid),k,midrid,rid)==-1){
                        l = mid;
                    }else{
                        r = mid;
                    }
                }
                if(r==(int)b[1])r=(int)b[1]-1;
                _path[cnt][1]=r;
                cnt++;
                pid = INTCPID(b,r);
            }
        }
    }
    SearchIter SearchRange(char* l,char* r,bool linclusive,bool rinclusive){
        int index,tmp=Search(l,linclusive?RIDMIN:RIDMAX);
        SearchIter res{_path[tmp][0],_path[tmp][1]};
        res.linclusive = linclusive, res.rinclusive=rinclusive;
        res.r = r;
        BufType page=bpm->getPage(fid,res.curpid,index);
        if(res.cursid >= page[1]){
            res.curpid = res.cursid =0;
            res.key = nullptr;
        }else{
            res.key = LEAFENTRY(page, res.cursid);
            res.rec_rid = LEAFRID(page,res.cursid);
        }
        return res;
    }
    bool Next(SearchIter& i){
        int index;
        if(i.curpid!=0){
            BufType page=bpm->getPage(fid,i.curpid,index);
            i.cursid++;
            if(i.cursid>=(int)page[1]){
                i.curpid=page[3];
                i.cursid=0;
                if(i.curpid){
                    page = bpm->getPage(fid,i.curpid,index);
                }
                else {
                    i.key = nullptr;
                    page = nullptr;
                    i.rec_rid = 0;
                    return false;
                }
            }
            i.key = LEAFENTRY(page, i.cursid);
            i.rec_rid = LEAFRID(page,i.cursid);
            return true;
        }
        return false;
    }
    bool Valid(const SearchIter& i) {
        if (i.curpid == 0 || i.key == nullptr)
            return false;
        if (i.r == nullptr)
            return true;
        int order = cmp(i.key, i.r, 0, 0);
        return order < 0 || (order == 0 && i.rinclusive);
    }
    bool Exist(char* key){
        SearchIter iter=SearchRange(key,key,1,1);
        if(Valid(iter))return true;
        else return false;
    }
    void _updatemax(int level, char* key){
        int index;
        for(int i=level;i>=0;i--){
            BufType b = bpm->getPage(fid,_path[i][0],index);
            int sid = _path[i][1];
            memcpy(INTENTRY(b,sid),key,keylen+8);
            bpm->markDirty(index);
            if(sid != b[1]-1)break;
        }
    }
    void _updatemax(int level,char* key,ull* rid){
        int index;
        for(int i=level;i>=0;i--){
            BufType b = bpm->getPage(fid,_path[i][0],index);
            int sid = _path[i][1];
            memcpy(INTENTRY(b,sid),key,keylen);
            memcpy(INTENTRY(b,sid)+keylen,rid,8);
            bpm->markDirty(index);
            if(sid != b[1]-1)break;
        }
    }
    int InsertEntry(char* key, ull rid){//key + 表rid。返回状态
        int index,cnt;
        if(meta.unique && !KeyHasNull(key,meta.col)){
            cnt = Search(key,RIDMIN);
            int uni_pid = _path[cnt][0],uni_sid = _path[cnt][1];
            BufType b = bpm->getPage(fid,uni_pid,index);
            if(uni_sid < b[1] && cmp(LEAFENTRY(b,uni_sid),key,0,0) == 0)return -2;
        }
        else cnt=Search(key,rid);
        if(_precise)return -1;
        int pid = _path[cnt][0],sid = _path[cnt][1];
        BufType b = bpm->getPage(fid,pid,index);
        if(b[1]<L){
            b[1]++;
            memmove(LEAFENTRY(b,sid+1),LEAFENTRY(b,sid),(b[1]-sid-1)*offset_leaf);
            memcpy(LEAFENTRY(b,sid),key,keylen);
            memcpy(LEAFENTRY(b,sid)+keylen,&rid,8);
            bpm->markDirty(index);
            if(sid == b[1]-1){
                _updatemax(cnt-1,key,&rid);
            }
        }else{
            int left = (L+1)/2,right = L+1-left;
            char* page= new char[PAGE_SIZE+offset_leaf];
            char* buf_l = new char[keylen],*buf_r = new char[keylen];
            memcpy(page,(char*)b+PAGEMETA,sid*offset_leaf);
            memcpy(page+sid*offset_leaf,key,keylen);
            memcpy(page+sid*offset_leaf+keylen,&rid,8);
            memcpy(page+(sid+1)*offset_leaf,(char*)b+PAGEMETA+sid*offset_leaf,(L-sid)*offset_leaf);
            PageMeta rpgm={0,right,pid,b[3]};
            b[1]=left;
            int oldnext = b[3];
            memcpy((char*)b+PAGEMETA,page,left*offset_leaf);
            bpm->markDirty(index);
            char* leftMax = page+(left-1)*offset_leaf,*rightMax = page+L*offset_leaf;
            ull leftRid,rightRid;
            memcpy(&leftRid,page+(left-1)*offset_leaf+keylen,8);
            memcpy(&rightRid,page+L*offset_leaf+keylen,8);
            int newpid = AllocPage(0);
            b = bpm->getPage(fid,newpid,index);
            memcpy(b,&rpgm,sizeof(PageMeta));
            memcpy((char*)b+PAGEMETA,page+left*offset_leaf,right*offset_leaf);
            bpm->markDirty(index);
            b = bpm->getPage(fid,pid,index);
            b[3] = newpid;
            bpm->markDirty(index);
            if(oldnext){
                b = bpm->getPage(fid,oldnext,index);
                b[2] = newpid;
                bpm->markDirty(index);
            }
            b = bpm->getPage(fid,0,index);
            bpm->markDirty(index);
            if(b[2]==pid){b[2]=newpid;meta.ltail = newpid;bpm->markDirty(index);}
            if(b[0]==pid){
                int newroot = AllocPage(1);
                meta.root = newroot;
                b = bpm->getPage(fid,newroot,index);
                b[1]=2;
                memcpy((char*)b+PAGEMETA,leftMax,keylen);
                memcpy((char*)b+PAGEMETA+keylen,&leftRid,8);
                memcpy((char*)b+PAGEMETA+keylen+8,&pid,4);
                memcpy((char*)b+PAGEMETA+offset_inter,rightMax,keylen);
                memcpy((char*)b+PAGEMETA+keylen+offset_inter,&rightRid,8);
                memcpy((char*)b+PAGEMETA+keylen+8+offset_inter,&newpid,4);
                bpm->markDirty(index);
                b = bpm->getPage(fid,0,index);
                b[0] = newroot;
                bpm->markDirty(index);
            }
            for(int i=cnt-1;i>=0;i--){
                pid = _path[i][0];
                b = bpm->getPage(fid,pid,index);
                sid = _path[i][1];
                if(b[1]<I){
                    if(sid<b[1]-1)memmove(INTENTRY(b,sid+2),INTENTRY(b,sid+1),(b[1]-sid-1)*offset_inter);
                    memcpy(INTENTRY(b,sid),leftMax,keylen);
                    memcpy(INTENTRY(b,sid)+keylen,&leftRid,8);
                    memcpy(INTENTRY(b,sid)+offset_inter,rightMax,keylen);
                    memcpy(INTENTRY(b,sid)+keylen+offset_inter,&rightRid,8);
                    memcpy(INTENTRY(b,sid)+keylen+offset_inter+8,&newpid,4);
                    b[1]++;
                    bpm->markDirty(index);
                    if(sid == b[1]-2){
                        _updatemax(i-1,rightMax,&rightRid);
                    }
                    break;
                }else{
                    left = (I+1)/2,right = I+1-left;
                    memcpy(buf_l,leftMax,keylen);
                    memcpy(buf_r,rightMax,keylen);
                    memcpy(page,(char*)b+PAGEMETA,sid*offset_inter);
                    memcpy(page+(sid)*offset_inter,buf_l,keylen);
                    memcpy(page+(sid)*offset_inter+keylen,&leftRid,8);
                    memcpy(page+(sid)*offset_inter+keylen+8,&_path[i+1][0],4);
                    memcpy(page+(sid+1)*offset_inter,buf_r,keylen);
                    memcpy(page+(sid+1)*offset_inter+keylen,&rightRid,8);
                    memcpy(page+(sid+1)*offset_inter+keylen+8,&newpid,4);
                    memcpy(page+(sid+2)*offset_inter,(char*)b+PAGEMETA+(sid+1)*offset_inter,(I-sid-1)*offset_inter);
                    PageMeta rpgm={1,right,0,0};
                    b[1]=left;
                    memcpy((char*)b+PAGEMETA,page,left*offset_inter);
                    bpm->markDirty(index);
                    leftMax = page+(left-1)*offset_inter;
                    rightMax = page+I*offset_inter;
                    memcpy(&leftRid,page+(left-1)*offset_inter+keylen,8);
                    memcpy(&rightRid,page+I*offset_inter+keylen,8);
                    newpid = AllocPage(1);
                    b = bpm->getPage(fid,newpid,index);
                    memcpy(b,&rpgm,sizeof(PageMeta));
                    memcpy((char*)b+PAGEMETA,page+left*offset_inter,right*offset_inter);
                    bpm->markDirty(index);
                    b = bpm->getPage(fid,0,index);
                    bpm->markDirty(index);
                    if(b[0]==pid){
                        int newroot = AllocPage(1);
                        meta.root = newroot;
                        b = bpm->getPage(fid,newroot,index);
                        b[1]=2;
                        memcpy((char*)b+PAGEMETA,leftMax,keylen);
                        memcpy((char*)b+PAGEMETA+keylen,&leftRid,8);
                        memcpy((char*)b+PAGEMETA+keylen+8,&pid,4);
                        memcpy((char*)b+PAGEMETA+offset_inter,rightMax,keylen);
                        memcpy((char*)b+PAGEMETA+keylen+offset_inter,&rightRid,8);
                        memcpy((char*)b+PAGEMETA+keylen+8+offset_inter,&newpid,4);
                        bpm->markDirty(index);
                        b = bpm->getPage(fid,0,index);
                        b[0] = newroot;
                        bpm->markDirty(index);
                        break;
                    }
                }
            }
            delete[] page;
            delete[] buf_l;
            delete[] buf_r;
        }
        return 0;
    }
    //void inline _rebalance(int level, char* buf);
    int DeleteEntry(char* key,ull rid){
        int index,cnt = Search(key,rid);
        if(!_precise)return -1;
        int pid = _path[cnt][0],sid = _path[cnt][1];
        BufType b = bpm->getPage(fid,0,index);
        if(b[0]==b[1]&&b[0]==b[2]){
            BufType b = bpm->getPage(fid,pid,index);
            memmove(LEAFENTRY(b,sid),LEAFENTRY(b,sid)+offset_leaf,(b[1]-sid-1)*offset_leaf);
            b[1]--;
            bpm->markDirty(index);
            return 0;
        }
        b = bpm->getPage(fid,pid,index);
        if(b[1]>Lmin){//不需要继续向上
            memmove(LEAFENTRY(b,sid),LEAFENTRY(b,sid)+offset_leaf,(b[1]-sid-1)*offset_leaf);
            b[1]--;
            bpm->markDirty(index);
            if(sid == b[1]){
                char* newmax = new char[offset_leaf];
                memcpy(newmax,LEAFENTRY(b,sid-1),offset_leaf);
                _updatemax(cnt-1,newmax);
                delete[] newmax;
            }
            return 0;
        }else{
            if(_path[cnt-1][1]>0){//左借
                b = bpm->getPage(fid,_path[cnt-1][0],index);
                int lsibpid = INTCPID(b,_path[cnt-1][1]-1);
                b = bpm->getPage(fid,lsibpid,index);
                if(b[1]>Lmin){//能借
                    char* buf = new char[offset_inter];
                    char *buf2 = new char[offset_inter];
                    memcpy(buf,LEAFENTRY(b,b[1]-1),keylen+8);
                    memcpy(buf2,LEAFENTRY(b,b[1]-2),keylen+8);
                    b[1]--;
                    bpm->markDirty(index);
                    b = bpm->getPage(fid,pid,index);
                    memmove(LEAFENTRY(b,sid),LEAFENTRY(b,sid)+offset_leaf,(b[1]-sid-1)*offset_leaf);
                    b[1]--;
                    memmove(LEAFENTRY(b,1),LEAFENTRY(b,0),b[1]*offset_leaf);
                    memcpy(LEAFENTRY(b,0),buf,offset_leaf);
                    b[1]++;
                    bpm->markDirty(index);
                    b = bpm->getPage(fid,_path[cnt-1][0],index);
                    memcpy(INTENTRY(b,_path[cnt-1][1]-1),buf2,keylen+8);
                    bpm->markDirty(index);
                    b = bpm->getPage(fid,_path[cnt][0],index);
                    if(sid == b[1]-1){
                        memcpy(buf,LEAFENTRY(b,b[1]-1),offset_leaf);
                        _updatemax(cnt-1,buf);
                    }
                    delete[] buf;
                    delete[] buf2;
                    return 0;
                }
            }
            b = bpm->getPage(fid,_path[cnt-1][0],index);
            if(_path[cnt-1][1]+1<b[1]){//右借
                int rsibpid = INTCPID(b,_path[cnt-1][1]+1);
                b = bpm->getPage(fid,rsibpid,index);
                if(b[1]>Lmin){
                    char* buf = new char[offset_inter];
                    memcpy(buf,LEAFENTRY(b,0),keylen+8);
                    memmove(LEAFENTRY(b,0),LEAFENTRY(b,1),(b[1]-1)*offset_leaf);
                    b[1]--;
                    bpm->markDirty(index);
                    b = bpm->getPage(fid,pid,index);
                    memmove(LEAFENTRY(b,sid),LEAFENTRY(b,sid)+offset_leaf,(b[1]-sid-1)*offset_leaf);
                    b[1]--;
                    memcpy(LEAFENTRY(b,b[1]),buf,offset_leaf);
                    b[1]++;
                    bpm->markDirty(index);
                    b = bpm->getPage(fid,_path[cnt-1][0],index);
                    memcpy(INTENTRY(b,_path[cnt-1][1]),buf,keylen+8);
                    bpm->markDirty(index);
                    delete[] buf;
                    return 0;
                }
            }
            char* buf = new char[PAGE_SIZE];
            if(_path[cnt-1][1]>0){
                b = bpm->getPage(fid,_path[cnt][0],index);
                int cur_succ = b[3], cur_count = b[1];
                memcpy(buf,(char*)b+PAGEMETA,sid*offset_leaf);
                memcpy(buf+sid*offset_leaf,(char*)b+(sid+1)*offset_leaf+PAGEMETA,(cur_count-sid-1)*offset_leaf);
                b = bpm->getPage(fid,_path[cnt-1][0],index);
                int lsibpid = INTCPID(b,_path[cnt-1][1]-1);
                memcpy(INTENTRY(b,_path[cnt-1][1]-1),buf+(cur_count-2)*offset_leaf,keylen+8);
                if(_path[cnt-1][1]<b[1]-1)memmove(INTENTRY(b,_path[cnt-1][1]),INTENTRY(b,_path[cnt-1][1]+1),(b[1]-_path[cnt-1][1]-1)*offset_inter);
                b[1]--;
                bpm->markDirty(index);
                b = bpm->getPage(fid,lsibpid,index);
                b[3] = cur_succ;
                memcpy(LEAFENTRY(b,b[1]),buf,(cur_count-1)*offset_leaf);
                b[1] += (cur_count - 1);
                bpm->markDirty(index);
                FreePage(pid);
                b = bpm->getPage(fid,0,index);
                if(b[2]==pid){
                    meta.ltail = b[2] = lsibpid;
                }
                bpm->markDirty(index);
                if(cur_succ){
                    BufType b = bpm->getPage(fid,cur_succ,index);
                    b[2] = lsibpid;
                    bpm->markDirty(index);
                }
            }
            else{
                b = bpm->getPage(fid,_path[cnt-1][0],index);
                int rsibpid = INTCPID(b,_path[cnt-1][1]+1);
                b = bpm->getPage(fid,rsibpid,index);
                int right_succ = b[3], right_count = b[1];
                memcpy(buf,(char*)b+PAGEMETA,right_count*offset_leaf);
                b = bpm->getPage(fid,_path[cnt][0],index);
                memmove(LEAFENTRY(b,sid),LEAFENTRY(b,sid+1),(b[1]-sid-1)*offset_leaf);
                memcpy(LEAFENTRY(b,b[1]-1),buf,right_count*offset_leaf);
                b[1] = b[1] - 1 + right_count;
                b[3] = right_succ;
                bpm->markDirty(index);
                b = bpm->getPage(fid,_path[cnt-1][0],index);
                memcpy(INTENTRY(b,_path[cnt-1][1]),buf+(right_count-1)*offset_leaf,keylen+8);
                if(_path[cnt-1][1]<b[1]-2)memmove(INTENTRY(b,_path[cnt-1][1]+1),INTENTRY(b,_path[cnt-1][1]+2),(b[1]-_path[cnt-1][1]-2)*offset_inter);
                b[1] --;
                bpm->markDirty(index);
                FreePage(rsibpid);
                b = bpm->getPage(fid,0,index);
                if(b[2]==rsibpid){
                    meta.ltail = b[2] = pid;
                }
                bpm->markDirty(index);
                if(right_succ){
                    BufType b = bpm->getPage(fid,right_succ,index);
                    b[2] = pid;
                    bpm->markDirty(index);
                }
            }
            _rebalance(cnt-1,buf);
            delete[] buf;
            return 0;
        }
    }
    void inline _rebalance(int level, char* buf){
        for(int i=level;i>=0;i--){
            int pid = _path[i][0],sid = _path[i][1];
            int index;
            BufType b;
            b = bpm->getPage(fid,pid,index);
            if(b[1] >= Imin){
                memcpy(buf,INTENTRY(b,b[1]-1),keylen+8);
                _updatemax(i-1,buf);
                return ;
            }
            else{
                if(i==0){
                    if(b[1] == 1){
                        int cpid = INTCPID(b,0);
                        FreePage(pid);
                        b = bpm->getPage(fid,0,index);
                        b[0] = meta.root = cpid;
                    }
                    return ;
                }
                if(_path[i-1][1]>0){//左借
                    b = bpm->getPage(fid,_path[i-1][0],index);
                    int lsibpid = INTCPID(b,_path[i-1][1]-1);
                    b = bpm->getPage(fid,lsibpid,index);
                    if(b[1]>Imin){
                        memcpy(buf,INTENTRY(b,b[1]-1),offset_inter);
                        memcpy(buf+offset_inter,INTENTRY(b,b[1]-2),offset_inter);
                        b[1]--;
                        bpm->markDirty(index);
                        b = bpm->getPage(fid,pid,index);
                        memmove(INTENTRY(b,1),INTENTRY(b,0),b[1]*offset_inter);
                        memcpy(INTENTRY(b,0),buf,offset_inter);
                        b[1]++;
                        memcpy(buf+2*offset_inter,INTENTRY(b,b[1]-1),offset_inter);
                        bpm->markDirty(index);
                        b = bpm->getPage(fid,_path[i-1][0],index);
                        memcpy(INTENTRY(b,_path[i-1][1]-1),buf+offset_inter,keylen+8);
                        bpm->markDirty(index);
                        _updatemax(i-1,buf+2*offset_inter);
                        return ;
                    }
                }
                b = bpm->getPage(fid,_path[i-1][0],index);
                if(_path[i-1][1]+1<b[1]){
                    int rsibpid = INTCPID(b,_path[i-1][1]+1);
                    b = bpm->getPage(fid,rsibpid,index);
                    if(b[1]>Imin){
                        memcpy(buf,INTENTRY(b,0),offset_inter);
                        memmove(INTENTRY(b,0),INTENTRY(b,1),(b[1]-1)*offset_inter);
                        b[1]--;
                        bpm->markDirty(index);
                        b = bpm->getPage(fid,pid,index);
                        memcpy(INTENTRY(b,b[1]),buf,offset_inter);
                        b[1]++;
                        bpm->markDirty(index);
                        b = bpm->getPage(fid,_path[i-1][0],index);
                        memcpy(INTENTRY(b,_path[i-1][1]),buf,keylen+8);
                        bpm->markDirty(index);
                        return ;
                    }
                }
                if(_path[i-1][1]>0){
                    b = bpm->getPage(fid,_path[i][0],index);
                    int cur_count = b[1];
                    memcpy(buf,(char*)b+PAGEMETA,cur_count*offset_inter);
                    b = bpm->getPage(fid,_path[i-1][0],index);
                    int lsibpid = INTCPID(b,_path[i-1][1]-1);
                    memcpy(INTENTRY(b,_path[i-1][1]-1),buf+(cur_count-1)*offset_inter,keylen+8);
                    if(_path[i-1][1]<b[1]-1)memmove(INTENTRY(b,_path[i-1][1]),INTENTRY(b,_path[i-1][1]+1),(b[1]-_path[i-1][1]-1)*offset_inter);
                    b[1]--;
                    bpm->markDirty(index);
                    b = bpm->getPage(fid,lsibpid,index);
                    memcpy(INTENTRY(b,b[1]),buf,cur_count*offset_inter);
                    b[1] += (cur_count);
                    bpm->markDirty(index);
                    FreePage(pid);
                }
                else{
                    b = bpm->getPage(fid,_path[i-1][0],index);
                    int rsibpid = INTCPID(b,_path[i-1][1]+1);
                    b = bpm->getPage(fid,rsibpid,index);
                    int right_count = b[1];
                    memcpy(buf,(char*)b+PAGEMETA,right_count*offset_inter);
                    b = bpm->getPage(fid,_path[i][0],index);
                    memcpy(INTENTRY(b,b[1]),buf,right_count*offset_inter);
                    b[1] = b[1] + right_count;
                    bpm->markDirty(index);
                    b = bpm->getPage(fid,_path[i-1][0],index);
                    memcpy(INTENTRY(b,_path[i-1][1]),buf+(right_count-1)*offset_inter,keylen+8);
                    if(_path[i-1][1]<b[1]-2)memmove(INTENTRY(b,_path[i-1][1]+1),INTENTRY(b,_path[i-1][1]+2),(b[1]-_path[i-1][1]-2)*offset_inter);
                    b[1] --;
                    bpm->markDirty(index);
                    FreePage(rsibpid);
                }
            }
        }
    }
};
