#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>
#include <utility>
#include <unordered_map>
#include <cstring>
#include <vector>
#include <map>
#include <cassert>

#define EPS (1e-6)

enum RC{
  OK,
  UNKNOWN_ERROR,
  CREATE_ERROR,
  OPEN_ERROR,
  LOAD_PAGE_ERROR,
  DISPOSE_ERROR,
  ADDPAGE_ERROR,
  REMOVEPAGE_ERROR,
  FORCEPAGE_ERROR,
  WRONG_PAGE_NUMBER,
  TOO_LONG_RECORD,
  INVALID_RID,
  INVALID_RECORD,
  EXPANDING_EMPTY_FILE,
  OPERATION_ON_HEAD_PAGE,
  NO_RID_IN_RECORD,
  INVALID_PAGE,
  NOT_FOUND,
  ATTRIBUTE_CATALOG_OPEN_ERROR,
  SYSTEM_CATALOG_OPEN_ERROR,
  ATTRIBUTE_CATALOG_CLOSE_ERROR,
  SYSTEM_CATALOG_CLOSE_ERROR,
  TABLE_CREATE_ERROR
};

enum AttrType{
  INT,
  FLOAT,
  STRING
};

enum CompOp{
  NO_OP,
  EQ_OP,
  LT_OP,
  GT_OP,
  LE_OP,
  GE_OP,
  NE_OP
};

enum ClientHint{
  NO_HINT
};

#define ALL_PAGES -1

typedef unsigned int BufType;
typedef unsigned char Byte;

const int RELNAME_LENGTH=30;
const int ATTRNAME_LENGTH=30;
const int MAX_ATTRIBUTE=100;
const int MAX_VARCHAR_LENGTH=500;
