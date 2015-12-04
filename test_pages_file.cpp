#include "pages_file.h"
#include <iostream>

using namespace std;

int main(){

  PF_Manager pfm;
  PF_FileHandle pfh;
  PF_PageHandle pph;

  char dbname[]="test_pf.db";

  cout<<"Destroy..."<<endl;
  pfm.DestroyFile(dbname);

  cout<<"Create..."<<endl;
  pfm.CreateFile(dbname);

  cout<<"Open..."<<endl;
  pfm.OpenFile(dbname, pfh);

  int total_page=200000;

  cout<<"Insert "<<total_page<<" pages..."<<endl;
  for(int i=0; i<total_page; i++){
    pfh.AllocatePage(pph);
    //pfm.GetBufferManager()->Show(cout);
  }

  //pfh.Show(cout);

  cout<<"Unpin page #19..."<<endl;
  pfh.UnpinPage(19);

  cout<<"Mark Dirty #17..."<<endl;
  pfh.MarkDirty(17);
  //pfm.GetBufferManager()->Show(cout);

  cout<<"Force page #17..."<<endl;
  pfh.ForcePage(17);
  //pfm.GetBufferManager()->Show(cout);

  cout<<"Unpin all..."<<endl;
  pfh.UnpinPage(ALL_PAGES);
  //pfm.GetBufferManager()->Show(cout);

  cout<<"Write first page..."<<endl;
  pfh.GetFirstPage(pph);
  Byte *data;
  pph.GetData(data);
  for(int i=0; i<(8<<10); i++) data[i]=1;
  pfh.MarkDirty(0);

  cout<<"Write last page..."<<endl;
  pfh.GetLastPage(pph);
  pph.GetData(data);
  int pn;
  pph.GetPageNum(pn);
  for(int i=0; i<(8<<10); i++) data[i]=Byte(pn+1);
  pfh.MarkDirty(total_page-1);

  cout<<"Write next page..."<<endl;
  pfh.GetNextPage(0, pph);
  pph.GetData(data);
  pph.GetPageNum(pn);
  for(int i=0; i<(8<<10); i++) data[i]=pn+1;
  pfh.MarkDirty(1);

  cout<<"Write prev page..."<<endl;
  pfh.GetPrevPage(3,pph);
  pph.GetData(data);
  pph.GetPageNum(pn);
  for(int i=0; i<(8<<10); i++) data[i]=pn+1;
  pfh.MarkDirty(2);

  cout<<"Write this page..."<<endl;
  pfh.GetThisPage(3,pph);
  pph.GetData(data);
  pph.GetPageNum(pn);
  for(int i=0; i<(8<<10); i++) data[i]=pn+1;
  pfh.MarkDirty(3);

  //  pfm.GetBufferManager()->Show(cout);

  cout<<"Close..."<<endl;
  pfm.CloseFile(pfh);

  cout<<"reopening..."<<endl;
  pfm.OpenFile(dbname, pfh);

  //pfh.Show(cout);

  cout<<"checking..."<<endl;
  for(int i=0; i<total_page; i++){
    if (i<=3 || i==total_page-1){
      pfh.GetThisPage(i, pph);
      pph.GetData(data);
      bool flag=0;
      for(int k=0; k<(8<<10); k++)
        if (data[k]!=Byte(i+1)){
          cout<<"[Error] Wrong content in #"<<i<<" page, #"<<k<<" byte! ("<<i+1<<" vs "<<int(data[k])<<")"<<endl;
          flag=1;
          break;
        }
      if (!flag)
        cout<<"Correct result in page #"<<i<<endl;
    }
  }

  cout<<"Close..."<<endl;
  pfm.CloseFile(pfh);

  cout<<"reopening for massive writing..."<<endl;
  pfm.OpenFile(dbname, pfh);
  for(int i=0; i<total_page; i++){
    pfh.GetThisPage(i, pph);
    pph.GetData(data);
    for(int j=0; j<(8<<10); j++)
      data[j]=Byte(i);
    pfh.MarkDirty(i);
  }
  //pfm.GetBufferManager()->Show(cout);
  cout<<"close..."<<endl;
  pfm.CloseFile(pfh);

  pfm.OpenFile(dbname, pfh);
  cout<<"checking massive writing..."<<endl;
  bool flag=0;
  for(int i=0; i<total_page; i++){
    pfh.GetThisPage(i, pph);
    pph.GetData(data);
    for(int k=0; k<(8<<10); k++)
      if (data[k]!=Byte(i)){
        cout<<"[Error] Wrong content in #"<<i<<" page, #"<<k<<" byte! ("<<int(Byte(i))<<" vs "<<int(data[k])<<")"<<endl;
        flag=1;
        break;
      }
    if (flag){
      break;
    }
  }
  if (!flag)
    cout<<"Correct result in all pages!"<<endl;
  pfm.CloseFile(pfh);

  //pfm.DestroyFile(pfh);
  
  return 0;
}
