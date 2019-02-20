/*
Copyright (C) 2019 Łukasz Konrad Moskała

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <fstream>
#include <vector>
#include <iostream>
#include <cstring> //strerror
using namespace std;

//In your samba config file:
//[somename]               : this is share name
//  path=/tmp/path         : this is key and value

//[global] is recognized also as share
//it just uses diffrent parameters

#include "exceptions.hpp"

//verbosity control
//there is no way of changing this at run-time, so... yeah...
//TODO: verbosity control using command line options
bool debug_input=true;
bool debug_command_parameters=false;
bool debug_command_result=true;
bool debug_script=true;

//config file location
//TODO: allow overriding using command line argument
string smbconf="/etc/samba/smb.conf";

//returns true if c is whitespace character
bool isWhistespace(char c) {
  return (c == '\t' || c == '\n' || c == '\r' || c == ' ');
}
//finds first non-whitespace character
//used for finding lines that are comments
//and that have leading whitespaces
char firstNonWhitespaceCharacter(string s) {
  for(int i=0;i<s.size();i++) {
    if(!isWhistespace(s[i])) return s[i];
  }
  return 0;
}
string stripWhitespaces(string s) {
  string ret="";
  for(int i=0;i<s.size();i++)
    if(!isWhistespace(s[i]))
      ret+=s[i];
  return ret;
}
string stripLeadingWhitespaces(string s) {
  int i=-1;
  while(isWhistespace(s[++i]));
  return s.substr(i);
}
string stripTailingWhitespaces(string s) {
  int i=s.length();
  while(isWhistespace(s[--i]));
  return s.substr(0,i+1);
}

void split(string src, string tofind, string& out1, string& out2) {
  int pos=src.find(tofind);
  if(pos == -1)
    throw ssnfexception;
  int len=tofind.length();

  out1=src.substr(0,pos);
  out2=src.substr(pos+len);
}

bool writeback=true; //if set to false, program will NOT try to write to smb.conf
//usefull for testing new functions, as it will not cause any harm to system
//since changes are made in ram only, and written to disk at the end


//pair of key=value
//if writeback is set to false, this entry will be skipped when
//writing config file
struct pair_t {
  string k;
  string v;
  bool writeback=true;
};
//used to store shares and it's key=value pairs
//writeback works as above
struct share_t {
  string sharename;
  bool writeback=true;
  vector <pair_t> conf;
};
//we have to store this somewhere, don't we?
vector <share_t> shares;

/* DATA STRUCTURE DESCRIPTION:
  +shares
    +string sharename : share name
    +bool writeback   : structure is written to file only if set to true(default), delete command changes this to false
    +pair_t conf[]    : all parameters for this section
      +string k       : config file key
      +string v       : config file value
      +bool writeback : same as above
*/

//converts share name to it's index in shares vector
//throws SMBShareNotFoundException on failure
int sharenametoid(string sharename) {
  for(int i=0;i<shares.size();i++) {
    if(shares[i].sharename == sharename) return i;
  }
  throw smbsnfexception;
}

//converts key to it's index in shares->conf vector
//throws ParamNotFoundException on failue
int keytoid(int shareid, string key) {
  for(int i=0;i<shares[shareid].conf.size();i++) {
    if(shares[shareid].conf[i].k == key) return i;
  }
  throw knfexception;
}

//sets config like this
/*
[sharename]
  paramname=value
*/
//share must exist before
void cmdSet(string sharename, string key, string value) {
  if(debug_command_parameters)
  cerr<<sharename<<" "<<key<<" "<<value<<endl;
  int sid=sharenametoid(sharename);
  try {
    int pid=keytoid(sid,key);
    //if following code executes, no exception were thrown
    shares[sid].conf[pid].v=value;
    shares[sid].conf[pid].writeback=true;
    if(debug_command_result)
      cerr<<"Edited parameter "<<key<<" in section "<<sharename<<" with value "<<value<<endl;
  }
  catch(KeyNotFoundException e) {
    //parameter doesn't exist, so create it instead of editing
    pair_t p;
    p.k=key;
    p.v=value;
    p.writeback=true;
    shares[sid].conf.push_back(p);
    if(debug_command_result)
      cerr<<"Created parameter "<<key<<" in section "<<sharename<<" with value "<<value<<endl;
  }
}
//creates section like this in config
/*
[sharename]
*/
void cmdAdd(string sharename) {
  share_t st;
  st.sharename=sharename;
  st.writeback=true;
  shares.push_back(st);
  if(debug_command_result)
    cerr<<"Created share "<<sharename<<endl;
}
//deletes section like that created above and all parameters
void cmdDel(string sharename) {
  int sid=sharenametoid(sharename);
  shares[sid].writeback=false;
  if(debug_command_result)
    cerr<<"Deleted share "<<sharename<<endl;
}
//deletes one parameter from section
void cmdDel(string sharename, string paramname) {
  int sid=sharenametoid(sharename);
  int pid=keytoid(sid,paramname);
  shares[sid].conf[pid].writeback=false;
  if(debug_command_result)
    cerr<<"Deleted parameter "<<sharename<<"."<<paramname<<endl;
}
//reads value from sharename parameter
void cmdGet(string sharename, string paramname) {
  if(debug_command_parameters)
  cerr<<sharename<<" "<<paramname<<endl;
  int sid=sharenametoid(sharename);
  int pid=keytoid(sid,paramname);
    cout<<shares[sid].conf[pid].v<<endl;
}

//does processing of arguments
//i have no idea how the fuck this works

string lastshare="";

void processshare(string& s) {
  if(s == "" && lastshare != "")
    s=lastshare;
  else
    lastshare=s;
}

int process(string cmd, string param) {
  if(cmd == "setwriteback") {
    if(param == "true")  {
      writeback=true;
      if(debug_command_result)
        cout<<"Writeback set to TRUE, at end of execution config will be saved"<<endl;
    }
    if(param == "false")  {
      writeback=false;
      if(debug_command_result)
        cout<<"Writeback set to FALSE, at end of execution config will NOT be saved"<<endl;
    }
  }
  if(cmd == "set") {
    string sn,pnv,pn,v;
    split(param, ".", sn,pnv);
    processshare(sn);
    split(pnv,"=",pn,v);
    cmdSet(sn,pn,v);
  }
  if(cmd == "get") {
    string sn,pn;
    split(param, ".",sn,pn);
    processshare(sn);
    cmdGet(sn,pn);
  }
  if(cmd == "add") {
    cmdAdd(param);
    lastshare=param;
  }
  if(cmd == "del") {
    string sn,pn;
    try {
      split(param, ".", sn,pn);
      processshare(sn);
      cmdDel(sn,pn);
    }
    catch(SubstrNotFoundException e) {
      cmdDel(param);
    }
  }
  return 1;
}
//re-generates new samba config file
void regen() {
  if(!writeback) {
    cerr<<"WARNING: writeback set to false, refusing to write!"<<endl;
    return;
  }
  string smbconfbak=smbconf+".bak";

  if(rename(smbconf.c_str(),smbconfbak.c_str())) {
    cerr<<"rename( "<<smbconf<<", "<<smbconfbak<<") failed: "<<strerror(errno)<<endl;
    return;
  }
  ofstream wcf;
  wcf.open(smbconf.c_str());
  if(!wcf.is_open()) {
    cerr<<"Failed to open "<<smbconf<<" to write!"<<endl;
    return;
  }
  //write everything
  //looks complicated, but it's simple and complicated at the same time

  //i'll try to comment this now

  //iterate through all shares in our config file
  for(int i=0;i<shares.size();i++) {
    //if writeback is set to false, we have to skip it
    //that's how deleting shares work
    if(shares[i].writeback) {
      //write section header to config
      wcf<<"["<<shares[i].sharename<<"]"<<endl;
      //iterate through all parameters for this section
      for(int j=0;j<shares[i].conf.size();j++) {
        //if writeback is set to false, skip
        if(shares[i].conf[j].writeback)
          //write param=value pair to file
          wcf<<"  "<<shares[i].conf[j].k<<"="<<shares[i].conf[j].v<<endl;
      }
    }
  }
  //finally, close file
  wcf.close();
}
int main(int args, char** argv) {
  if(args < 3) {
    //todo: use gnu DD-like command line arguments
    cerr<<"WARINIG: THIS PROGRAM IS NOT MEANT FOR PRODUCTION USAGE! (YET)"<<endl;
    cerr<<endl;
    cerr<<"Usage: scedit set sharename.paramname=value - set parameter in share"<<endl;
    cerr<<"              get sharename.paramname       - get parameter in share (*)"<<endl;
    cerr<<"              add sharename                 - create share"<<endl;
    cerr<<"              del sharename                 - delete share"<<endl;
    cerr<<"              del sharename.paramname       - delete parameter from share"<<endl;
    cerr<<"              f   filename                  - execute commands from file filename"<<endl;
    cerr<<endl;
    cerr<<"(*) using AWK, GREP and SED in some combination probably would be and faster for scripting"<<endl;
    cerr<<endl;
    cerr<<"scedit Copyright (C) 2019 Łukasz Konrad Moskała"<<endl;
    cerr<<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
    cerr<<"This is free software, and you are welcome to redistribute it"<<endl;
    cerr<<"under certain conditions; Read attached license file for details."<<endl;
  }
  else {
    string c(argv[1]);
    string v(argv[2]);
    ifstream smbconffile;
    smbconffile.open(smbconf.c_str());
    if(!smbconffile.is_open()) {
      cerr<<"ERROR: Failed to open "<<smbconf<<endl;
      return 1;
    }
    if(debug_input)
      cerr<<"INPUT: Opened samba config file "<<smbconf<<endl;
    int currentshare=-1;
    //this parses config file
    while(smbconffile.good()) {
      string s;
      getline(smbconffile,s);
      char c=firstNonWhitespaceCharacter(s);
      if(c == '#' || c == ';') continue; //skip comments
      if(c == '[') //beginning of section
      {
        currentshare++;
        share_t st;
        s=stripWhitespaces(s); //before anything else
        s=s.substr(1,s.length()-2); //remote ']' from string
        if(debug_input)
          cerr<<"INPUT: using section "<<s<<endl;
        st.sharename=s;
        st.writeback=true;
        shares.push_back(st);
      }
      else
      {
        //we'r not using split here, because here a lot exceptions may be thrown
        //and that's possible performance problem
        try {
          pair_t p;
          p.writeback=true;
          split(s,"=",p.k,p.v);
          //remove leading and trailing whitespaces
          p.k=stripLeadingWhitespaces(p.k);
          p.k=stripTailingWhitespaces(p.k);
          p.v=stripLeadingWhitespaces(p.v);
          p.v=stripTailingWhitespaces(p.v);

          shares[currentshare].conf.push_back(p);
          if(debug_input)
            cerr<<"INPUT: "<<shares[currentshare].sharename<<"."<<p.k<<"="<<p.v<<endl;
        }
        catch(SubstrNotFoundException e) {
          //not a valid line, ignore exception and continue
          //program execution because it may happen
          //for many reasons, eg empty line in file
        }
      }
    }
    if(c != "f") {
      process(c,v);
      //only get command doesn't require to regenerate file
      if(c != "get")
        regen();
    }
    else {
      ifstream commands;
      commands.open(v.c_str());
      if(!commands.is_open()) {
        cerr<<"Failed to open file "<<v<<endl;
        return 1;
      }
      //maybe add stdin support?
      while(commands.good()) {
        string s;
        getline(commands, s);
        if(s.length() == 0 || firstNonWhitespaceCharacter(s) == '#')
          continue;
        split(s, " ", c, v);
        if(debug_script) //endl at beginning to improve readability
          cerr<<endl<<"SCRIPT> "<<c<<" "<<v<<endl;
        process(c,v);
      }
      //we'r assuming that there was set/add/del command used and regenerating file
      regen();
    }
  }

}
