#include "parser.h"

StringParser::StringParser(const std::string input) : input(input),length(input.length()){
    next = 0;
    last = 0;
    lastOp = VALID;
    line = 0;
    lastline = 0;
}

size_t StringParser::remaining() const{
    return length-next;
}

std::string StringParser::advanceLine(){
    if(remaining()==0){
        lastOp = ENDOFFILE;
        return "";
    }
    std::string line = advanceToChar('\n');
    lastOp = VALID;
    return line;
}

std::string StringParser::advanceToChar(const char c, const bool advance){
    if(remaining()==0){
        lastOp = ENDOFFILE;
        return "";
    }
    last = next;
    while(true){
        next++;
        char cn = input[next-1];

        if(cn=='\n') line++;

        if(cn==c){
            lastOp = VALID;
            break;
        }
        if(!advance&&cn=='\n'){
            lastOp = ENDOFLINE;
            break;
        }
        else if(next==length){
            lastOp = ENDOFFILE;
            return input.substr(last,next-last);
        }
    }
    return input.substr(last,next-last-1);
}

std::string StringParser::advanceBlock(const char open, const char close, const bool advance){
    if(remaining()==0){
        lastOp = ENDOFFILE;
        return "";
    }
    unsigned int opened = 1;
    last = next;
    while(true){
        next++;
        char cn = input[next-1];

        if(cn=='\n') line++;

        if(cn==open){
            opened++;
        }
        else if(cn==close){
            opened--;
            if(opened==0){
                lastOp = VALID;
                break;
            }
        }
        else if(!advance&&cn=='\n'){
            lastOp = ENDOFLINE;
            break;
        }
        else if(next==length){
            lastOp = ENDOFFILE;
            return input.substr(last,next-last);
        }
    }
    return input.substr(last,next-last-1);
}

bool StringParser::goBack(){
    if(next==last) return false;
    next = last;
    line = lastline;
    return true;
}

std::string trim(const std::string in){
    if(in.empty()) return "";
    size_t start = 0;
    for(size_t i = 0; i < in.length(); i++){
        if(!isspace(in[i])){
            start = i;
            break;
        }
    }
    size_t end = start-1;
    for(size_t j = in.length()-1; j >= start; j--){
        if(!isspace(in[j])){
            end = j;
            break;
        }
        if(j==0) break;
    }
    return in.substr(start,end-start+1);
}

std::vector<std::string> split(const std::string in, const char c, const bool trimsubstrings){
    if(in.empty()){
        if(trimsubstrings){
            return {};
        }
        return {""};
    }
    size_t i = 0;
    size_t last = 0;
    std::vector<std::string> res;
    while(i <= in.length()){
        if(i==in.length()||c==in[i]){
            std::string str = in.substr(last,i-last);
            last = i+1;
            if(trimsubstrings){
                str = trim(str);
                if(!str.empty()){
                    res.push_back(str);
                }
            }
            else{
                res.push_back(str);
            }
        }
        i++;
    }
    return res;
}

std::vector<std::string> splitWords(const std::string in, const bool trimsubstrings){
    if(in.empty()){
        if(trimsubstrings){
            return {};
        }
        return {""};
    }
    size_t i = 0;
    size_t last = 0;
    std::vector<std::string> res;
    while(i <= in.length()){
        if(i==in.length()||isspace(in[i])){
            std::string str = in.substr(last,i-last);
            last = i+1;
            if(trimsubstrings){
                str = trim(str);
                if(!str.empty()){
                    res.push_back(str);
                }
            }
            else{
                res.push_back(str);
            }
        }
        i++;
    }
    return res;
}

std::string extract(const std::string in){
    if(in.length()>=2){
        return in.substr(1,in.length()-2);
    }
    else return "";
}

bool startsWith(const std::string in, const std::string pattern){
    if(pattern.empty()) return true;
    if(pattern.length()>in.length()) return false;
    
    for(size_t i = 0; i < pattern.length(); i++){
        if(in[i]!=pattern[i]){
            return false;
        }
    }
    return true;
}

unsigned int count(const std::string in, const char c){
    unsigned int n = 0;
    for(size_t i = 0; i < in.length(); i++){
        if(in[i] == c) n++;
    }
    return n;
}

std::string readFile(std::ifstream& file){
    return std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
}

std::string readFileFromPath(const std::string absolutepath){
    std::ifstream file;
    bool read = false;
    std::string content;
    try{
        file.open(absolutepath, std::ios::in);
        if(file.is_open()){
            content = readFile(file);
            read = true;
        }
    }
    catch(...){}
    try{
        file.close();
    }
    catch(...){}
    if(read) return content;
    throw std::runtime_error("Error opening file at \""+absolutepath+"\".");
}

void writeToFile(std::string absolutepath, std::string content, bool append){
    std::ofstream file;
    bool written = false;
    try{
        file.open(absolutepath, std::ios::out | (append ? std::ios::app : std::ios::trunc));
        if(file.is_open()){
            file << content;
            written = true;
        }
    }
    catch(...){}
    try{
        file.close();
    }
    catch(...){}
    if(!written) throw std::runtime_error("Error writing to file at \""+absolutepath+"\".");
}

std::string getPath(const std::string arg, const std::string filelocation){
    std::string rel = trim(arg);
    std::string loc = trim(filelocation);
    if(!startsWith(rel,".")){
        if(startsWith(rel,"/")) return rel;
        else return "/"+rel;
    }
    if(loc.empty()||startsWith(loc,".")) return "";
    if(!startsWith(loc,"/")) loc = "/"+loc;
    std::vector<std::string> relv = split(rel,'/',false);
    std::list<std::string> rell(relv.begin(), relv.end());
    std::vector<std::string> locv = split(loc.substr(1),'/',false);
    locv.pop_back();
    while(!rell.empty()){
        std::string next = trim(rell.front());
        if(next=="."){
            rell.pop_front();
        }
        else if(next == ".."){
            rell.pop_front();
            if(!locv.empty()) locv.pop_back();
        }
        else{
            break;
        }
    }
    std::string pathAbs = "";
    for(size_t i = 0; i < locv.size(); i++){
        pathAbs += "/" + locv[i];
    }
    if(rell.empty()){
        return pathAbs + "/";
    }
    while(!rell.empty()){
        pathAbs += "/" + rell.front();
        rell.pop_front();
    }
    return pathAbs;
}

std::string took(std::chrono::time_point<std::chrono::steady_clock> start){
    auto end = std::chrono::steady_clock::now();
    return took(std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());
}

std::string took(const long us){
    std::string took = "";
    long usec = us;
    if(usec<5000){
        took = std::to_string(usec)+"us";
    }
    else{
        usec = usec/1000;
        if(usec<5000){
            took = std::to_string(usec)+"ms";
        }
        else{
            usec = usec/1000;
            if(usec<300){
                took = std::to_string(usec)+"s";
            }
            else{
                usec = usec/60;
                took = std::to_string(usec)+"min";
            }
        }
    }
    return took;
}

std::string aZToLowerCase(std::string input){
    const char minL = 'a';
    const char max = 'Z';
    const char minU = 'A';
    const char dif = minU - minL;

    std::string res = "";
    for(size_t i = 0; i < input.size(); i++){
        char c = input[i];
        if(c>=minU&&c<=max){
            c = c - dif;
        }
        res += c;
    }

    return res;
}