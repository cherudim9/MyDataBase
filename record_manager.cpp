#include "record_manager.h"

using namespace std;

void RM_PrintError (RC rc){
  cerr << "--RM--: ";
  switch(rc){
    
  case EXPANDING_EMPTY_FILE:
    cerr<< " Tried to expand the file specified in record_file_handle, but the handle is invalid";
    break;

  case TOO_LONG_RECORD:
    cerr<< "Record size too long, exceeded (8192-96)";
    break;

  case OPERATION_ON_HEAD_PAGE:
    cerr<<"Operation on head page is invalid";
    break;

  default:
    cerr<<"Unknown error??? (code: "<<rc<<" )";
  }
  cerr << endl;
}

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
    return INVALID_RID;
  return OK;
}

RC RID::GetSlotNum(int &slot_number)const{
  slot_number=this->slot_number;
  if (slot_number<0)
    return INVALID_RID;
  return OK;
}

bool RID::Valid()const{
  return slot_number>=0 && page_number>=0;
}

//-------------------------------[RID]-----------------------------------

//-------------------------------[RM_RECORD]-----------------------------------

RM_Record::RM_Record(){

}

RM_Record::RM_Record(const Byte *pData, int record_size, const RID &rid){
  this->mem=new Byte[record_size];
  memcpy(this->mem, pData, record_size * sizeof(Byte));
  this->rid=rid;
}

RM_Record::~RM_Record(){

}

RC RM_Record::GetData(const Byte *&pData)const{
  pData=this->mem;
  if (!Valid())
    return INVALID_RECORD;
  return OK;
}

RC RM_Record::GetRid(RID &rid)const{
  rid=this->rid;
  if (!Valid())
    return INVALID_RECORD;
  return OK;
}

bool RM_Record::Valid()const{
  return rid.Valid();
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

RM_FileHandle::RM_FileHandle(){
  this->record_size=-1;
  this->max_record_on_page=-1;
  this->total_record=-1;
  this->avail_spaces=0;
}

RM_FileHandle::RM_FileHandle(const RM_FileHandle &rfh){
  this->record_size = rfh.GetRecordSize();
  this->max_record_on_page = rfh.GetMaxRecordOnPage();
  this->total_record = rfh.GetTotalRecord();
  this->pf_file_handle = rfh.GetPFFileHandle();
  int n=this->pf_file_handle.GetTotalPage();
  this->avail_spaces = new int [n];
  for(int i=0; i<n; i++)
    this->avail_spaces[i]=rfh.GetAvailSpaces(i);
}

RM_FileHandle& RM_FileHandle::operator=(const RM_FileHandle &rfh2){
  this->record_size = rfh2.GetRecordSize();
  this->max_record_on_page = rfh2.GetMaxRecordOnPage();
  this->total_record = rfh2.GetTotalRecord();
  this->pf_file_handle = rfh2.GetPFFileHandle(); 
  int n=this->pf_file_handle.GetTotalPage();
  this->avail_spaces=new int[n];
  for(int i=0; i<n; i++)
    this->avail_spaces[i]=rfh2.GetAvailSpaces(i);
  return *this;
}

RM_FileHandle::~RM_FileHandle(){
  if (avail_spaces){
    delete avail_spaces;
  }
}

RC RM_FileHandle::_ExpandPages(){
  if (pf_file_handle.getF()==0){
    RM_PrintError(EXPANDING_EMPTY_FILE);
    return EXPANDING_EMPTY_FILE;
  }
  int current_total_page=pf_file_handle.GetTotalPage();

  PF_PageHandle pf_page_handle;
  for(int i=0; i<current_total_page; i++){
    RC ret=pf_file_handle.AllocatePage(pf_page_handle);
    if (ret!=OK)
      return ret;
  }

  int *a=new int[pf_file_handle.GetTotalPage()];
  for(int i=0; i<current_total_page; i++)
    a[i]=this->avail_spaces[i];
  for(int i=current_total_page; i<pf_file_handle.GetTotalPage(); i++)
    a[i]=this->max_record_on_page;
  this->avail_spaces=a;

  return OK;
}

RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec){
  int page_number;
  int slot_number;
  RC ret=rid.GetPageNum(page_number);
  if (ret!=OK)
    return ret;
  ret=rid.GetSlotNum(slot_number);
  if (ret!=OK)
    return ret;

  if (page_number==0){
    RM_PrintError(OPERATION_ON_HEAD_PAGE);
    return OPERATION_ON_HEAD_PAGE;
  }

  if (!IsValidSlot(page_number, slot_number)){
    rec=RM_Record();
    return NOT_FOUND;
  }
  
  PF_PageHandle page_handle;
  ret=pf_file_handle.GetThisPage(page_number, page_handle);
  if (ret!=OK)
    return ret;
  pf_file_handle.MarkDirty(page_number);
  
  Byte *mem;
  ret=page_handle.GetData(mem);

  mem+=96+record_size*slot_number;
  
  rec=RM_Record(mem, record_size, RID(page_number, slot_number));

  return OK;
}

RC RM_FileHandle::UpdateRec(const RM_Record &rec){
  RID rid;
  RC ret=rec.GetRid(rid);
  if (ret!=OK)
    return ret;
  
  int page_number;
  int slot_number;
  ret=rid.GetPageNum(page_number);
  if (ret!=OK)
    return ret;
  ret=rid.GetSlotNum(slot_number);
  if (ret!=OK)
    return ret;

  if (page_number==0){
    RM_PrintError(OPERATION_ON_HEAD_PAGE);
    return OPERATION_ON_HEAD_PAGE;
  }

  if (!IsValidSlot(page_number, slot_number)){
    return NOT_FOUND;
  }
  
  PF_PageHandle page_handle;
  ret=pf_file_handle.GetThisPage(page_number, page_handle);
  if (ret!=OK)
    return ret;
  pf_file_handle.MarkDirty(page_number);
  
  Byte *mem;
  ret=page_handle.GetData(mem);
  mem+=96+record_size*slot_number;
  
  const Byte *new_mem;
  ret=rec.GetData(new_mem);

  memcpy(mem, new_mem, record_size);
  
  return OK;
}

RC RM_FileHandle::ForcePages(int page_number){
  return pf_file_handle.ForcePage(page_number);
}

RC RM_FileHandle::InsertRec(const Byte *pData, RID &rid){
  int page_number=-1;
  RC ret;
  //don't insert into the first page; it's the head page
  for(int i=1; i<pf_file_handle.GetTotalPage(); i++)
    if (avail_spaces[i]!=0){
      page_number=i;
      break;
    }
  if (page_number==-1){
    page_number=(pf_file_handle.GetTotalPage());
    ret=_ExpandPages();
    if (ret!=OK)
      return ret;
  }

  if (page_number==0){
    RM_PrintError(OPERATION_ON_HEAD_PAGE);
    return OPERATION_ON_HEAD_PAGE;
  }
  
  PF_PageHandle pf_page_handle;
  ret=pf_file_handle.GetThisPage(page_number, pf_page_handle);
  if (ret!=OK)
    return ret;

  Byte *mem;
  ret=pf_page_handle.GetData(mem);
  pf_file_handle.MarkDirty(page_number);

  int slot=-1;
  Byte *p=mem+(8<<10);
  for(int i=0; i<max_record_on_page; ){
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
  int page_number;
  int slot_number;
  RC ret=rid.GetPageNum(page_number);
  if (ret!=OK)
    return ret;
  ret=rid.GetSlotNum(slot_number);
  if (ret!=OK)
    return ret;

  if (page_number==0){
    RM_PrintError(OPERATION_ON_HEAD_PAGE);
    return OPERATION_ON_HEAD_PAGE;
  }

  if (!IsValidSlot(page_number, slot_number)){
    return NOT_FOUND;
  }
  
  PF_PageHandle pf_page_handle;
  ret=pf_file_handle.GetThisPage(page_number, pf_page_handle);
  if (ret!=OK)
    return ret;
  Byte *mem;
  ret=pf_page_handle.GetData(mem);
  pf_file_handle.MarkDirty(page_number);

  Byte *p=mem+(8<<10);
  for(int i=0; ; ){
    p--;
    if (slot_number>=i && slot_number<i+8){
      int np=*p-(1<<(slot_number-i));
      memcpy(p, &np, 1);
      break;
    }else
      i+=8;
  }

  total_record--;
  avail_spaces[page_number]++;
  
  return OK;
}

bool RM_FileHandle::IsValidSlot(const int page_number, const int slot_number){
  PF_PageHandle pf_page_handle;
  pf_file_handle.GetThisPage(page_number, pf_page_handle);
  Byte *mem;
  pf_page_handle.GetData(mem);
  mem+=(8<<10);
  mem-=(slot_number/8)+1;
  int x=slot_number%8;
  return (((*mem)>>x)&1)==1;
}

void RM_FileHandle::Show(std::ostream &Out){
  Out<<"[Show] ";
  Out<<"record_size = "<<record_size<<"\t";
  Out<<"max_record_on_page = "<<max_record_on_page<<"\t\t";
  Out<<"total_record = "<<total_record<<"\t";
  Out<<endl<<"       ";
  Out<<"total page = "<<pf_file_handle.GetTotalPage()<<", ";
  Out<<"available_spaces = [";
  const int max_record_shown=500;
  for(int i=0, n=pf_file_handle.GetTotalPage(), cnt=0; i<n; i++){
    if (cnt<max_record_shown || i==n-1){
      Out<<avail_spaces[i]<<" ";
      cnt++;
    }else{
      if (cnt==max_record_shown)
        Out<<" ... ";
      cnt++;
    }
  }
  Out<<"]"<<endl;
}

//-------------------------------[RM_FILEHANDLE]-----------------------------------


//-------------------------------[RM_FILESCAN]-----------------------------------

RM_FileScan::RM_FileScan(RM_FileHandle &rm_file_handle2,
                         AttrType attr_type,
                         int attr_length,
                         int attr_offset,
                         CompOp comp_op,
                         void *value,
                         int valueLength,
                         ClientHint pinHint)
  :rm_file_handle(rm_file_handle2){
  this->attr_type=attr_type;
  this->attr_length=attr_length;
  this->attr_offset=attr_offset;
  this->comp_op=comp_op;
  this->value=value;
  this->valueLength=valueLength;

  this->current_page=1;
  this->current_slot=0;
}

RM_FileScan::~RM_FileScan(){
  
}

bool RM_FileScan::_Validate(const int op1, const int op2)const{
  switch(comp_op){
  case EQ_OP:
    return op1 == op2;
    break;
  case LT_OP:
    return op1 < op2;
    break;
  case GT_OP:
    return op1>op2;
    break;
  case LE_OP:
    return op1<=op2;
    break;
  case GE_OP:
    return op1>=op2;
    break;
  case NE_OP:
    return op1!=op2;
    break;
  default:
    ;
  }
  return 0;
}

bool RM_FileScan::_Validate(const std::string &op1, const std::string &op2)const{
  switch(comp_op){
  case EQ_OP:
    return op1 == op2;
    break;
  case LT_OP:
    return op1 < op2;
    break;
  case GT_OP:
    return op1>op2;
    break;
  case LE_OP:
    return op1<=op2;
    break;
  case GE_OP:
    return op1>=op2;
    break;
  case NE_OP:
    return op1!=op2;
    break;
  default:
    ;
  }
  return 0;
}

bool RM_FileScan::_Validate(const float op1, const float op2)const{
  float d=op1-op2;
  switch(comp_op){
  case EQ_OP:
    return d<EPS && d>-EPS;
    break;
  case LT_OP:
    return d<-EPS;
    break;
  case GT_OP:
    return d>EPS;
    break;
  case LE_OP:
    return d<EPS;
    break;
  case GE_OP:
    return d>-EPS;
    break;
  case NE_OP:
    return d>EPS || d<-EPS;
    break;
  default:
    ;
  }
  return 0;
}

bool RM_FileScan::over()const{
  return this->current_page>=rm_file_handle.GetTotalPage();
}

string GenString(char *a, int n){
  int i=0; 
  for(; i<n; i++)
    if (a[i]==0)
      break;
  return string(a,i);
}

RC RM_FileScan::GetNextRec(RM_Record &rec){
  rec=RM_Record();//null
  while(1){
    if (this->current_slot>=rm_file_handle.GetMaxRecordOnPage()){
      this->current_slot=0;
      this->current_page++;
    }
    if (this->current_page>=rm_file_handle.GetTotalPage())
      break;

    if (!rm_file_handle.IsValidSlot(this->current_page, this->current_slot)){
      this->current_slot++;
      continue;
    }
    
    RC ret=rm_file_handle.GetRec(RID(this->current_page, this->current_slot), rec);
    bool flag=false;
    const Byte *mem=0;
    if (ret==OK){
      RC ret=rec.GetData(mem);
      switch(attr_type){
      case INT:
        {
          int op1;
          memcpy(&op1, mem+attr_offset, attr_length);
          int op2=*(int*)value;
          if (_Validate(op1,op2))
            flag=true; 
          break;
        }

      case FLOAT:
        {
          float op1_f;
          memcpy(&op1_f, mem+attr_offset, attr_length);
          float op2_f=*(float*)value;
          if (_Validate(op1_f, op2_f))
            flag=true;
          break;
        }

      case STRING:
        if (_Validate(GenString((char*)(mem+attr_offset), attr_length), GenString((char*)value, valueLength)))
          flag=true;
        break;

      default:
        ;
      }
    }

    if (flag){
      rec=RM_Record(mem, rm_file_handle.GetRecordSize(), RID(this->current_page, this->current_slot));
      this->current_slot++;
      return OK;
    }
    
    this->current_slot++;
  }
  rec=RM_Record();
  return NOT_FOUND;
}

RC RM_FileScan::CloseScan(){
  return OK;
}

//-------------------------------[RM_FILESCAN]-----------------------------------


//-------------------------------[RM_MANAGER]-----------------------------------

RM_Manager::RM_Manager(PF_Manager &pfm2)
  :pfm(pfm2){

}

RM_Manager::~RM_Manager(){

}

RC RM_Manager::CreateFile(const char *fileName, int record_size){
  if (record_size> (8<<10)-96-10){
    RM_PrintError(TOO_LONG_RECORD);
    return TOO_LONG_RECORD;
  }
  RC ret=pfm.CreateFile(fileName);
  if (ret!=OK)
    return ret;
  PF_FileHandle pf_file_handle;
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

  return OK;
}

RC RM_Manager::DestroyFile(const char *fileName){
  remove(fileName);
  return OK;
}

RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &rm_file_handle){
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
  memcpy(avail_spaces, mem, sizeof(int) * pf_file_handle.GetTotalPage() );

  rm_file_handle=RM_FileHandle(record_size, max_record_on_page, total_record, avail_spaces
                               , pf_file_handle);

  return OK;
}

RC RM_Manager::CloseFile(RM_FileHandle &rm_file_handle){
  PF_FileHandle pf_file_handle=rm_file_handle.GetPFFileHandle();

  //allocate header page
  PF_PageHandle pf_page_handle;
  RC ret=pf_file_handle.GetFirstPage(pf_page_handle);
  if (ret!=OK)
    return ret;
  
  //Get data address of the header page
  Byte *mem;
  ret=pf_page_handle.GetData(mem);
  
  //write record_size, max_record_on_page total_record, record_number[] 
  int x=rm_file_handle.GetRecordSize();
  memcpy(mem, &x, sizeof(x));
  mem+=sizeof(x);

  x=rm_file_handle.GetMaxRecordOnPage();
  memcpy(mem, &x, sizeof(x));
  mem+=sizeof(x);

  x=rm_file_handle.GetTotalRecord();
  memcpy(mem, &x, sizeof(x));
  mem+=sizeof(x);

  for(int i=0; i<pf_file_handle.GetTotalPage(); i++){
    x=rm_file_handle.GetAvailSpaces(i);
    memcpy(mem, &x, sizeof(x));
    mem+=sizeof(x);
  }
  
  pf_file_handle.MarkDirty(0);
  pfm.CloseFile(pf_file_handle);

  return OK;
}

//-------------------------------[RM_MANAGER]-----------------------------------
