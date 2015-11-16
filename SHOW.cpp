#include "record_manager.h"
#include <string>
#include <iostream>

using namespace std;

int main (
          int argc,      // length of array argv
          char **argv )  // argv is an array of pointers
// argv[0] points to the name of the command
// argv[1] points to DATABASE
// argv[2] points to argument DBname
{
  char *dbname;

  if (argc==3 && string(argv[1])=="DATABASE"){
    
    dbname=argv[2];
    
    return 0;
  }

  cerr << "Usage "<< argv[0] <<" DATABASE databaseName \n";
  exit(1);
 }
