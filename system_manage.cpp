#include "system_manage.h"

using namespace std;

void SM_PrintError(RC rc){

}

void PutIntData(Byte *& pData, const int x){
  memcpy(pData, &x, 4);
  pData+=4;
}

void PutFloatData(Byte *& pData, const float x){
  memcpy(pData, &x, 4);
  pData+=4;
}

void PutStringData(Byte *& pData, const char *s, const int len){
  int s_len=strlen(s);
  for(int i=0; i<len; i++){
    if (i>=s_len)
      *pData=0;
    else
      *pData=s[i];
    pData++;
  }
}

//----------------------------SM_MANAGER----------------------------------

SM_Manager::SM_Manager(RM_Manager &rmm2)
  :rmm(rmm2){
  
}

SM_Manager::~SM_Manager(){

}

RC SM_Manager::CreateDB(const string &db_name){
  string db_dir(db_name+SYS_SEP);
  rmm.CreateFile( (db_dir+SYSTEM_CATALOG_NAME).c_str(),  SYSTEM_TUPLE_LENGTH);
  rmm.CreateFile( (db_dir+ATTRIBUTE_CATALOG_NAME).c_str(), ATTRIBUTE_TUPLE_LENGTH);
  return OK;
}

RC SM_Manager::OpenDb(const char *db_name){
  db_dir=string(db_name)+SYS_SEP;

  RC ret=rmm.OpenFile(db_dir+SYSTEM_CATALOG_NAME, system_fh);
  if (ret!=OK){
    SM_PrintError(ret);
    return SYSTEM_CATALOG_OPEN_ERROR;
  }

  ret=rmm.OpenFile(db_dir+ATTRIBUTE_CATALOG_NAME, attribute_fh);
  if (ret!=OK){
    SM_PrintError(ret);
    return ATTRIBUTE_CATALOG_OPEN_ERROR;
  }
  
  return OK;
}

RC SM_Manager::CloseDb(){
  
  RC ret=rmm.CloseFile(system_fh);
  if (ret!=OK){
    SM_PrintError(ret);
    return SYSTEM_CATALOG_CLOSE_ERROR;
  }

  ret=rmm.CloseFile(attribute_fh);
  if (ret!=OK){
    SM_PrintError(ret);
    return ATTRIBUTE_CATALOG_CLOSE_ERROR;
  }

  return OK;
}

RC SM_Manager::CreateTable(const char *rel_name, int attr_count, AttrInfo *attr_info){
  
  //for system catalog
  int tuple_length=0;
  int index_count=0;
  for(int i=0; i<attr_count; i++){
    tuple_length += attr_info[i].attrLength;
    if (!attr_info[i].notNull)
      tuple_length++;
  }

  Byte pData[500];
  Byte *nowp=pData;
  PutStringData(nowp, rel_name, RELNAME_LENGTH);
  PutIntData(nowp, tuple_length);
  PutIntData(nowp, attr_count);
  PutIntData(nowp, index_count);

  RID rid;
  system_fh.InsertRec(pData, rid);
  //------------------- 


  //for attribute catalog
  for(int i=0, now_offset=0; i<attr_count; i++){
    RID rid;
    nowp=pData;
    PutStringData(nowp, rel_name, RELNAME_LENGTH);
    PutStringData(nowp, attr_info[i].attrName.c_str(), ATTRNAME_LENGTH);

    PutIntData(nowp, now_offset);
    now_offset+=attr_info[i].attrLength;
    if (!attr_info[i].notNull)
      now_offset++;

    PutIntData(nowp, (int)attr_info[i].attrType);
    PutIntData(nowp, attr_info[i].attrLength);
    PutIntData(nowp, -1); //for index
    PutIntData(nowp, attr_info[i].notNull);
    attribute_fh.InsertRec(pData, rid);
  }
  //-------------------

  //for table file
  RC ret=rmm.CreateFile(rel_name, tuple_length);
  if (ret!=OK){
    return TABLE_CREATE_ERROR;
  }
  //------------------

  return OK;
}

RC SM_Manager::DropTable(const char *relName){
  string rel_name(relName);
  
  //drop in system catalog
  RM_FileScan system_scanner(system_fh, STRING, RELNAME_LENGTH, 0, EQ_OP, 
                             (void*)relName, strlen(relName));
  RC ret;
  RM_Record record;
  ret=system_scanner.GetNextRec(record);
  if (ret==NOT_FOUND){
    return NOT_FOUND;
  }
  RID rid;
  record.GetRid(rid);
  system_fh.DeleteRec(rid);
  //------------------------

  //drop in attribute catalog
  RM_FileScan attribute_scanner(attribute_fh, STRING, RELNAME_LENGTH, 0, EQ_OP, 
                                (void*)relName, strlen(relName));
  while( (ret=attribute_scanner.GetNextRec(record)) != NOT_FOUND ){
    record.GetRid(rid);
    attribute_fh.DeleteRec(rid);
  }
  //-----------------------
  
  return OK;
}

RC SM_Manager::Help(){
  return OK;
}

RC SM_Manager::Help(const char *rel_name){
  return OK;
}

RC SM_Manager::Print(const char *rel_name){
  return OK;
}

//----------------------------SM_MANAGER----------------------------------
