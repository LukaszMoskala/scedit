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
int process(string cmd, string param) {
  if(cmd == "set") {
    int dp=param.find(".");
    string sn=param.substr(0,dp); // get share name
    string pnv=param.substr(dp+1); //get key=value
    int ep=pnv.find("=");
    string pn=pnv.substr(0,ep); //   get key
    string v=pnv.substr(ep+1);  //   get value
    cmdSet(sn,pn,v);
  }
  if(cmd == "get") {
    int dp=param.find(".");
    string sn=param.substr(0,dp); //get share name
    string pn=param.substr(dp+1); //get key
    cmdGet(sn,pn);
  }
  if(cmd == "add") {
    cmdAdd(param);
  }
  if(cmd == "del") {
    int dp=param.find(".");
    if(dp != -1) {
      string sn=param.substr(0,dp);
      string pn=param.substr(dp+1);
      cmdDel(sn,pn);
    }
    else {
      cmdDel(param);
    }
  }
  return 1;
}
//re-generates new samba config file
void regen() {
  //lets make a backup, we'll probably need it because chances that something goes
  //terribly wrong, are high
  //TODO: pass file from command line
  if(rename("/etc/samba/smb.conf","/etc/samba/smb.conf.bak")) {
    cerr<<"rename( /etc/samba/smb.conf, /etc/samba/smb.conf.bak) failed: "<<strerror(errno)<<endl;
    return;
  }
  ofstream wcf;
  //TODO: pass file from command line
  wcf.open("/etc/samba/smb.conf");
  if(!wcf.is_open()) {
    cerr<<"Failed to open /etc/samba/smb.conf to write!"<<endl;
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
    //TODO: command line option to set config file
    smbconffile.open("/etc/samba/smb.conf");
    if(!smbconffile.is_open()) {
      cerr<<"ERROR: Failed to open smb.conf"<<endl;
      return 1;
    }
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
        s=s.substr(1,s.length()-2);
        if(debug_input)
          cerr<<"INPUT: using section "<<s<<endl;
        st.sharename=s;
        st.writeback=true;
        shares.push_back(st);
      }
      else
      {
        int ep=s.find("=");
        if(ep != -1) {
          string c=s.substr(0,ep);
          string v=s.substr(ep+1);
          //remove leading and trailing whitespaces
          //copied from stack overflow, i have no idea how that works
          //increases compilation time by 3 seconds, maybe should be moved to another statically-linked file?
          //WARINING: If you have double-space in path, or any parameter, then you'r fucked

          //TODO: implement without using regex
          c=stripLeadingWhitespaces(c);
          c=stripTailingWhitespaces(c);

          v=stripLeadingWhitespaces(v);
          v=stripTailingWhitespaces(v);

          pair_t p;
          p.k=c;
          p.v=v;
          p.writeback=true;
          shares[currentshare].conf.push_back(p);
          if(debug_input)
            cerr<<"INPUT: "<<shares[currentshare].sharename<<"."<<c<<"="<<v<<endl;
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
        if(s.length() == 0 || s[0] == '#')
          continue;
        //first space seperates command from data
        int space=s.find(" ");
        //split skipping that space
        c=s.substr(0,space);
        v=s.substr(space+1);
        process(c,v);
      }
      //we'r assuming that there was set/add/del command used and regenerating file
      regen();
    }
  }

}
