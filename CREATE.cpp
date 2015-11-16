#include "system_manage.h"
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
  char command[80] = "mkdir ";

  if (argc==3 && string(argv[1])=="DATABASE"){
    
    // The database name is the second argument
    dbname = argv[2];

    // Create a subdirectory for the database
    system (strcat(command,dbname));

    if (chdir(dbname) < 0) {
      cerr << argv[0] << " chdir error to " << dbname << "\n";
      exit(1);
    }

    // Create the system catalogs
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    SM_Manager smm(rmm);
    smm.CreateDB(string(dbname));

    return 0;
  }

  cerr << "Usage[0]: "<< argv[0] <<" DATABASE databaseName \n";
  exit(1);
 }
