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

bool debug_input=false;
bool debug_command_parameters=false;
bool debug_command_result=true;

bool isWhistespace(char c) {
  return (c == '\t' || c == '\n' || c == '\r' || c == ' ');
}
char firstNonWhitespaceCharacter(string s) {
  for(int i=0;i<s.size();i++) {
    if(!isWhistespace(s[i])) return s[i];
  }
  return 0;
}
struct pair_t {
  string k;
  string v;
  bool writeback=true;
};
struct section_t {
  string section;
  bool writeback=true;
  vector <pair_t> conf;
};
vector <section_t> sections;

int sharenametoid(string sharename) {
  for(int i=0;i<sections.size();i++) {
    if(sections[i].section == sharename) return i;
  }
  return -1;
}
int paramnametoid(int sectionid, string paramname) {
  for(int i=0;i<sections[sectionid].conf.size();i++) {
    if(sections[sectionid].conf[i].k == paramname) return i;
  }
  return -1;
}
void cmdSet(string sharename, string paramname, string value) {
  if(debug_command_parameters)
  cerr<<sharename<<" "<<paramname<<" "<<value<<endl;
  int sid=sharenametoid(sharename);
  int pid=paramnametoid(sid,paramname);
  if(pid == -1) {
    //create parameter
    pair_t p;
    p.k=paramname;
    p.v=value;
    p.writeback=true;
    sections[sid].conf.push_back(p);
    if(debug_command_result)
    cerr<<"Created parameter "<<paramname<<" in section "<<sharename<<" with value "<<value<<endl;
  }
  else {
    sections[sid].conf[pid].v=value;
    sections[sid].conf[pid].writeback=true;
    if(debug_command_result)
    cerr<<"Edited parameter "<<paramname<<" in section "<<sharename<<" with value "<<value<<endl;
  }
}
void cmdAdd(string sectionname) {
  section_t st;
  st.section=sectionname;
  st.writeback=true;
  sections.push_back(st);
  if(debug_command_result)
  cerr<<"Created share "<<sectionname<<endl;
}
void cmdDel(string sectionname) {
  int sid=sharenametoid(sectionname);
  sections[sid].writeback=false;
  if(debug_command_result)
  cerr<<"Deleted share "<<sectionname<<endl;
}
void cmdDel(string sectionname, string paramname) {
  int sid=sharenametoid(sectionname);
  int pid=paramnametoid(sid,paramname);
  sections[sid].conf[pid].writeback=false;
  if(debug_command_result)
  cerr<<"Deleted parameter "<<sectionname<<"."<<paramname<<endl;
}
void cmdGet(string sharename, string paramname) {
  if(debug_command_parameters)
  cerr<<sharename<<" "<<paramname<<endl;
  int sid=sharenametoid(sharename);
  int pid=paramnametoid(sid,paramname);
  if(pid == -1) {
    //CHUUUUUJ
    if(debug_command_result)
    cerr<<"Sram psa jak sra"<<endl;
  }
  else {
    cout<<sections[sid].conf[pid].v<<endl;
  }
}

int process(string cmd, string param) {
  if(cmd == "set") {
    int dp=param.find(".");
    string sn=param.substr(0,dp);
    string pnv=param.substr(dp+1);
    int ep=pnv.find("=");
    string pn=pnv.substr(0,ep);
    string v=pnv.substr(ep+1);
    cmdSet(sn,pn,v);
  }
  if(cmd == "get") {
    int dp=param.find(".");
    string sn=param.substr(0,dp);
    string pn=param.substr(dp+1);
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
void regen() {
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
    cerr<<"Usage: scedit set sharename.paramname=value - set parameter in share"<<endl;
    cerr<<"              get sharename.paramname       - get parameter in share"<<endl;
    cerr<<"              add sharename                 - create share"<<endl;
    cerr<<"              del sharename                 - delete share"<<endl;
    cerr<<"              del sharename.paramname       - delete parameter from share"<<endl;
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
    smbconffile.open("/etc/samba/smb.conf");
    if(!smbconffile.is_open()) {
      cerr<<"ERROR!"<<endl;
      return 1;
    }
    int currentsection=-1;
    while(smbconffile.good()) {
      string s;
      getline(smbconffile,s);
      char c=firstNonWhitespaceCharacter(s);
      if(c == '#' || c == ';') continue;
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
