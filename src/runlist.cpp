#include "global.h"
#include <fstream>
#include <cstring>

std::string readLine(std::fstream & f){
  std::string aux;
  std:size_t pos;
  getline(f, aux);
  pos = aux.find('>');
  pos++;
  return aux.substr(pos, aux.size() - pos - 5);
}

bool readAttemp(std::fstream &f, Attempt &novo){
  std::string aux;
  Settings settings;

  getline(f, aux); //  <tr>
  if(aux == "</tbody></table>") return 0;
  novo.id = 1000 + to<int>(readLine(f)); //  <td><b>#</b></td> (numero do attemp)
  getline(f, aux); //   <td><b>Site</b></td>

  strcpy(novo.team, readLine(f).c_str()); //   <td><b>User</b></td>
  novo.when =  settings.begin + 60*(to<int>(readLine(f))); //  <td><b>Time</b></td>
  novo.problem = readLine(f)[0] - 'A'; // <td><b>Problem</b></td>

  getline(f, aux); // <td><b>Language</b></td>
  getline(f, aux); // <td><b>Filename</b></td>
  getline(f, aux); // <td><b>Status</b></td>
  getline(f, aux); // <td><b>Judge (Site)</b></td>

  novo.verdict = (readLine(f)[0] == 'Y') ? AC : WA; // <td><b>Answer</b></td>

  getline(f, aux);

  return 1; 
}

namespace Runlist{
  void load(){
    std::fstream f("runlist/runlist.html");
    if (!f.is_open()) return;
    
    FILE *fp = fopen("attempts.bin", "wb");
    if(!fp) return;

    std::string aux;
    while(getline(f, aux))
      if(aux.find("Answer") != std::string::npos)
        break;

    getline(f, aux);
    
    Attempt tmp;
    while(readAttemp(f, tmp)){
      fwrite(&tmp, sizeof(Attempt), 1, fp);
    }
      // fprintf(fp, "%d \"%s\" %c %d\n", (int)tmp.when, tmp.team, tmp.problem, tmp.verdict);

    fclose(fp);
  }
}
