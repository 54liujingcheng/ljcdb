#include "system_management.h"
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