#include "pages_file.h"

using namespace std;

void PF_PrintError(RC rc){
  cerr<<"--PF--: ";
  switch (rc) {
  case OK:
    cerr<<"Nothing wrong."<<endl;
    break;
  case UNKNOWN_ERROR:
    cerr<<"??Unknown error."<<endl;
    break;
  case CREATE_ERROR:
    cerr<<"Error when try to create pf file."<<endl;
    break;
  case OPEN_ERROR:
    cerr<<"Error when try to open pf file."<<endl;
    break;
  case LOAD_PAGE_ERROR:
    cerr<<"Error when try to load contents from pf files."<<endl;
    break;
  default:
    cerr<<"### unhandled error code: "<<rc<<endl;
  }
}

//--------------------------[PF_PAGEHANDLE]----------------------------------

PF_PageHandle::PF_PageHandle(){
  this->mem=0;
  this->page_number=-1;
}

PF_PageHandle::PF_PageHandle(Byte *mem, int page_number){
  this->mem=mem;
  this->page_number=page_number;
}

PF_PageHandle::~PF_PageHandle(){
  //do nothing
}

PF_PageHandle::PF_PageHandle  (const PF_PageHandle &page_handle){
  this->mem=page_handle.mem;
  this->page_number=page_handle.page_number;
}

PF_PageHandle& PF_PageHandle::operator= (const PF_PageHandle &page_handle){
  this->mem=page_handle.mem;
  this->page_number=page_handle.page_number;
  return *this;
}

RC PF_PageHandle::GetData(Byte *& pData)const{
  pData=this->mem;
  if (pData==0){
    PF_PrintError(INVALID_PAGE);
    return INVALID_PAGE;
  }
  return OK;
}

RC PF_PageHandle::GetPageNum(int &page_number)const{
  page_number=this->page_number;
  if (page_number<0){
    PF_PrintError(INVALID_PAGE);
    return INVALID_PAGE;
  }
  return OK;
}

//--------------------------[PF_PAGEHANDLE]----------------------------------


//--------------------------[PF_FILEHANDLE]----------------------------------

bool CheckPageExistence(FILE *f){
  Byte c[3];
  int ret=fread(c,1,1,f);
  if (ret==0)
    return false;//EOF
  fseek(f, -1, SEEK_CUR);
  return true;//AVAILABLE
}

void PF_FileHandle::_InitPages(){
  total_page=0;
  fseek(f, 0, SEEK_SET);
  while(CheckPageExistence(f)){
    total_page++;
    fseek(f, (8<<10), SEEK_CUR);
  }
}

PF_FileHandle::PF_FileHandle(){
  this->f=0;
  this->file_id=-1;
  this->buffer_manager=0;
}

PF_FileHandle::PF_FileHandle(FILE *f, int file_id, PF_BufferManager *buffer_manager2)
  :buffer_manager(buffer_manager2){
  this->f=f;
  this->file_id=file_id;
  _InitPages();
}

PF_FileHandle::~PF_FileHandle(){
  //do nothing
}

PF_FileHandle::PF_FileHandle(const PF_FileHandle &other){
  this->f=other.f;
  this->file_id=other.file_id;
  this->buffer_manager=other.buffer_manager;
  _InitPages();
}

PF_FileHandle& PF_FileHandle::operator= (const PF_FileHandle &other){
  this->f=other.f;
  this->file_id=other.file_id;
  this->buffer_manager=other.buffer_manager;
  _InitPages();
  return *this;
}

FILE* PF_FileHandle::getF(){
  return f;
}

RC PF_FileHandle::LoadPageWithCurrentPosition(PF_PageHandle &page_handle, int page_num){
  if (CheckPageExistence(f)){
    //the current page is ok
    Byte *mem;
    RC ret=buffer_manager->AddPage(f, file_id, page_num, mem);
    if (ret!=OK)
      return ret;
    page_handle=PF_PageHandle(mem, page_num);
    return OK;
  }else{
    PF_PrintError(LOAD_PAGE_ERROR);
    return LOAD_PAGE_ERROR;
  }
}

RC PF_FileHandle::GetFirstPage(PF_PageHandle &page_handle){
  int page_num=0;

  Byte *mem=0;
  RC ret=buffer_manager->FindPage(file_id, page_num, mem);
  if (mem!=0){
    page_handle=PF_PageHandle(mem, page_num);
    return OK;
  }

  fseek(f, 0, SEEK_SET);
  return LoadPageWithCurrentPosition(page_handle, page_num);
}

RC PF_FileHandle::GetLastPage(PF_PageHandle &page_handle){
  int page_num=total_page-1;

  Byte *mem=0;
  RC ret=buffer_manager->FindPage(file_id, page_num, mem);
  if (mem!=0){
    page_handle=PF_PageHandle(mem, page_num);
    return OK;
  }

  fseek(f, -(8<<10), SEEK_END);
  return LoadPageWithCurrentPosition(page_handle, page_num);
}

RC PF_FileHandle::GetNextPage(int current, PF_PageHandle &page_handle) {
  if (current<0 || current>=total_page-1)
    return WRONG_PAGE_NUMBER;
  int page_num=current+1;

  Byte *mem=0;
  RC ret=buffer_manager->FindPage(file_id, page_num, mem);
  if (mem!=0){
    page_handle=PF_PageHandle(mem, page_num);
    return OK;
  }

  fseek(f, (current+1)*(8<<10), SEEK_SET);
  return LoadPageWithCurrentPosition(page_handle, page_num);
}

RC PF_FileHandle::GetPrevPage(int current, PF_PageHandle &page_handle) {
  if (current<=0 || current>=total_page)
    return WRONG_PAGE_NUMBER;
  int page_num=current-1;

  Byte *mem=0;
  RC ret=buffer_manager->FindPage(file_id, page_num, mem);
  if (mem!=0){
    page_handle=PF_PageHandle(mem, page_num);
    return OK;
  }

  fseek(f, (current-1)*(8<<10), SEEK_SET);
  return LoadPageWithCurrentPosition(page_handle, page_num);
}

RC PF_FileHandle::GetThisPage(int current, PF_PageHandle &page_handle) {
  if (current<0 || current>=total_page)
    return WRONG_PAGE_NUMBER;
  int page_num=current;

  Byte *mem=0;
  RC ret=buffer_manager->FindPage(file_id, page_num, mem);
  if (mem!=0){
    page_handle=PF_PageHandle(mem, page_num);
    return OK;
  }

  fseek(f, current*(8<<10), SEEK_SET);
  return LoadPageWithCurrentPosition(page_handle, page_num);
}

RC PF_FileHandle::AllocatePage(PF_PageHandle &page_handle){
  fseek(f, 0, SEEK_END);
  static Byte buffer[(8<<10)+2];
  memset(buffer, 0, (8<<10)+2);
  fwrite(buffer, sizeof(Byte), 8<<10, f);
  fseek(f, -(8<<10), SEEK_END);

  total_page++;
  int page_num=total_page-1;

  return LoadPageWithCurrentPosition(page_handle, page_num);
}

RC PF_FileHandle::MarkDirty(int page_num){
  return buffer_manager->MarkDirty(file_id, page_num);
}

RC PF_FileHandle::UnpinPage(int page_num){
  int l_num=page_num, r_num=page_num;
  if (page_num==ALL_PAGES)
    l_num=0, r_num=total_page-1;
  fseek(f, l_num*(8<<10), SEEK_SET);
  for(int page_num=l_num; page_num<=r_num; page_num++){
    RC ret=buffer_manager->RemovePage(f, file_id, page_num);
    if (ret!=OK)
      return ret;
  }
  return OK;
}

RC PF_FileHandle::ForcePage(int page_num){
  int l_num=page_num, r_num=page_num;
  if (page_num==ALL_PAGES)
    l_num=0, r_num=total_page-1;
  fseek(f, l_num*(8<<10), SEEK_SET);
  for(int page_num=l_num; page_num<=r_num; page_num++){
    RC ret=buffer_manager->ForcePage(f, file_id, page_num);
    if (ret!=OK)
      return ret;
  }
  return OK;
}

RC PF_FileHandle::Show(ostream &Out){
  Out<<"--------------FileHandle--------------"<<endl;
  Out<<"file_id\t\t=\t"<<file_id<<endl;
  Out<<"total_page\t=\t"<<total_page<<endl;
  
  Out<<"--------------FileHandle(end)---------"<<endl;

  return OK;
}

//--------------------------[Pf_FILEHANDLE]----------------------------------


//--------------------------[PF_BUFFERMANAGER]----------------------------------

RC PF_BufferManager::_FindIndex(int &idx, bool &overwrite){
  if (avail_idx.size()==0){
    idx=time2idx.begin()->second;
    time2idx.erase(time2idx.begin());
    overwrite=1;
  }else{
    idx=*(avail_idx.end()-1);
    avail_idx.pop_back();
    overwrite=0;
  }
  time[idx]=(time_stamp++);
  time2idx[time[idx]]=idx;
  return OK;
}

RC PF_BufferManager::_RemoveIndex(const int idx){
  time2idx.erase(time[idx]);
  time[idx]=-1;
  avail_idx.push_back(idx);
  return OK;
}

RC PF_BufferManager::_VisitIndex(const int idx){
  time2idx.erase(time[idx]);
  time[idx]=(time_stamp++);
  time2idx[time[idx]]=idx;
  return OK;
}

PF_BufferManager::PF_BufferManager(){
  for(int i=0; i<BUFFER_SIZE; i++){
    avail_idx.push_back(i);
  }
  time_stamp=0;
}

PF_BufferManager::~PF_BufferManager(){

}

RC PF_BufferManager::FindPage(const int file_id, const int page_number, Byte *&mem){
  int cnt=filepage2idx.count(make_pair(file_id,page_number));
  if (cnt==0){
    mem=0;
    return OK;
  }
  int idx=filepage2idx[make_pair(file_id, page_number)];
  mem=addr[idx];
  _VisitIndex(idx);
  return OK;
}

RC PF_BufferManager::AddPage(FILE *f, const int file_id, const int page_number, Byte *&mem){
  //read next 8k bytes from f, and put them in buffer

  RC ret=FindPage(file_id, page_number, mem);
  if (ret!=OK)
    return ret;
  if (mem!=0)
    return OK;

  int idx;
  bool overwrite;
  ret=_FindIndex(idx, overwrite);
  if (ret!=OK)
    return ret;

  if(overwrite){
    filepage2idx.erase(make_pair(this->file_id[idx], this->page_number[idx]));
    fseek(f, this->page_number[idx]*(8<<10), SEEK_SET);
    if (dirty[idx])
      fwrite(addr[idx], sizeof(Byte), (8<<10), f);
  }

  fseek(f, page_number*(8<<10), SEEK_SET);
  filepage2idx[make_pair(file_id, page_number)]=idx;
  this->file_id[idx]=file_id;
  this->page_number[idx]=page_number;
  this->dirty[idx]=0;

  mem=addr[idx];
  _VisitIndex(idx);
  fread(mem, sizeof(Byte), (8<<10), f);

  return OK;
}

RC PF_BufferManager::RemovePage(FILE *f, const int file_id, const int page_number){
  int cnt=filepage2idx.count(make_pair(file_id,page_number));
  if (cnt==0){
    fseek(f, (8<<10), SEEK_CUR);
    return OK;
  }
  int idx=filepage2idx[make_pair(file_id, page_number)];
  if (dirty[idx]){
    dirty[idx]=0;
    fwrite(addr[idx], sizeof(Byte), (8<<10), f);
  }else{
    fseek(f, (8<<10), SEEK_CUR);
  }
  filepage2idx.erase(make_pair(file_id, page_number));
  _RemoveIndex(idx);
  return OK;
}

RC PF_BufferManager::ForcePage(FILE *f, const int file_id, const int page_number){
  int cnt=filepage2idx.count(make_pair(file_id,page_number));
  if (cnt==0){
    fseek(f, (8<<10), SEEK_CUR);
    return OK;
  }
  int idx=filepage2idx[make_pair(file_id, page_number)];
  if (dirty[idx]){
    dirty[idx]=0;
    fwrite(addr[idx], sizeof(Byte), (8<<10), f);
  }else{
    fseek(f, (8<<10), SEEK_CUR);
  }
  return OK;
}

RC PF_BufferManager::MarkDirty(const int file_id, const int page_number){
  int cnt=filepage2idx.count(make_pair(file_id,page_number));
  if (cnt==0)
    return OK;
  int idx=filepage2idx[make_pair(file_id, page_number)];
  dirty[idx]=1;
  return OK;
}

RC PF_BufferManager::Show(ostream &Out){
  Out<<"-------------------Current Status of Buffer Manager------------------"<<endl;
  vector<bool> flag(BUFFER_SIZE);
  for(std::unordered_map<std::pair<int,int>, int, PairHash>::iterator it=filepage2idx.begin(); it!= filepage2idx.end(); it++)
    flag[it->second]=1;
  for(int i=0; i<BUFFER_SIZE; i++){
    Out<<"\tbuffer #"<<i<<endl;
    if (flag[i]){
      Out<<"\t\tfile_id\t\t=\t"<<file_id[i]<<endl;
      Out<<"\t\tpage_number\t=\t"<<page_number[i]<<endl;
      Out<<"\t\ttime_stamp\t=\t"<<time[i]<<endl;
      Out<<"\t\tdirty\t\t=\t"<<dirty[i]<<endl;
    }else{
      Out<<"\t\tNothing"<<endl;
    }
  }
  Out<<" --file-page to index:"<<endl;
  for(std::unordered_map<std::pair<int,int>, int, PairHash>::iterator it=filepage2idx.begin(); it!=filepage2idx.end(); it++)
    cout<<"\tfile="<<it->first.first<<" pn="<<it->first.second<<"\tidx="<<it->second<<endl;
  Out<<"--------------------------------END----------------------------------"<<endl;
  return OK;
}

//--------------------------[PF_BUFFERMANAGER]----------------------------------


//--------------------------[PF_MANAGER]----------------------------------

PF_Manager::PF_Manager(){
  current_file_id=0;
}

PF_Manager::~PF_Manager(){

}

RC PF_Manager::CreateFile(const char *fileName){
  FILE *f=fopen(fileName, "rb");
  if (f!=0){
    fclose(f);
    PF_PrintError(CREATE_ERROR);
    return CREATE_ERROR;
  }
  f=fopen(fileName, "wb");
  fclose(f);
  return OK;
}

RC PF_Manager::DestroyFile(const char *fileName){
  remove(fileName);
  return OK;
}

RC PF_Manager::OpenFile(const char *fileName, PF_FileHandle &fileHandle){
  FILE *f=fopen(fileName, "rb+");
  if (f==0){
    PF_PrintError(OPEN_ERROR);
    return OPEN_ERROR;
  }
  fileHandle = PF_FileHandle(f, current_file_id, &buffer_manager);
  current_file_id++;
  return OK;
}

RC PF_Manager::CloseFile(PF_FileHandle &fileHandle){
  fileHandle.ForcePage(ALL_PAGES);
  fileHandle.UnpinPage(ALL_PAGES);
  FILE *f=fileHandle.getF();
  fclose(f);
  return OK;
}

//--------------------------[PF_MANAGER]----------------------------------
