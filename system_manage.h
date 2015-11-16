#ifndef SYSTEM_MANAGE_H
#define SYSTEM_MANAGE_H

#include "pages_file.h"
#include "record_manager.h"

#define SYS_SEP "/"
#define SYSTEM_CATALOG_NAME "__SYSTEM_CATALOG.bin"
#define ATTRIBUTE_CATALOG_NAME "__ATTRIBUTE_CATALOG.bin"

const int SYSTEM_TUPLE_LENGTH = RELNAME_LENGTH + 4 + 4 + 4;
const int ATTRIBUTE_TUPLE_LENGTH = RELNAME_LENGTH + ATTRNAME_LENGTH + 4 + 4 + 4 + 4 + 4 ;

void SM_PrintError(RC rc);

//Put data into pData and forward pData by the length of the data
void PutIntData(Byte*& pData, const int x);
void PutFloatData(Byte*& pData, const float x);
void PutStringData(Byte*& pData, const char *s, const int len);

// Used by SM_Manager::CreateTable
struct AttrInfo {
  std::string     attrName;           // Attribute name
  AttrType attrType;            // Type of attribute
  int      attrLength;          // Length of attribute
  int notNull;
};

// in system catalog: relName, tupleLength, attrCount, indexCount
// in attribute catalog: relName, attrName, offset, attrType, attrLenth, indexNo, Nullable

class SM_Manager {
  
  RM_Manager &rmm;

  std::string db_dir;
  RM_FileHandle system_fh;
  RM_FileHandle attribute_fh;

 public:
  SM_Manager  (RM_Manager &rmm);  // Constructor
  ~SM_Manager ();                                  // Destructor
  RC CreateDB    (const std::string &s);
  RC OpenDb      (const char *dbName);                // Open database
  RC CloseDb     ();                                  // Close database
  RC CreateTable (const char *relName,                // Create relation
		  int        attrCount,
		  AttrInfo   *attributes);
  RC DropTable   (const char *relName);               // Destroy relation

  /*
  RC CreateIndex (const char *relName,                // Create index
		  const char *attrName);
  RC DropIndex   (const char *relName,                // Destroy index
		  const char *attrName);
  */

  /*
    RC Load        (const char *relName,                // Load utility
		  const char *fileName);
  */
  RC Help        ();                                  // Help for database
  RC Help        (const char *relName);               // Help for relation
  RC Print       (const char *relName);               // Print relation
};
#endif
