#include "record_manager.h"

//-------------------------------[RID]-----------------------------------

RID::RID(){
  this->page_number=-1;
  this->slot_number=-1;
}

RID::RID(int page_number, int slot_number){
  this->page_number=page_number;
  this->slot_number=slot_number;
}

RID::~RID(){

}

RC RID::GetPageNum(int &page_number)const{
  page_number=this->page_number;
  if (page_number<0)
    return NO_PAGE_IN_RID;
  return OK;
}

RC RID::GetSlotNum(int &slot_number)const{
  slot_number=this->slot_number;
  if (slot_number<0)
    return NO_SLOT_IN_RID;
  return OK;
}

//-------------------------------[RID]-----------------------------------

//-------------------------------[RM_RECORD]-----------------------------------

RM_Record::RM_Record(){

}

RM_Record::RM_Record(char *pData, const RID &rid){
  this->mem=pData;
  this->rid=rid;
}

RM_Record::~RM_Record(){

}

RC RM_Record::GetData(char *&pData)const{
  pData=this->mem;
  return OK;
}

RC RM_Record::GetRid(RID &rid)const{
  rid=this->rid;
  if (rid.page_number<0 || rid.slot_number<0)
    return NO_RID_IN_RECORD;
  return OK;
}

//-------------------------------[RM_RECORD]-----------------------------------


//-------------------------------[RM_FILEHANDLE]-----------------------------------

RM_FileHandle::RM_FileHandle(const int record_size, const int max_record_on_page, const int total_record,
                             int *avail_spaces, const PF_FileHandle &pf_file_handle){
  this->record_size=record_size;
  this->max_record_on_page=max_record_on_page;
  this->total_record=total_record;
  this->avail_spaces=avail_spaces;
  this->pf_file_handle=pf_file_handle;
}

RM_FileHandle::~RM_FileHandle(){
  delete avail_spaces;
}

RC RM_FileHandle::_ExpandPages(){
  int current_total_page=pf_file_handle.GetTotalPage();
  PF_PageHandle pf_page_handle;
  for(int i=0; i<current_total_page; i++){
    RC ret=pf_file_handle.AllocatePage(pf_page_handle);
    if (ret!=OK)
      return ret;
  }
  return OK;
}

RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec)const{
  int page_number=rid.GetPageNum();
  int slot_number=rid.GetSlotNum();
  
  PF_PageHandle page_handle;
  RC ret=pf_file_handle.GetThisPage(page_number, page_handle);
  if (ret!=OK)
    return ret;
  
  Byte *mem;
  ret=page_handle.GetData(mem);
  
  mem+=96+record_size*slot_number;
  
  rec=RM_Record(mem, record_size);

  return OK;
}

RC RM_FileHandle::UpdateRec(const RM_Record &rec){
  RID rid;
  RC ret=rec.GetRid(rid);
  
  int page_number=rid.GetPageNum();
  int slot_number=rid.GetSlotNum();
  
  PF_PageHandle page_handle;
  ret=pf_file_handle.GetThisPage(page_number, page_handle);
  if (ret!=OK)
    return ret;
  
  Byte *mem;
  ret=page_handle.GetData(mem);
  mem+=96+record_size*slot_number;
  
  Byte *new_mem;
  ret=rec.GetData(new_mem);

  memcpy(mem, new_mem, record_size);
  pf_file_handle.MarkDirty(page_number);
  
  return OK;
}

RC RM_FileHandle::ForcePages(int page_number)const{
  return pf_file_handle.ForcePages(page_number);
}

RC RM_FileHandle::InsertRec(const char *pData, RID &rid){
  int page_number=-1;
  for(int i=0; i<pf_file_handle.GetTotalPage(); i++)
    if (avail_spaces[i]!=0){
      page_number=i;
      break;
    }
  if (page_number==-1){
    page_number=(pf_file_handle.GetTotalPage());
    _ExpandPages();
  }
  
  PF_PageHandle pf_page_handle;
  RC ret=pf_file_handle.GetThisPage(page_number, pf_page_handle);
  if (ret!=OK)
    return ret;

  Byte *mem;
  ret=pf_page_handle.GetData(mem);
  pf_file_handle.MarkDirty(page_number);

  int slot=-1;
  Byte *p=mem+(8<<10);
  for(int i=0; i<=max_record_on_page; ){
    p--;
    int di=-1;
    for(int j=0; j<8; j++)
      if ( (((*p)>>j)&1)==0 ){
        di=j;
        break;
      }
    if (di!=-1){
      slot=i+di;
      memcpy(mem+96+slot*record_size, pData, record_size);
      int np=(*p)+(1<<di);
      memcpy(p, &np, 1);
      break;
    }else{
      i+=8;
    }
  }

  total_record++;
  avail_spaces[page_number]--;
  
  rid=RID(page_number, slot);
  return OK;
}

RC RM_FileHandle::DeleteRec(const RID &rid){
  int page_number=rid.GetPageNum();
  int slot_number=rid.GetSlotNum();
  
  PF_PageHandle pf_page_handle;
  RC ret=pf_file_handle.GetThisPage(page_number, pf_page_handle);
  if (ret!=OK)
    return ret;
  Byte *mem;
  ret=pf_page_handle.GetData(mem);
  pf_file_handle.MarkDirty(page_number);

  Byte *p=mem+(8<<10);
  for(int i=0; ; ){
    p--;
    if (slot>=i && slot<i+8){
      int np=p+(1<<(slot-i));
      memcpy(p, &np, 1);
      break;
    }else
      i+=8;
  }

  total_record++;
  avail_spaces[page_number]--;
  
  rid=RID(page_number, slot);
  
  return OK;
}

//-------------------------------[RM_FILEHANDLE]-----------------------------------


//-------------------------------[RM_MANAGER]-----------------------------------

RM_MANAGER::RM_MANAGER(PF_Manager &pfm2)
  :pfm(pfm2){

}

RM_MANAGER::~RM_MANAGER(){

}

RC RM_MANAGER::CreateFile(const char *fileName, int record_size){
  if (record_size> (8<<10)-96-10)
    return TOO_LONG_RECORD;
  RC ret=pfm.CreateFile(fileName);
  if (ret!=OK)
    return ret;
  PF_PageHandle pf_file_handle;
  pfm.OpenFile(fileName, pf_file_handle);

  //allocate header page
  PF_PageHandle pf_page_handle;
  ret=pf_file_handle.AllocatePage(pf_page_handle);
  
  //Get data address of the header page
  Byte *mem;
  ret=pf_page_handle.GetData(mem);
  
  //write record_size, max_record_on_page total_record, record_number[] 
  memcpy(mem, &record_size, sizeof(record_size));
  mem+=sizeof(record_size);

  int max_record_on_page=((8<<10)-96-10)/record_size;
  memcpy(mem, &max_record_on_page, sizeof(max_record_on_page));
  mem+=sizeof(max_record_on_page);

  int total_record=0;
  memcpy(mem, &total_record, sizeof(total_record));
  mem+=sizeof(total_record);

  //... do not need to write 

  pf_file_handle.MarkDirty(0);
  pfm.CloseFile(pf_file_handle);
}

RC RM_MANAGER::DestroyFile(const char *fileName){
  //to be constructed
  return OK;
}

RC RM_MANAGER::OpenFile(const char *fileName, RM_FileHandle &rm_file_handle){
  PF_FileHandle pf_file_handle;
  RC ret=pfm.OpenFile(fileName, pf_file_handle);
  if (ret!=OK)
    return ret;

  PF_PageHandle pf_page_handle;
  ret=pf_file_handle.GetFirstPage(pf_page_handle);
  if (ret!=OK)
    return ret;

  Byte *mem;
  ret=pf_page_handle.GetData(mem);
  
  int record_size;
  memcpy(&record_size, mem, sizeof(record_size));
  mem+=sizeof(record_size);

  int max_record_on_page;
  memcpy(&max_record_on_page, mem, sizeof (max_record_on_page));
  mem+=sizeof(max_record_on_page);
  
  int total_record;
  memcpy(&total_record, mem, sizeof(total_record));
  mem+=sizeof(total_record);

  int *avail_spaces=new int[pf_file_handle.GetTotalPage()];
  for(int i=0; i<pf_file_handle.GetTotalPage(); i++){
    memcpy(avail_spaces+i, mem, sizeof(int));
    mem+=sizeof(int);
  }

  rm_file_handle=RM_FileHandle(record_size, max_record_on_page, total_record, avail_spaces
                               , pf_file_handle);

  return OK;
}

RC RM_MANAGER::CloseFile(RM_FileHandle &rm_file_handle){
  PF_PageHandle pf_file_handle;
  pfm.OpenFile(fileName, pf_file_handle);

  //allocate header page
  PF_PageHandle pf_page_handle=rm_file_handle.GetPFFileHandle();
  
  //Get data address of the header page
  Byte *mem;
  ret=pf_page_handle.GetData(mem);
  
  //write record_size, max_record_on_page total_record, record_number[] 
  int x=rm_file_handle.GetRecordSize();
  memcpy(mem, &x, sizeof(x));
  mem+=sizeof(x);

  int x=rm_file_handle.GetMaxRecordOnPage();
  memcpy(mem, &x, sizeof(x));
  mem+=sizeof(x);

  int x=rm_file_handle.GetTotalRecord();
  memcpy(mem, &x, sizeof(x));
  mem+=sizeof(x);

  for(int i=0; i<pf_page_handle.GetTotalPage(); i++){
    int x=rm_page_handle.GetAvailSpaces(i);
    memcpy(mem, &x, sizeof(x));
    mem+=sizeof(x);
  }
  
  pf_file_handle.MarkDirty(0);
  pfm.CloseFile(pf_file_handle);
}

//-------------------------------[RM_MANAGER]-----------------------------------
