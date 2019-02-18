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
#include <regex>

using namespace std;


#include "exceptions.hpp"

/*
  I'm very sorry for this program. Comments are poor, and naming isn't consistent
  But i'm not going to use it in production enviromment and neither should you!
  I need it for ONE script for ONE exam in school, and my teacher will probably
  never look up source code of this program, so I don't give a fuck about that

  If you'r in situation like me (that is, you have exam that requires you to
  write script to set and remove network shares), then you might want to use
  this program. You should not anyway, and in production enviromment editing
  samba config file by hand is much better idea
*/

//verbosity control
//there is no way of changing this at run-time, so... yeah...
//but who cares, i'll forget about this program in a week after exam and
//probably never use it again
bool debug_input=false;
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
//pair of key=value
//if writeback is set to false, this entry will be skipped when
//writing config file
struct pair_t {
  string k;
  string v;
  bool writeback=true;
};
//used to store sections and it's key=value pairs
//writeback works as above
struct section_t {
  string section;
  bool writeback=true;
  vector <pair_t> conf;
};
//we have to store this somewhere, don't we?
vector <section_t> sections;

//converts share name to it's index in sections vector
int sharenametoid(string sharename) {
  for(int i=0;i<sections.size();i++) {
    if(sections[i].section == sharename) return i;
  }
  throw smbsnfexception;
}

//converts key to it's index in sections->conf vector
int paramnametoid(int sectionid, string paramname) {
  for(int i=0;i<sections[sectionid].conf.size();i++) {
    if(sections[sectionid].conf[i].k == paramname) return i;
  }
  throw pnfexception;
}

//sets config like this
/*
[sharename]
  paramname=value
*/
void cmdSet(string sharename, string paramname, string value) {
  if(debug_command_parameters)
  cerr<<sharename<<" "<<paramname<<" "<<value<<endl;
  int sid=sharenametoid(sharename);
  try {
    int pid=paramnametoid(sid,paramname);
    //if following code executes, no exception were thrown
    sections[sid].conf[pid].v=value;
    sections[sid].conf[pid].writeback=true;
    if(debug_command_result)
      cerr<<"Edited parameter "<<paramname<<" in section "<<sharename<<" with value "<<value<<endl;
  }
  catch(ParamNotFoundException e) {
    //parameter doesn't exist, so create it instead of editing
    pair_t p;
    p.k=paramname;
    p.v=value;
    p.writeback=true;
    sections[sid].conf.push_back(p);
    if(debug_command_result)
      cerr<<"Created parameter "<<paramname<<" in section "<<sharename<<" with value "<<value<<endl;
  }
}
//creates section like this in config
/*
[sectionname]
*/
void cmdAdd(string sectionname) {
  section_t st;
  st.section=sectionname;
  st.writeback=true;
  sections.push_back(st);
  if(debug_command_result)
    cerr<<"Created share "<<sectionname<<endl;
}
//deletes section like that created above and all parameters
void cmdDel(string sectionname) {
  int sid=sharenametoid(sectionname);
  sections[sid].writeback=false;
  if(debug_command_result)
    cerr<<"Deleted share "<<sectionname<<endl;
}
//deletes one parameter from section
void cmdDel(string sectionname, string paramname) {
  int sid=sharenametoid(sectionname);
  int pid=paramnametoid(sid,paramname);
  sections[sid].conf[pid].writeback=false;
  if(debug_command_result)
    cerr<<"Deleted parameter "<<sectionname<<"."<<paramname<<endl;
}
//reads value from sharename parameter
void cmdGet(string sharename, string paramname) {
  if(debug_command_parameters)
  cerr<<sharename<<" "<<paramname<<endl;
  int sid=sharenametoid(sharename);
  int pid=paramnametoid(sid,paramname);
    cout<<sections[sid].conf[pid].v<<endl;
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
  if(rename("/etc/samba/smb.conf","/etc/samba/smb.conf.bak")) {
    cerr<<"rename( /etc/samba/smb.conf, /etc/samba/smb.conf.bak) failed: "<<strerror(errno)<<endl;
    return;
  }
  ofstream wcf;
  wcf.open("/etc/samba/smb.conf");
  if(!wcf.is_open()) {
    cerr<<"Failed to open /etc/samba/smb.conf to write!"<<endl;
    return;
  }
  //write everything
  //looks complicated, but it's simple and complicated at the same time
  for(int i=0;i<sections.size();i++) {
    if(sections[i].writeback) {
      wcf<<"["<<sections[i].section<<"]"<<endl;
      for(int j=0;j<sections[i].conf.size();j++) {
        if(sections[i].conf[j].writeback)
          wcf<<"  "<<sections[i].conf[j].k<<"="<<sections[i].conf[j].v<<endl;
      }
    }
  }
  wcf.close();
}
int main(int args, char** argv) {
  if(args < 3) {
    cerr<<"WARINIG: THIS PROGRAM IS NOT MEANT FOR PRODUCTION USAGE!"<<endl;
    cerr<<"IT MAY COMPLETELY FUCK YOUR SAMBA CONFIG!"<<endl;
    cerr<<"I MADE IT BECAUSE I NEEDED TO HAVE A WAY TO SCRIPT CREATING SMB SHARES"<<endl;
    cerr<<"FOR MY EXAM! YOU SHOULD NOT BE USING IT AT ALL!"<<endl;
    cerr<<endl;
    cerr<<"PROGRAM DOESN'T GIVE A FUCK ABOUT YOU'R COMMENTS! (and will delete them)"<<endl;
    cerr<<endl;
    cerr<<"Usage: scedit set sharename.paramname=value - set parameter in share"<<endl;
    cerr<<"              get sharename.paramname       - get parameter in share (*)"<<endl;
    cerr<<"              add sharename                 - create share"<<endl;
    cerr<<"              del sharename                 - delete share"<<endl;
    cerr<<"              del sharename.paramname       - delete parameter from share"<<endl;
    cerr<<endl;
    cerr<<"(*) using AWK, GREP and SED in some combination probably would be and faster for scripting"<<endl;
    cerr<<endl;
    cerr<<"scedit Copyright (C) 2019 Łukasz Konrad Moskała"<<endl;
    cerr<<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
    cerr<<"This is free software, and you are welcome to redistribute it"<<endl;
    cerr<<"under certain conditions; Read attached license file for details."<<endl;
  }
  else {
    //this message is just for fun (my teacher will see it when he runs my script)
    //greetings, Mr. Grzegorz
    cerr<<"Starting SCEDIT Copyright (C) 2019 Łukasz Konrad Moskała"<<endl;
    string c(argv[1]);
    string v(argv[2]);
    ifstream smbconffile;
    smbconffile.open("/etc/samba/smb.conf");
    if(!smbconffile.is_open()) {
      cerr<<"ERROR!"<<endl;
      return 1;
    }
    int currentsection=-1;
    //this parses config file
    //somehow works, but if your config sections look like this
    //[ name ]
    //not like this
    //[name]
    //you'r in trouble. Mine doesn't so it works for me
    while(smbconffile.good()) {
      string s;
      getline(smbconffile,s);
      char c=firstNonWhitespaceCharacter(s);
      if(c == '#' || c == ';') continue; //skip comments
      if(c == '[') //beginning of section
      {
        currentsection++;
        section_t st;
        s=s.substr(1,s.length()-2);
        if(debug_input)
        cerr<<"DBG: using section "<<s<<endl;
        st.section=s;
        st.writeback=true;
        sections.push_back(st);
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
          c=std::regex_replace(c, std::regex("^ +| +$|( ) +"), "$1");
          v=std::regex_replace(v, std::regex("^ +| +$|( ) +"), "$1");
          pair_t p;
          p.k=c;
          p.v=v;
          p.writeback=true;
          sections[currentsection].conf.push_back(p);
          if(debug_input)
          cerr<<"DBG: "<<sections[currentsection].section<<"."<<c<<"="<<v<<endl;
        }
      }
    }
    process(c,v);
    if(c != "get")
    regen();
  }

}
