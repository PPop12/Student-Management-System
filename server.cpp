// Student Management System - C++ HTTP server (Windows / Winsock)
// Build: g++ server.cpp -std=c++17 -o server.exe -lws2_32
// Run:   server.exe
// Open:  http://localhost:8080

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "Ws2_32.lib")
using namespace std;

struct Student {
    string roll, name, department;
    double cgpa = 0;
    int attendance = 0;
    Student* next = nullptr;
};

class StudentList {
    Student* head = nullptr;
    const string filename = "students.csv";

    static string csvEscape(const string& s) {
        string out = "\"";
        for (char c : s) { if (c == '"') out += "\"\""; else out += c; }
        return out + "\"";
    }
    static vector<string> splitCSV(const string& line) {
        vector<string> fields; string cur; bool quoted = false;
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '"') {
                if (quoted && i + 1 < line.size() && line[i+1] == '"') { cur += '"'; ++i; }
                else quoted = !quoted;
            } else if (c == ',' && !quoted) { fields.push_back(cur); cur.clear(); }
            else cur += c;
        }
        fields.push_back(cur);
        return fields;
    }
public:
    ~StudentList() { clear(); }
    void clear() { while (head) { Student* t=head; head=head->next; delete t; } }
    bool exists(const string& roll) const {
        for (Student* p=head; p; p=p->next) if (p->roll==roll) return true;
        return false;
    }
    bool add(const Student& s) {
        if (exists(s.roll)) return false;
        Student* n = new Student{s.roll,s.name,s.department,s.cgpa,s.attendance,nullptr};
        if (!head) head=n;
        else { Student* p=head; while(p->next) p=p->next; p->next=n; }
        save(); return true;
    }
    bool remove(const string& roll) {
        Student* p=head; Student* prev=nullptr;
        while(p && p->roll!=roll) { prev=p; p=p->next; }
        if(!p) return false;
        if(prev) prev->next=p->next; else head=p->next;
        delete p; save(); return true;
    }
    vector<Student> getAll() const {
        vector<Student> v;
        for(Student* p=head;p;p=p->next) v.push_back(*p);
        return v;
    }
    vector<Student> search(const string& q) const {
        vector<Student> v;
        for(Student* p=head;p;p=p->next)
            if(p->roll.find(q)!=string::npos || p->name.find(q)!=string::npos) v.push_back(*p);
        return v;
    }
    vector<Student> sorted(const string& by) const {
        auto v=getAll();
        if(by=="name") sort(v.begin(),v.end(),[](const Student&a,const Student&b){return a.name<b.name;});
        else if(by=="attendance") sort(v.begin(),v.end(),[](const Student&a,const Student&b){return a.attendance>b.attendance;});
        else sort(v.begin(),v.end(),[](const Student&a,const Student&b){return a.cgpa>b.cgpa;});
        return v;
    }
    void load() {
        ifstream in(filename); if(!in) return;
        string line; getline(in,line); // header
        while(getline(in,line)) {
            auto f=splitCSV(line); if(f.size()<5) continue;
            try {
                Student s{f[0],f[1],f[2],stod(f[3]),stoi(f[4]),nullptr};
                if(!exists(s.roll)) {
                    Student* n=new Student{s.roll,s.name,s.department,s.cgpa,s.attendance,nullptr};
                    if(!head) head=n; else {Student*p=head;while(p->next)p=p->next;p->next=n;}
                }
            } catch(...) {}
        }
    }
    void save() const {
        ofstream out(filename);
        out << "roll,name,department,cgpa,attendance\n";
        for(Student*p=head;p;p=p->next)
            out << csvEscape(p->roll) << ',' << csvEscape(p->name) << ','
                << csvEscape(p->department) << ',' << p->cgpa << ',' << p->attendance << '\n';
    }
};

static string decode(const string& s) {
    string o;
    for(size_t i=0;i<s.size();++i) {
        if(s[i]=='+') o+=' ';
        else if(s[i]=='%' && i+2<s.size()) {
            try { o+=(char)stoi(s.substr(i+1,2),nullptr,16); i+=2; } catch(...) { o+=s[i]; }
        } else o+=s[i];
    }
    return o;
}
static string param(const string& data,const string& key) {
    string token=key+"="; size_t p=data.find(token);
    if(p==string::npos) return "";
    p+=token.size(); size_t e=data.find('&',p);
    return decode(data.substr(p,e==string::npos?string::npos:e-p));
}
static string jsonEscape(const string& s) {
    string o;
    for(char c:s) { if(c=='"'||c=='\\') o+='\\'; if(c=='\n') o+="\\n"; else o+=c; }
    return o;
}
static string toJSON(const vector<Student>& v) {
    ostringstream o; o << "[";
    for(size_t i=0;i<v.size();++i) {
        if(i) o << ",";
        o << "{\"roll\":\""<<jsonEscape(v[i].roll)<<"\",\"name\":\""<<jsonEscape(v[i].name)
          <<"\",\"department\":\""<<jsonEscape(v[i].department)<<"\",\"cgpa\":"<<v[i].cgpa
          <<",\"attendance\":"<<v[i].attendance<<"}";
    }
    o << "]"; return o.str();
}
static string readFile(const string& path) {
    ifstream f(path,ios::binary); if(!f) return "";
    stringstream b; b<<f.rdbuf(); return b.str();
}
static void respond(SOCKET client,const string& status,const string& type,const string& body) {
    string header="HTTP/1.1 "+status+"\r\nContent-Type: "+type+"\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Length: "+to_string(body.size())+"\r\n\r\n";
    string all=header+body; send(client,all.c_str(),(int)all.size(),0);
}

int main() {
    WSADATA wsa; if(WSAStartup(MAKEWORD(2,2),&wsa)!=0) { cerr<<"Winsock startup failed\n"; return 1; }
    StudentList db; db.load();
    SOCKET server=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(8080); addr.sin_addr.s_addr=INADDR_ANY;
    int opt=1; setsockopt(server,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt));
    if(bind(server,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR || listen(server,10)==SOCKET_ERROR) {
        cerr<<"Could not start server on port 8080. Close any app using that port.\n"; closesocket(server); WSACleanup(); return 1;
    }
    cout<<"Server running at http://localhost:8080\nPress Ctrl+C to stop.\n";
    while(true) {
        SOCKET client=accept(server,nullptr,nullptr); if(client==INVALID_SOCKET) continue;
        string req; char buf[4096]; int n;
        while((n=recv(client,buf,sizeof(buf),0))>0) { req.append(buf,n); if(req.find("\r\n\r\n")!=string::npos) {
            size_t h=req.find("\r\n\r\n"); size_t clp=req.find("Content-Length:");
            int contentLen=0;
            if(clp!=string::npos && clp<h) { size_t e=req.find("\r\n",clp); contentLen=atoi(req.substr(clp+15,e-clp-15).c_str()); }
            size_t bodyStart=h+4;
            if(req.size()>=bodyStart+(size_t)contentLen) break;
        }}
        size_t lineEnd=req.find("\r\n"); string first=req.substr(0,lineEnd);
        string method, target; {istringstream is(first); is>>method>>target;}
        size_t qpos=target.find('?'); string path=target.substr(0,qpos);
        string query=qpos==string::npos?"":target.substr(qpos+1);
        size_t split=req.find("\r\n\r\n"); string body=split==string::npos?"":req.substr(split+4);
        if(method=="GET" && path=="/") {
            string html=readFile("index.html"); respond(client,"200 OK","text/html; charset=utf-8",html);
        } else if(method=="GET" && path=="/style.css") respond(client,"200 OK","text/css",readFile("style.css"));
        else if(method=="GET" && path=="/app.js") respond(client,"200 OK","application/javascript",readFile("app.js"));
        else if(method=="GET" && path=="/api/students") respond(client,"200 OK","application/json",toJSON(db.getAll()));
        else if(method=="GET" && path=="/api/search") respond(client,"200 OK","application/json",toJSON(db.search(param(query,"q"))));
        else if(method=="GET" && path=="/api/sort") respond(client,"200 OK","application/json",toJSON(db.sorted(param(query,"by"))));
        else if(method=="POST" && path=="/api/add") {
            Student s{param(body,"roll"),param(body,"name"),param(body,"department"),0,0,nullptr};
            try { s.cgpa=stod(param(body,"cgpa")); s.attendance=stoi(param(body,"attendance")); } catch(...) {}
            bool valid=!s.roll.empty()&&!s.name.empty()&&!s.department.empty()&&s.cgpa>=0&&s.cgpa<=10&&s.attendance>=0&&s.attendance<=100;
            if(!valid) respond(client,"400 Bad Request","application/json","{\"ok\":false,\"message\":\"Enter valid details. CGPA: 0-10; attendance: 0-100.\"}");
            else if(!db.add(s)) respond(client,"409 Conflict","application/json","{\"ok\":false,\"message\":\"That roll number already exists.\"}");
            else respond(client,"200 OK","application/json","{\"ok\":true,\"message\":\"Student added.\"}");
        } else if(method=="POST" && path=="/api/delete") {
            if(db.remove(param(body,"roll"))) respond(client,"200 OK","application/json","{\"ok\":true,\"message\":\"Student deleted.\"}");
            else respond(client,"404 Not Found","application/json","{\"ok\":false,\"message\":\"Roll number not found.\"}");
        } else respond(client,"404 Not Found","text/plain","Not found");
        closesocket(client);
    }
    closesocket(server); WSACleanup(); return 0;
}
