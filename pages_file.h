#ifndef PAGES_FILE_H
#define PAGES_FILE_H

#include <iostream>
#include "pf_defines.h"

void PF_PrintError(RC rc);

class PF_PageHandle {

 private:
  Byte *mem;
  int page_number;

 public:

  PF_PageHandle();

  PF_PageHandle  (Byte *mem, int page_number);

  ~PF_PageHandle ();                          // Destructor

  PF_PageHandle  (const PF_PageHandle &pageHandle);

  PF_PageHandle& operator= (const PF_PageHandle &pageHandle);

  //This method provides access to the actual contents of a page. The PF_PageHandle object for
  //which this method is called must refer to a page that is pinned in the buffer pool.
  //If the method is successful, pData is set to point to the contents of the page in the buffer pool.
  RC GetData        (Byte *&pData) const;        // Set pData to point to
  //   the page contents
  RC GetPageNum     (int &pageNum) const;    // Return the page number
};

//http://stackoverflow.com/questions/4870437/pairint-int-pair-as-key-of-unordered-map-issue
class PairHash{
 public:
  size_t operator()(std::pair<int, int> x) const throw() {
    size_t h = x.first*100000007+x.second;
    return h;
  }
};

//Manager of buffer space for loaded pages
class PF_BufferManager{

 private:

  static const int BUFFER_SIZE=1000;

  //dirty array marks whether the loaded pages in memory has been modified or not
  bool dirty[BUFFER_SIZE];

  //addr records all memory addresses of the buffer
  Byte addr[BUFFER_SIZE][(8<<10)+8];

  //Each of file_id[i] corresponds to addr[i[ and page_number[i]
  int file_id[BUFFER_SIZE];

  //Same as above
  int page_number[BUFFER_SIZE];

  //Map from file_id - page_number pair to buffer index
  std::unordered_map<std::pair<int,int>, int, PairHash> filepage2idx;

  //current time stamp
  int time_stamp;

  //time stamp for the access of all buffers
  int time[BUFFER_SIZE];

  //Map from visiting time stamp to buffer index
  std::map<int,int> time2idx;

  //Available indexes of buffer
  std::vector<int> avail_idx;

  //FindIndex finds an empty space in buffer and return it as idx. When all space in
  //buffer is used up, it applies Least Recently Used algorithm to free one index and
  //take its spot.
  RC _FindIndex(int &idx, bool &overwrite);

  //RemoveIndex remove the idx-th buffer from the memory.
  RC _RemoveIndex(const int idx);

  //Mark the idx-th buffer as newly visited.
  RC _VisitIndex(const int idx);

 public:
  PF_BufferManager();

  ~PF_BufferManager();

  //Find page_number-th page in file indexed by file_id if it's already loaded into buffer
  // and return mem=0 if it's not. Also mark it as visited.
  RC FindPage(const int file_id, const int page_number, Byte *&mem);

  //Loaded page_number-th page in file indexed by file_id into buffer. Do nothing if it's
  //already loaded. Update
  RC AddPage(FILE * f, const int file_id, const int page_number, Byte *&mem);

  //Removee page_number-th page in file indexed by file_id from buffer. Do nothing if it's
  //not in buffer. Before removal, save its content into corresponding file if it's dirty.
  //Also, remove its index from index list.
  RC RemovePage(FILE *f, const int file_id, const int page_number);

  //Force page_number-th page in file indexed by file_id to save temporary content into disk
  //if it's dirty.
  RC ForcePage(FILE *f, const int file_id, const int page_number);

  //Mark page_number-th page in file indexed by file_id as dirty.
  RC MarkDirty(const int file_id, const int page_number);

  RC Show(std::ostream &Out);

};

class PF_FileHandle {

 private:

  PF_BufferManager *buffer_manager;

  FILE *f;

  int file_id;

  int total_page;

  void _InitPages();

  RC LoadPageWithCurrentPosition(PF_PageHandle &page_handle, int page_num);

 public:

  int GetTotalPage()const{
    return total_page;
  }

  PF_FileHandle();

  PF_FileHandle  (FILE *f, int f_id, PF_BufferManager *buffer_manager2);                                  // Default constructor

  ~PF_FileHandle ();                                  // Destructor

  PF_FileHandle  (const PF_FileHandle &fileHandle);   // Copy constructor

  PF_FileHandle& operator= (const PF_FileHandle &fileHandle);

  FILE* getF();

  //For this and the following methods, it is a (positive) error if the PF_FileHandle
  //object for which the method is called does not refer to an open file. This method
  //reads the first page of the file into the buffer pool in memory. If the page fetch
  //is successful, the pageHandle object becomes a handle for the page. The page handle
  //is used to access the page's contents (see the PF_PageHandle class description below).
  //The page read is automatically pinned in the buffer pool and remains pinned until
  //it is explicitly unpinned by calling the UnpinPage method (below). This method
  //returns the positive code PF_EOF if end-of-file is reached (meaning there is no first page).
  RC GetFirstPage   (PF_PageHandle &pageHandle) ;   // Get the first page

  RC GetLastPage    (PF_PageHandle &pageHandle) ;   // Get the last page

  //This method reads into memory the next valid page after the page whose
  //number is current. If the page fetch is successful, pageHandle becomes a handle
  //for the page. The page read is pinned in the buffer pool until it is unpinned by
  //calling the UnpinPage method. This method returns PF_EOF if end-of-file is reached
  //(meaning there is no next page). Note that it is not an error if current does not
  //correspond to a valid page (e.g., if the page numbered current has been disposed of).
  RC GetNextPage    (int current, PF_PageHandle &pageHandle) ;

  RC GetPrevPage    (int current, PF_PageHandle &pageHandle) ;

  //Parameter pageNum must be a valid page number. As usual, the page read is pinned
  //in the buffer pool until it is explicitly unpinned.
  RC GetThisPage    (int pageNum, PF_PageHandle &pageHandle) ;

  //This method allocates a new page in the file, reads the new page into memory,
  //and pins the new page in the buffer pool. If successful, pageHandle becomes a handle for
  //the new page.
  RC AllocatePage   (PF_PageHandle &pageHandle);         // Allocate a new page

  //This method marks the page specified by pageNum as "dirty," indicating that the contents
  //of the page have been or will be modified. The page must be pinned in the buffer pool.
  //A page marked as dirty is written back to disk when the page is removed from the buffer
  //pool. (Pages not marked as dirty are never written back to disk.)
  RC MarkDirty      (int pageNum) ;             // Mark a page as dirty

  //This method tells the PF component that the page specified by pageNum is no longer
  //needed in memory.
  RC UnpinPage      (int pageNum = ALL_PAGES);             // Unpin a page

  //This method copies the contents of the page specified by pageNum from the buffer
  //pool to disk if the page is in the buffer pool and is marked as dirty. The page remains
  //in the buffer pool but is no longer marked as dirty. If no specific page number is provided
  //(i.e., pageNum = ALL_PAGES), then all dirty pages of this file that are in the buffer pool
  //are copied to disk and are no longer marked as dirty. Note that page contents are copied
  //to disk whether or not a page is pinned.
  RC ForcePage     (int pageNum = ALL_PAGES) ;

  RC Show(std::ostream &Out); 

 };

class PF_Manager
{

 private:
  PF_BufferManager buffer_manager;
  int current_file_id;

 public:
  PF_Manager    ();                           // Constructor

  ~PF_Manager   ();                           // Destructor

  PF_BufferManager* GetBufferManager(){
    return &buffer_manager;
  }

  RC CreateFile    (const char *fileName);       // Create a new file

  RC DestroyFile   (const char *fileName);       // Destroy a file

  //This method opens the paged file whose name is fileName. The file must already
  //exist and it must have been created using the CreateFile method. If the method
  //is successful, the fileHandle object whose address is passed as a parameter becomes
  //a "handle" for the open file. The file handle is used to manipulate the pages of
  //the file (see the PF_FileHandle class description below). It is a (positive) error
  //if fileHandle is already a handle for an open file when it is passed to the OpenFile method.
  //It is not an error to open the same file more than once if desired, using a different
  //fileHandle object each time. Each call to the OpenFile method creates a new "instance" of
  //the open file. Warning: Opening a file more than once for data modification is not prevented
  //by the PF component, but doing so is likely to corrupt the file structure and may crash the
  //PF component. Opening a file more than once for reading is no problem.
  RC OpenFile      (const char *fileName, PF_FileHandle &fileHandle);

  //This method closes the open file instance referred to by fileHandle. The file must have
  //been opened using the OpenFile method. All of the file's pages are flushed from the buffe
  //r pool when the file is closed. It is a (positive) error to attempt to close a file when
  //any of its pages are still pinned in the buffer pool.
  RC CloseFile     (PF_FileHandle &fileHandle);  // Close a file

  //This method allocates a "scratch" memory page (block) in the buffer pool and sets buffer
  //to point to it. The amount of memory available in the block is PF_PAGE_SIZE + 4 = 4096 bytes.
  //The scratch page is automatically pinned in the buffer pool.
  //RC AllocateBlock (char *&buffer);

  //This method disposes of the scratch page in the buffer pool pointed to by buffer, which
  //must have been allocated previously by PF_Manager::AllocateBlock. Similar to pinning an
  //d unpinning, you must call PF_Manager::DisposeBlock for each buffer block obtained
  //by calling PF_Manager::AllocateBlock; otherwise you will lose pages in the buffer pool permanently.
  //RC DisposeBlock  (char *buffer);
};


#endif
