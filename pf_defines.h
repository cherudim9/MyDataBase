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

#define EPS (1e-8)

enum RC{
  OK,
  UNKNOWN_ERROR,
  CREATE_ERROR,
  OPEN_ERROR,
  FIRST_PAGE_ERROR,
  DISPOSE_ERROR,
  ADDPAGE_ERROR,
  REMOVEPAGE_ERROR,
  FORCEPAGE_ERROR,
  WRONG_PAGE_NUMBER,
  TOO_LONG_RECORD,
  NO_PAGE_IN_RID,
  NO_SLOT_IN_RID,
  NO_RID_IN_RECORD,
  INVALID_PAGE,
  NOT_FOUND
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
