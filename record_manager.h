#ifndef RECORD_MANAGER_H
#define RECORD_MANAGER_H
#include "pages_file.h"


void RM_PrintError (RC rc);

class RID {

 private:
  int page_number;
  int slot_number;

 public:
  RID();
  ~RID       ();         
  
  //This method should construct a new record identifier object from a given page number and slot number.
  RID        (int pageNum, int slotNum);

  //For this and the following method, it should be a (positive) error if the RID object for which the 
  //method is called is not a viable record identifier. (For example, a RID object will not be a viable 
  //record identifier if it was created using the default constructor and has not been passed to the 
  //RM_Record::GetRid method.) If this method succeeds, it should set pageNum to the record identifier's page number.
  RC GetPageNum (int &pageNum) const;  // Return page number

  RC GetSlotNum (int &slotNum) const;  // Return slot number

  bool Valid()const;
};

class RM_Record {
  Byte *mem;
  RID rid;
 public:
  RM_Record  (); 
  RM_Record(const Byte *pData, int record_size, const RID &rid);
  ~RM_Record (); 

  //For this and the following method, it should be a (positive) error if a
  //record has not already been read into the RM_Record object for which the method is
  //called. This method provides access to the contents (data) of the record. If the 
  //method succeeds, pData should point to the contents of the copy of the record 
  //created by RM_FileHandle::GetRec() or RM_FileScan::GetNextRec().
  RC GetData    (const Byte *&pData) const;   

  RC GetRid     (RID &rid) const;  

  bool Valid()const;
};


class RM_FileHandle {
  
  int record_size;
  int max_record_on_page;
  int total_record;
  int *avail_spaces;
  
  PF_FileHandle pf_file_handle;

  RC _ExpandPages();

 public:
  
  int GetRecordSize()const{
    return record_size;
  }

  int GetMaxRecordOnPage()const{
    return max_record_on_page;
  }

  int GetTotalRecord()const{
    return total_record;
  }

  int GetAvailSpaces(int i)const{
    assert(i>=0 && i<pf_file_handle.GetTotalPage());
    return avail_spaces[i];
  }

  int GetTotalPage()const{
    return pf_file_handle.GetTotalPage();
  }

  PF_FileHandle GetPFFileHandle()const{
    return pf_file_handle;
  }

  RM_FileHandle();
  RM_FileHandle  (const int, const int, const int, int *, const PF_FileHandle&);      
  RM_FileHandle(const RM_FileHandle &rfh);

  RM_FileHandle& operator=(const RM_FileHandle &rfh);

  ~RM_FileHandle ();      

  //For this and the following methods, it should be a (positive) error if the RM_FileHandle
  //object for which the method is called does not refer to an open file. This method should
  // retrieve the record with identifier rid from the file. It should be a (positive) error
  //if rid does not identify an existing record in the file. If the method succeeds, rec
  //should contain a copy of the specified record along with its record identifier 
  //(see the RM_Record class description below).
  RC GetRec         (const RID &rid, RM_Record &rec) ;

  //This method should insert the data pointed to by pData as a new record in the file. 
  //If successful, the return parameter &rid should point to the record identifier of the
  //newly inserted record.
  RC InsertRec      (const Byte *pData, RID &rid);       // Insert a new record,

  //This method should delete the record with identifier rid from the file. If the page 
  //containing the record becomes empty after the deletion, you can choose either to dispose
  //of the page (by calling PF_Manager::DisposePage) or keep the page in the file for use
  //in the future, whichever you feel will be more efficient and/or convenient.
  RC DeleteRec      (const RID &rid);                    // Delete a record

  //This method should update the contents of the record in the file that is associated 
  //with rec (see the RM_Record class description below). This method should replace the
  //existing contents of the record in the file with the current contents of rec.
  RC UpdateRec      (const RM_Record &rec);              // Update a record

  //This method should call the corresponding method PF_FileHandle::ForcePages in order
  //to copy the contents of one or all dirty pages of the file from the buffer pool to disk.
  RC ForcePages     (int pageNum = ALL_PAGES) ;

  bool IsValidSlot(const int page_number, const int slot_number);

  void Show(std::ostream &Out);
};

class RM_FileScan {

  RM_FileHandle &rm_file_handle;
  AttrType attr_type;
  int attr_length;
  int attr_offset;
  CompOp comp_op;
  void *value;
  int valueLength;

  int current_page;
  int current_slot;

  bool _Validate(const int op1, const int op2)const;
  bool _Validate(const float op1, const float op2)const;
  bool _Validate(const std::string &op1, const std::string &op2)const;

 public:             

  //This method should initialize a scan over the records in the open file referred to by
  //parameter fileHandle. During the scan, only those records whose specified attribute 
  //satisfies the specified condition (a comparison with a value) should be retrieved. 
  //If value is a null pointer, then there is no condition and all records are retrieved
  //during the scan. If value is not a null pointer, then value points to the value that
  //attributes are to be compared with.
  //----
  //Parameters attrType and attrLength indicate the type and length of the attribute being
  //compared.
  //--
  // Type AttrType is defined in redbase.h as follows: INT for integer, FLOAT for floating 
  //point number, and STRING for character string. You will need to cast the value into the 
  //appropriate type for the attribute (or, in the case of an integer or float, copy it into
  //a separate variable to avoid alignment problems). If a character string has length n,
  //then the attribute and the value will each be exactly n bytes long. They will not be 
  //<= n bytes, i.e., no "padding" is required, and they are not null-terminated.
  //--
  //Parameter attrOffset indicates where the attribute is found within the contents of 
  //each record.
  //--
  //Parameter compOp indicates the way that the record's attribute value should be compared
  //with the value parameter. 
  RM_FileScan     (RM_FileHandle &fileHandle, 
		   AttrType      attrType,
		   int           attrLength,
		   int           attrOffset,
		   CompOp        compOp,
		   void          *value,
		   int           valueLength=4,
		   ClientHint    pinHint = NO_HINT);                          
  ~RM_FileScan ();  

  //This method should retrieve a copy of the next record in the file scan that satisfies
  //the scan condition. If this method succeeds, rec should contain a copy of the record
  //along with its record identifier. This method should return RM_EOF (which you should 
  //define) if there are no records left satisfying the scan condition. You may assume 
  //that RM component clients will not close the corresponding open file instance while a
  //scan is underway.
  RC GetNextRec   (RM_Record &rec);    

  bool over()const;   

  //This method should terminate the file scan.           
  RC CloseScan    ();                                
};

class RM_Manager {

 private:
  PF_Manager &pfm;

 public:
  RM_Manager  (PF_Manager &pfm);            // Constructor
  ~RM_Manager ();                           // Destructor

  //This method should call PF_Manager::CreateFile to create a paged file called fileName. 
  //The records in this file will all have size recordSize. This method should initialize
  //the file by storing appropriate information in the header page. Although recordSize will
  // usually be much smaller than the size of a page, you should compare recordSize with 
  //PF_PAGE_SIZE and return a nonzero code if recordSize is too large for your RM component to handle.
  RC CreateFile  (const char *fileName, int recordSize);  
  RC CreateFile  (std::string s, int recordSize){ return CreateFile(std::string(s), recordSize); }

  //This method should destroy the file whose name is fileName by calling PF_Manager::DestroyFile.
  RC DestroyFile (const char *fileName);     
  RC DestroyFile (std::string s){ return DestroyFile(std::string(s)); }

  //This method should open the file called fileName by calling PF_Manager::OpenFile.
  //If the method is successful, the fileHandle object should become a "handle" for the 
  //open RM component file. The file handle is used to manipulate the records in the file
  //(see the RM_FileHandle class description below). As in the PF component, it should 
  //not be an error if a client opens the same RM file more than once, using a different 
  //fileHandle object each time. Each call to the OpenFile method should create a new 
  //instance of the open file. You may assume if a file has more than one opened instance
  //then each instance of the open file may be read but will not be modified. If a file is 
  //modified while opened more than once, you need not guarantee the integrity of the file 
  //or the RM component. You may also assume that DestroyFile will never be called on an open file.
  RC OpenFile    (const char *fileName, RM_FileHandle &fileHandle);
  RC OpenFile    (std::string s, RM_FileHandle &fileHandle){ return OpenFile(std::string(s), fileHandle); }


  //This method should close the open file instance referred to by fileHandle by calling PF_Manager:: CloseFile.
  RC CloseFile   (RM_FileHandle &fileHandle);  
};

#endif
