#include "options.h"

bool isSet(Flag f){
    return (f==SETFALSE)||(f==SETTRUE);
}

std::string flag_string(Flag f){
    if(f==DEFAULT) return "?";
    if(f==SETTRUE||f==TRUE) return "1";
    return "0";
}

Flag setFlag(Flag f){
    if(f==TRUE) return SETTRUE;
    if(f==FALSE) return SETFALSE;
    return f;
}

std::string mode_to_string(int mode){
    switch(mode){
        case 0: return "SEQ - -";
        case 1: return "SEQ ALT";
        case 8: return "TSETS";
        default:;
    }
    bool checkS = (mode>1&&mode<8);
    if(!checkS) return "INVALID";
    bool sinkSVal = (mode%2)==1;
	bool checkWC1 = (mode >= 4);
	bool sinkWC1Val = (mode==6||mode==7);
    std::string res = "SEQ ";
    res += (sinkSVal ? "T " : "F ");
    res += (checkWC1 ? (sinkWC1Val ? "T" : "F") : "-");
    return res;
}

Options::Options(std::vector<std::string>* results){
    timeout = 0;
    lines = {0};
    v = DEFAULT;
    c = DEFAULT;
    o = DEFAULT;
    d = DEFAULT;
    g = 0;
    max = DEFAULT;
    log = DEFAULT;
    s = DEFAULT;
    bighoa = DEFAULT;
    this->results = results;
    reset_data();
    testmodes = std::vector<bool>(MODES,true);
}

bool Options::verbose() const{
    return (v==SETTRUE)||(v==TRUE);//||(v==DEFAULT);
}

bool Options::compare() const{
    return (c==SETTRUE)||(c==TRUE);
}

bool Options::output() const{
    return (o==SETTRUE)||(o==TRUE);
}

bool Options::debug() const{
    return (d==SETTRUE)||(d==TRUE);
}

bool Options::useAlt() const{
    return (g==1);
}

bool Options::canWrite() const{
    return !outputdirectory.empty() || !filelocation.empty();
}

bool Options::useLoopComplexity() const{
    return (max==SETTRUE)||(max==TRUE);
}

bool Options::logdata() const{
    return (log==SETTRUE)||(log==TRUE);
}

bool Options::stats() const{
    return (s==SETTRUE) || (s==TRUE);
}

bool Options::compressHoa() const{
    return (bighoa==SETFALSE||bighoa==FALSE||bighoa==DEFAULT);
}

void Options::writeToLog(std::string data) const{
    if(!logdata()||!canWrite()) return;
    std::string path;
    if(outputdirectory.empty()) path = getPath("./",filelocation);
    else path = outputdirectory;
    path += "log";
    try{
        writeToFile(path,data,true);
    }
    catch(...){};
}

void Options::update(const std::string input){
    std::string in = trim(input);
    std::vector<std::string> inn = split(in,'=',false);
    if(inn.size()==1){
        std::string inn0 = trim(inn[0]);
        if(inn0=="-a"||inn0=="a") throw std::invalid_argument("Lacking mandatory argument.");
        if(inn0=="-k"||inn0=="k"||inn0=="-g"||inn0=="g") return update(inn0+"=");
        return update(inn[0]+"=true");  //default = true
    }
    else if(inn.size()==2){
        std::string inn0 = trim(inn[0]);
        std::string inn1 = trim(inn[1]);

        if(inn0=="-a"||inn0=="a"){
            if(inn1=="") throw std::invalid_argument("Lacking mandatory argument.");
            if(inn1[inn1.size()-1]!='/') inn1 += "/";
            outputdirectory = getPath(inn1,filelocation);
            return;
        }
        if(inn0=="-k"||inn0=="k"){
            if(inn1==""){
                timeout = 0;
                return;
            }
            timeout = stoi(inn1);
            return;
        }
        if(inn0=="-g"||inn0=="g"){
            if(inn1==""){
                g=0;
                return;
            }
            int newg = stoi(inn1);
            if(newg<0||newg>=MODES) throw std::invalid_argument("Mode '"+std::to_string(newg)+"' does not exist.");
            g = newg;
            return;
        }

        Flag val;
        if(inn1=="true"||inn1=="1"){
            val = TRUE;
        }
        else if(inn1=="false"||inn1=="0"){
            val = FALSE;
        }
        else if(inn1==""){
            val = DEFAULT;
        }
        else{
            throw std::invalid_argument("\""+inn1+"\" is not a valid boolean value.");  //error: bad assignment of bool
        }

        if(inn0=="v"||inn0=="-v"){
            if(!isSet(v)) v = val;
        }
        else if(inn0=="c"||inn0=="-c"){
            if(!isSet(c)) c = val;
        }
        else if(inn0=="o"||inn0=="-o"){
            if(!isSet(o)) o = val;
        }
        else if(inn0=="d"||inn0=="-d"){
            if(!isSet(d)) d = val;
        }
        else if(inn0=="m"||inn0=="-m"){
            if(!isSet(max)) max = val;
        }
        else if(inn0=="l"||inn0=="-l"||inn0=="log"||inn0=="-log"){
            if(!isSet(log)) log = val;
        }
        else if(inn0=="s"||inn0=="-s"){
            if(!isSet(s)) s = val;
        }
        else if(inn0=="b"||inn0=="-b"){
            if(!isSet(bighoa)) bighoa = val;
        }
        else{
            throw std::invalid_argument("\""+inn0+"\" is not an option.");   //error: bad option
        }
    }
    else{
        //error: more than one '='
        throw std::invalid_argument("Could not parse option update.");
    }
}

void Options::setline(const unsigned int line){
    if(!lines.empty()) lines[lines.size()-1] = line;
}

void Options::print() const{
    std::string res = ">Options: "
        "verbose: "+flag_string(v)+", "
        "mode: "+mode_to_string(g)+", "
        "use upper star height bound of STAMINA: "+flag_string(max)+", "
        "compare: "+flag_string(c)+", "
        "output: "+flag_string(o)+", "
        "debug: "+flag_string(d)+", "
        "timeout: "+(timeout>0 ? std::to_string(timeout)+"min" : "-")+", "
        "log data: "+flag_string(log)+", "
        "print statistics: "+flag_string(s)+"";
        //", use uncompressed hoa file: "+flag_string(bighoa)+"";
    res += "\n>Output directory: " + (outputdirectory.empty() ? "none" : "'"+outputdirectory+"'");
    res += "\n>Command directory: " + (filelocation.empty() ? "none" : "'"+filelocation+"'");
    std::cout << res << std::endl;
}

void Options::reset_data(){
    data = std::vector<std::vector<long>>(MODES,std::vector<long>(DATA_SIZE,0L));
    dset = std::vector<bool>(MODES,false);
    stamtime = 0L;
    stamcalls = 0L;
    stamlim = 0L;
}

void Options::add_data(Data d, long val){
    dset[g] = true;
    data[g][d] += val;
}

bool Options::has_data() const{
    if(stamcalls>0) return true;
    for(int m = 0; m < MODES; m++){
        if(dset[m]) return true;
    }
    return false;
}

void Options::add_result(std::string result){
    std::string res = make_prefix()+": "+result;
    if(results!=NULL) results->push_back(res);
}
void Options::add_test_results(){
    add_result(make_test_results());
}

void Options::add_data_results(){
    for(int m = 0; m < MODES; m++){
        if(!dset[m]) continue;
        auto lines = make_data_results(m);
        for(int n = 0; n<lines.size(); n++){
            add_result("["+std::to_string(m)+"]"+lines[n]);
        }
    }
}

std::string Options::make_prefix() const{
    std::string res = "";
    bool first = true;
    for(size_t i = 0; i < lines.size(); i++){
        if(lines[i]>0){
            if(!first) res += "_";
            res += std::to_string(lines[i]);
            first = false;
        }
    }
    return res;
}

std::string Options::make_path_prefix(const bool out) const{
    std::string res = "";
    if(outputdirectory.empty()) res = getPath("./",filelocation);
    else res = outputdirectory;
    res += (output() || out ? make_prefix() : "tmp");
    return res;
}

std::string Options::make_test_results() const{
    return std::to_string(passed)+"/"+std::to_string(tests);
}

std::vector<std::string> Options::make_data_results() const{
    std::vector<std::string> res;
    for(int m = 0; m < MODES; m++){
        if(dset[m]){
            auto resm = make_data_results(m);
            for(int l = 0; l < resm.size(); l++){
                res.push_back("["+mode_to_string(m)+"]"+resm[l]);
            }
        }
    }
    if(stamcalls>0){
        res.push_back("[STAMINA]LIM[x"+std::to_string(stamcalls)+"]: T="+took(stamtime/stamcalls) + ", LIMITED: x"+std::to_string(stamlim));
    }
    return res;
}

std::vector<std::string> Options::make_data_results(const int mode) const{
    std::vector<std::string> res;
    std::string cres;
    long ndet = data[mode][NDET_CALLS];
    if(ndet>0){
        cres = "NONDET[x"+std::to_string(ndet)+"]: N=" + std::to_string(data[mode][NDET_N]/ndet)+"/"+std::to_string(data[mode][NDET_NMAX]/ndet)+"="
            ""+std::to_string(100.0 * (double) data[mode][NDET_N]/(double) data[mode][NDET_NMAX])+"%, A="+std::to_string(data[mode][A]/ndet);
        res.push_back(cres);
    }
    long spot = data[mode][SPOT_CALLS];
    if(spot>0){
        cres = "DETERM[x"+std::to_string(spot)+"]: W="+took(data[mode][SPOT_W]/spot)+", D="+took(data[mode][SPOT_D]/spot)+", R="+took(data[mode][SPOT_R]/spot)+", "
        "N="+std::to_string(data[mode][DET_N]/spot)+", P="+std::to_string(data[mode][DET_P]/spot);
        res.push_back(cres);
    }
    long game = data[mode][GAME_CALLS];
    if(game>0){
        cres = "GAME[x"+std::to_string(game)+"]: T="+took(data[mode][GAME_T]/game)+", ";
        long vismax = data[mode][GAME_VISMAX];
        if(vismax>0){
            cres +="V="+std::to_string(data[mode][GAME_VIS]/game)+"/"+std::to_string(vismax/game)+"="
            ""+std::to_string(100.0 * (double)data[mode][GAME_VIS]/(double) vismax)+"%, ";
        }
        cres += "N="+std::to_string(data[mode][GAME_N]/game)+"/"+std::to_string(data[mode][GAME_NMAX]/game)+"="
            ""+std::to_string(100.0 * (double) data[mode][GAME_N]/(double) data[mode][GAME_NMAX])+"%, "
        "P="+std::to_string(data[mode][GAME_P]/game);
        res.push_back(cres);
    }
    long oink = data[mode][OINK_CALLS];
    if(oink>0){
        cres = "SOLVE[x"+std::to_string(oink)+"]: W="+took(data[mode][OINK_W]/oink)+", S="+took(data[mode][OINK_S]/oink)+", R="+took(data[mode][OINK_R]/oink);
        res.push_back(cres);
    }
    long lim = data[mode][LIM_CALLS];
    if(lim>0){
        cres = "LIM[x"+std::to_string(lim)+"]: T="+took(data[mode][LIM_T]/lim)+", LIMITED: x"+std::to_string(data[mode][LIM_LIM]);
        res.push_back(cres);
    }

    return res;
}

void Options::set(){
    v = setFlag(v);
    c = setFlag(c);
    o = setFlag(o);
    d = setFlag(d);
    max = setFlag(max);
    log = setFlag(log);
    s = setFlag(s);
    bighoa = setFlag(bighoa);
}

ResetOPT::ResetOPT(Options* opt){
    r = opt;
    o = *opt;
    opt->tests = 0;
    opt->passed = 0;
    opt->reset_data();
    opt->lines.push_back(0);
    opt->set();
}

ResetOPT::~ResetOPT(){
    if(!r->lines.empty()&&r->lines[r->lines.size()-1]>0){
        r->lines.pop_back();
        //r->add_data_results();
        if(r->tests>1){
            r->add_test_results();
            if(r->verbose()) std::cout << ">Passed "<<r->make_test_results()<<" tests." << std::endl;
        }
    }
    o.passed += r->passed;
    o.tests += r->tests;
    o.stamtime += r->stamtime;
    o.stamcalls += r->stamcalls;
    o.stamlim += r->stamlim;
    for(int m = 0; m < MODES; m++){
        if(r->dset[m]){
            o.dset[m] = true;
            for(int n = 0; n < DATA_SIZE; n++){
                o.data[m][n] += r->data[m][n];
            }
        }
    }
    *r = o;
}