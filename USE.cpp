#include "record_manager.h"
#include "parser.h"
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

  if (argc==2){

    char *dbname=argv[1];
    int len=strlen(dbname);
    if (len>0 && dbname[len-1]==';')
      dbname[len-1]=0;
    
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    SM_Manager smm(rmm);

    // open the database
    RC rc = smm.OpenDb(dbname);
    if (rc!=OK){
      cerr << "-- Unable to open database "<<dbname<<endl;
      return 0;
    }

    MyParse(cin, smm);

    rc = smm.CloseDb();
    if (rc!=OK){
      cerr << "-- Unable to close database "<<dbname<<endl;
      return 0;
    }

    return 0;
  }

  cerr << "Usage[0]: "<< argv[0] <<" databaseName \n";
  exit(1);
 }
