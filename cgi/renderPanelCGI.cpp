#include <iostream>
#include <vector>
#include <sys/types.h>
#include <dirent.h>
#include <boost/algorithm/string/predicate.hpp>
#define HOSTNUM 12
using std::cout;
using std::endl;

void renderTable(int ,std::vector<std::string> *,std::vector<std::string> *,std::string); 
void getTestCase(std::vector<std::string> *);

int main(){
    int n_servers = 5;
    std::string form_action = "console.cgi";
    std::string form_method = "GET";
    std::string domain = ".cs.nctu.edu.tw";
    std::vector<std::string> hosts_list;
    hosts_list.reserve(HOSTNUM);
    for(int i = 0;i < HOSTNUM;i++)
        hosts_list.push_back("nplinux"+std::to_string(i+1));

    std::vector<std::string> test_cases;
    getTestCase(&test_cases);   

    //cout << "HTTP/1.1 200 OK" << endl;
    cout << "Content-type: text/html" << endl;
    cout << endl;
    cout << "<!DOCTYPE html>" << endl;
    cout << "<html lang=\"en\">" << endl;
    cout << "<head>" << endl;
    cout << "<title>NP Project 3 Panel</title>" << endl;
    cout << "<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\" "
         << "integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\" crossorigin=\"anonymous\" />"<< endl;
    
    cout << "<link href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\" rel=\"stylesheet\" />" << endl;
    cout << "<link rel=\"icon\" type=\"image/png\""
         << " href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"  />" << endl;
    cout << "<style>" << endl;
    cout << "* {" << endl;
    cout << "font-family: 'Source Code Pro', monospace;" << endl;
    cout << "  }" << endl;
    cout << "</style>" << endl;
    cout << "</head>" << endl;

    cout << "<body class=\"bg-secondary pt-5\">" << endl;
    cout << "<form action=\"" << form_action << "\" method=\"" << form_method << "\">" << endl;
    cout << "<table class=\"table mx-auto bg-light\" style=\"width: inherit\">" << endl;
    cout << "<thread class=\"thead-dark\">" << endl;
    cout << "<tr>" << endl;
    cout << "<th scope=\"col\">#</th>" << endl;
    cout << "<th scope=\"col\">Host</th>" << endl;
    cout << "<th scope=\"col\">Port</th>" << endl;
    cout << "<th scope=\"col\">Input File</th>" << endl;
    cout << "</tr>" << endl;
    cout << "</thread>" << endl;
    
    renderTable(n_servers, &hosts_list, &test_cases, domain);
 
    cout << "</table>" << endl;
    cout << "</form>" << endl;   
    cout << "</body>" << endl;
    cout << "</html>" << endl;
    return 0;
}

void renderTable(int n_servers, std::vector<std::string> *h_list, std::vector<std::string> *t_cases, std::string domain){
    cout << "<tbody>" << endl;
    for(int i = 0;i < n_servers;i++){
        cout << "<tr>" << endl;
        cout << "<th scope=\"row\" class=\"align-middle\">Session " << i+1 << "</th>" << endl;
        cout << "<td>" << endl;
        cout << "<div class=\"input-group\">" << endl;
        cout << "<select name=\"h" << i << "\" class=\"custom-select\">" << endl;
     
        cout << "<option></option>" << endl;
        for(auto itr = (*h_list).begin();itr != (*h_list).end(); itr++)
            cout << "<option value=\"" << (*itr)+domain << "\">" << (*itr) << "</option>";
        
        cout << "</select>" << endl;
        cout << "<div class=\"input-group-append\">" << endl;
        cout << "<span class=\"input-group-text\">" << domain << "</span>" << endl;
        cout << "</div>" << endl;
        cout << "</div>" << endl;
        cout << "</td>" << endl;

        cout << "<td>" << endl;
        cout << "<input name=\"p" << i << "\" type=\"text\" class=\"form-control\" size=\"5\" />" << endl;
        cout << "</td>" << endl;
        
        cout << "<td>" << endl;
        cout << "<select name=\"f" << i << "\" class=\"custom-select\">" << endl;
        
        cout << "<option></option>" << endl;
        for(auto itr = (*t_cases).begin();itr != (*t_cases).end();itr++)
            cout << "<option value=\"" << (*itr) << "\">" << (*itr) << "</option>";

        cout << "</select>" << endl;
        cout << "</td>" << endl;
        cout << "</tr>" << endl;
    }
    cout << "<tr>" << endl;
    cout << "<td colspan=\"3\"></td>" << endl;
    cout << "<td>" << endl;
    cout << "<button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>" << endl;
    cout << "</td>" << endl;   
    cout << "</tr>" << endl;
    cout << "</tbody>" << endl;
}

void getTestCase(std::vector<std::string> *vec){
    DIR *dir;
    struct dirent *ent;
    if((dir = opendir("./test_case/")) != NULL){
        while((ent = readdir(dir)) != NULL){
            std::string tmp = ent->d_name;
            if(boost::algorithm::ends_with(tmp,".txt"))
                (*vec).push_back(tmp);
        }
        closedir(dir);
    }
    else{
      std::cerr << "Error: Cannot open test_case" << endl;
    }
}
