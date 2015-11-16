#include "parser.h"

using namespace std;

void ParseAttributeList(const string &attribute_list, int &attr_count, AttrInfo *attributes){
  //attribute_list = attrName1 Type1, attrName2 Type2, ..., attrNameN TypeN NOT NULL,

  for(int i=0, j=attribute_list.find(',', 0); j!=-1; i=j, j=attribute_list.find(',', i+1)){
    stringstream ss2(attribute_list.substr(i+1, j-i-1));

    string attr_name;
    ss2>>attr_name;

    if (attr_name=="PRIMARY"){
      //PRIMARY KEY ( ID )

      string keys;
      ss2>>keys;
      if (keys=="KEY"){

        char c;
        while(ss2>>c)
          if (c=='(')
            break;

        if (c=='(' && ss2>>attr_name){
          if (attr_name.back()==')')
            attr_name.pop_back();
          if (attr_name.size()){
            
            //attr_name is the primary key

            //succeeded in parsing
            continue;
          }
        }

      }

    }else{

      string tmp=ss2.str();
      
      int left_bracket=tmp.find('('), right_bracket=tmp.find(')');
      if (left_bracket!=-1 && right_bracket!=-1 && left_bracket<right_bracket){

        // type ... ( ... length ... ) ... [NOT NULL]

        stringstream ss_type(tmp.substr(0,left_bracket));
        string attr_type;
        stringstream ss_length(tmp.substr(left_bracket+1, right_bracket-left_bracket-1));
        int attr_length;
        stringstream ss_not_null(tmp.substr(right_bracket+1));
        string nots;

        if (ss_type>>attr_type && ss_length>>attr_length){

          attributes->notNull=false;
          ss_not_null>>nots;
          if (nots=="NOT"){
            ss_not_null>>nots;
            if (nots=="NULL"){
              attributes->notNull=true;
            }
          }            

          if (attr_type=="float"){
            attributes->attrName=attr_name;
            attributes->attrType=FLOAT;
            attributes->attrLength=4;
            attributes++;
            attr_count++;
            continue;
          }
          if (attr_type=="int"){
            attributes->attrName=attr_name;
            attributes->attrType=INT;
            attributes->attrLength=4;
            attributes++;
            attr_count++;
            continue;
          }
          if (attr_type=="varchar" && attr_length>0 && attr_length<MAX_VARCHAR_LENGTH){
            attributes->attrName=attr_name;
            attributes->attrType=STRING;
            attributes->attrLength=MAX_VARCHAR_LENGTH;
            attributes++;
            attr_count++;
            continue;
          }

        }
        
      }

    }
    
    //failed to finish parsing
    attr_count=0;
    break;
  }

}

void MyParse(istream &Input, SM_Manager &smm){
  string s;
  while(Input>>s){

    if (s==";"){
      continue;
    }
    
    if (s=="EXIT" || s=="EXIT;"){
      return;
    }

    if (s=="CREATE"){
      Input>>s;
      
      if (s=="TABLE"){

        string input_s;
        char c='$';
        while(Input>>c){
          if (c==';')
            break;
          input_s.push_back(c);
        }

        int left_bracket=input_s.find('(');
        int right_bracket=input_s.find(')');

        if (left_bracket!=-1 && right_bracket!=-1 && c==';'){

          stringstream ss1(input_s.substr(0, left_bracket));
          string rel_name;
          ss1>>rel_name;
          
          int attr_count=0;
          AttrInfo attributes[MAX_ATTRIBUTE];
          ParseAttributeList(input_s.substr(left_bracket+1, right_bracket-left_bracket-1)+",", attr_count, attributes);

          if (attr_count!=0){
            smm.CreateTable(rel_name.c_str(), attr_count, attributes);          
            continue;
          }

        }
      }

    }

    if (s=="DROP"){
      Input>>s;
      
      if (s=="TABLE"){
        Input>>s;

        if (s.back()==';')
          s.pop_back();
        
        smm.DropTable(s.c_str());

        continue;
      }

    }

    cout<<"Wrong command: "<<s<<endl;
    return;
  }
}
