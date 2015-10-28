#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>
#include <map>
#include <cassert>

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
  NO_PAGE_IN_PAGE,
  NO_DATA_IN_PAGE
};

#define ALL_PAGES -1

typedef unsigned int BufType;
typedef unsigned char Byte;
