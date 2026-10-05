#include "bufmanager/BufPageManager.h"
#include "fileio/FileManager.h"
#include "utils/pagedef.h"
#include <cstring>
#include <cmath>
#include <cstdint>
#include <iostream>
#pragma once
#define uchar unsigned char
#define ull unsigned long long
#define PID(x) ((int)(x>>32))
#define SID(x) ((int)(x&0xFFFFFFFF))
#define RID(x,y) (((ull)(x)<<32)|((ull)(y)))
#define MAX_STR 32
/*
数据页元数据
槽容量 已使用槽数量 后继页
槽位图
记录长度和schema从TableMeta继承
*/
const int DPOFFSET = 12;
struct DataPageMeta{
    int slotcapa;
    int slotused;
    int succ;
    uchar* slotmap;
};
/*
第0张表记录表的元数据
记录长度 列数 NULL位图长度 数据页页数 空闲链表头

列描述结构体
列 列名 类型 vchar最大长度 能否null 有无defalut和defalut值

schema
列 类型 偏移量
*/
struct Col{
    char name[MAX_STR];
    int type;//0:INT 1:FLOAT 2:VARCHAR
    int len;
    int nullable;
    int defalutable;
    int default_offset;//0 -> null
    //int default_len;
};
struct Schema{
    int recordsize;
    int nullen;
    int col;
    int offset[33];//offset[col]==recordsize
    Col desc[32];
    char defval[2048];
};
struct TableMeta{
    int head;
    int page;
    Schema schema;
};
struct RecordManager{
    int fid;
    int slotbit;
    int offset;
    int slotcapa;
    BufPageManager* bpm;
    TableMeta tbmeta;
    #ifdef TEST_OLD
    FileManager* fm;
    RecordManager(char* path, Schema& sch){
        fm = new FileManager();
        bpm = new BufPageManager(fm);
        fm->createFile(path);
        fm->openFile(path,fid);
        int index;
        BufType tbmetapage = bpm->allocPage(fid,0,index,false);
        tbmeta.head = tbmeta.page = 0;
        memcpy(&tbmeta.schema,&sch,sizeof(Schema));
        slotcapa = 8 * (PAGE_SIZE - 1 - DPOFFSET) / (9*tbmeta.schema.recordsize);
        slotbit = ceil(slotcapa / 8.0);
        offset = DPOFFSET + slotbit;
        memcpy(tbmetapage,&tbmeta,sizeof(TableMeta));
        bpm->markDirty(index);
    }
    RecordManager(char* path){
        fm = new FileManager();
        bpm = new BufPageManager(fm);
        fm->openFile(path,fid);
        int index;
        BufType metapage = bpm->getPage(fid,0,index);
        bpm->access(index);
        memcpy(&tbmeta, metapage, sizeof(TableMeta));
        slotcapa = 8 * (PAGE_SIZE - 1 - DPOFFSET) / (9*tbmeta.schema.recordsize);
        slotbit = ceil(slotcapa / 8.0);
        offset = DPOFFSET + slotbit;
    }
    #endif
    RecordManager(int fid, BufPageManager* _bpm){
        bpm = _bpm;
        this->fid = fid;
        int index;
        BufType metapage = bpm->getPage(fid,0,index);
        bpm->access(index);
        memcpy(&tbmeta, metapage, sizeof(TableMeta));
        slotcapa = 8 * (PAGE_SIZE - 1 - DPOFFSET) / (9*tbmeta.schema.recordsize);
        slotbit = ceil(slotcapa / 8.0);
        offset = DPOFFSET + slotbit;
    }
    RecordManager(int fid, BufPageManager* _bpm, Schema& sch){
        this->fid = fid;
        bpm = _bpm;
        int index;
        BufType tbmetapage = bpm->allocPage(fid,0,index,false);
        tbmeta.head = tbmeta.page = 0;
        memcpy(&tbmeta.schema,&sch,sizeof(Schema));
        slotcapa = 8 * (PAGE_SIZE - 1 - DPOFFSET) / (9*tbmeta.schema.recordsize);
        slotbit = ceil(slotcapa / 8.0);
        offset = DPOFFSET + slotbit;
        memcpy(tbmetapage,&tbmeta,sizeof(TableMeta));
        bpm->markDirty(index);
    }
    void LoadRecord(ull rid, char* dest){
        int pid=PID(rid),sid=SID(rid);
        int index, recordsize = tbmeta.schema.recordsize;
        BufType datapage = bpm->getPage(fid,pid,index);
        bpm->access(index);
        memcpy(dest, ((char*)datapage) + offset + recordsize * sid, recordsize);
    }
    /*
    只有当没有空闲页才可以扩容
    */
    int NewPage(BufType& b){
        int index;
        BufType tbmetapage = bpm->getPage(fid,0,index);
        int pid = tbmeta.page + 1;
        tbmeta.page = pid;
        tbmeta.head = pid;
        tbmetapage[1] = pid;//page
        tbmetapage[0] = pid;//head
        bpm->markDirty(index);
        b = bpm->allocPage(fid, pid, index, false);
        memset(b,0,PAGE_SIZE);
        b[0] = slotcapa;//slotcapa
        bpm->markDirty(index);
        return pid;
    }
    ull InsertRecord(char* src){
        int index, recordsize = tbmeta.schema.recordsize;
        int pid,sid;
        BufType b;
        if(tbmeta.head == 0){
            pid = NewPage(b);
            sid = 0;
            b[1] = 1;//slotused
            ((char*)b)[12]=0b1;//slotmap
            memcpy(((char*)b)+offset,src,recordsize);
        }
        else{
            pid = tbmeta.head;
            b = bpm->getPage(fid,pid,index);
            b[1] += 1;//slotused
            uchar* bitmap = ((uchar*)b)+12;
            sid=0;
            for(int i=0;i<slotbit;i++){
                if(bitmap[i]==0b11111111){
                    sid += 8;
                    continue;
                }
                else{
                    int tmp=__builtin_ctz(~bitmap[i]);
                    sid += tmp;
                    bitmap[i] |= (1<<tmp);
                    break;
                }
            }
            memcpy(((char*)b)+offset+sid*recordsize, src, recordsize);
            bpm->markDirty(index);
            if(b[1]==b[0]){
                tbmeta.head = b[2];
                BufType tbmetapage = bpm->getPage(fid,0,index);
                tbmetapage[0] = tbmeta.head;//head
                bpm->markDirty(index);
            }
        }
        return RID(pid,sid);
    }
    void EraseRecord(ull rid){
        int index,pid = PID(rid), sid = SID(rid);
        BufType b = bpm->getPage(fid,pid,index);
        uchar* bitmap = ((uchar*)b)+12;
        if((bitmap[sid>>3] & (1<<(sid&0b111))) == 0)return ;
        b[1] -= 1;
        bitmap[sid>>3] &= (~(1<<(sid&0b111)));
        bpm->markDirty(index);
        if(b[1] == b[0] - 1){
            BufType tbmetapage = bpm->getPage(fid,0,index);
            b[2] = tbmeta.head;
            tbmeta.head = pid;
            tbmetapage[0] = pid;//head
            bpm->markDirty(index);
        }
        return ;
    }
    void UpdataRecord(ull rid, char* src){
        int index, recordsize = tbmeta.schema.recordsize;
        int pid = PID(rid), sid = SID(rid);
        BufType b = bpm->getPage(fid,pid,index);
        uchar* bitmap = ((uchar*)b)+12;
        if((bitmap[sid>>3] & (1<<(sid&0b111))) == 0)return ;
        memcpy(((char*)b)+offset+sid*recordsize,src,recordsize);
        bpm->markDirty(index);
        return ;
    }
    ull FirstRecord(){
        for(int pid=1;pid<=tbmeta.page;pid++){
            int index;
            BufType b = bpm->getPage(fid,pid,index);
            uchar* bitmap = ((uchar*)b)+12;
            for(int sid=0;sid<slotcapa;sid++){
                if((bitmap[sid>>3] & (1<<(sid&0b111))) != 0){
                    return RID(pid,sid);
                }
            }
        }
        return RID(0,0);
    }
    ull NextRecord(ull rid){
        int index,pid = PID(rid);
        BufType b = bpm->getPage(fid,pid,index);
        uchar* bitmap = ((uchar*)b)+12;
        for(int sid=SID(rid)+1;sid<slotcapa;sid++){
            if((bitmap[sid>>3] & (1<<(sid&0b111))) != 0){
                return RID(pid,sid);
            }
        }
        for(int pid=PID(rid)+1;pid<=tbmeta.page;pid++){
            BufType b = bpm->getPage(fid,pid,index);
            uchar* bitmap = ((uchar*)b)+12;
            for(int sid=0;sid<slotcapa;sid++){
                if((bitmap[sid>>3] & (1<<(sid&0b111))) != 0){
                    return RID(pid,sid);
                }
            }
        }
        return RID(0,0);
    }
    bool isValid(ull rid){
        int index,pid = PID(rid);int sid = SID(rid);
        if (pid < 1 || pid > tbmeta.page)
            return false;
        if (sid < 0 || sid >= slotcapa)
            return false;
        BufType b = bpm->getPage(fid,pid,index);
        uchar* bitmap = ((uchar*)b)+12;
        if((bitmap[sid>>3] & (1<<(sid&0b111))) != 0){
                return 1;
        }
        else return 0;
    }
};
