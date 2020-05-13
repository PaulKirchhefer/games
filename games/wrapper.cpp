#include "wrapper.h"

std::vector<Player> parseSolutionFile(const std::string content){
    std::vector<Player> res;
    std::vector<bool> resset;
    StringParser sol(content);
    std::string line = trim(sol.advanceToChar(';',true));
    auto words = splitWords(line,true);
    if(sol.lastOp==VALID&&words.size()==2&&words[0]=="paritysol"){
        try{
            res.resize(stoi(words[1]));
            resset.resize(res.size());
        }
        catch(...){
            throw runtime_error("Could not parse number of states \""+words[1]+"\" in solution file header.");
        }
    }
    else{
        throw runtime_error("Could not parse solution file header \""+line+"\".");
    }
    while(true){
        line = trim(sol.advanceToChar(';',true));
        if(sol.lastOp==ENDOFFILE) break;
        words = splitWords(line,true);
        if(words.size()<2){
            throw runtime_error("Could not parse line \""+line+"\" of solution file body.");
        }
        state p;
        bool w;
        try{
            p = stoi(words[0]);
            if(words[1]=="0") w = false;
            else if(words[1]=="1") w = true;
            else throw runtime_error("");
        }
        catch(...){
            throw runtime_error("Could not parse state/winning player \""+words[0]+"/"+words[1]+"\".");
        }
        res[p] = (w ? P1 : P0);
        resset[p] = true;
    }
    for(state p = 0; p < res.size(); p++){
        if(!resset[p]) throw runtime_error("Could not parse solution from file, not all states were assigned a winning player.");
    }
    return res;
}

std::vector<Player> solveParityGame(const ParityGame& parityGame, Options* opt){
    if(!opt->canWrite()){
        if(opt->output()) throw std::runtime_error("No directory found to write data to.");
        throw std::runtime_error("No directory found to write temporary data to.");
    }
    //write parityGame
    std::string prefix = opt->make_path_prefix();
    std::string game_file = prefix + "_game";
    std::string sol_file = prefix + "_sol";
    auto start = std::chrono::steady_clock::now();
    writeToFile(game_file,parityGame.to_pgsolver());
    auto end = std::chrono::steady_clock::now();
    long write = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    long solve = 0L;
    long read = 0L;
    //solve
    int i = -1;
    if(system(NULL)){
        std::string timeout1 = "";
        std::string timeout2 = "";
        if(opt->timeout>0){
            unsigned int tosec = opt->timeout*60;
            unsigned int killsec = tosec + (tosec/10);
            timeout1 = "timeout --kill-after=6 " + to_string(killsec) + " ";
            timeout2 = "-z " + to_string(tosec) + " ";
        }
        std::string cmd = timeout1 + "oink "+game_file+" -w 0 "+timeout2+sol_file;
        if((!opt->verbose()&&!opt->debug())||true){
#ifdef __linux__
            cmd += " > /dev/null";
#endif
        }
        if(opt->verbose()){
            std::cout << ">Running oink.." << std::endl;
        }
        start = std::chrono::steady_clock::now();
        i = system(cmd.c_str());
        end = std::chrono::steady_clock::now();
        solve = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
        if(opt->verbose()){
            std::cout << ">Solving the parity game took: "+took(solve) << std::endl;
        }
    }
    if(i!=0){
        //handle error
        std::string exit = "";
#ifdef __unix__
        if(WIFEXITED(i)) exit = "[exit "+to_string(WEXITSTATUS(i))+"]";
        else if(WIFSIGNALED(i)) exit = "[term "+to_string(WTERMSIG(i))+"]";
#endif
        throw runtime_error("Could not successfully run oink to solve parity game."+exit);
    }
    //read/parse solution
    start = std::chrono::steady_clock::now();
    string sol = readFileFromPath(sol_file);
    std::vector<Player> res = parseSolutionFile(sol);
    end = std::chrono::steady_clock::now();
    read = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

    if(opt->logdata()){
        opt->writeToLog(opt->make_prefix()+" sol: w="+took(write)+", s="+took(solve)+", r="+took(read)+"\n");
    }
    if(opt->stats()){
        opt->add_data(OINK_W, write);
        opt->add_data(OINK_S, solve);
        opt->add_data(OINK_R, read);
        opt->add_data(OINK_CALLS, 1L);
    }

    return res;
}

DetParityAut determinizeParityAut(ParityAut* parityAut, Options* opt, bool deleteAut, const Alphabet* alph){
    if(!opt->canWrite()){
        if(opt->output()) throw std::runtime_error("No directory found to write data to.");
        throw std::runtime_error("No directory found to write temporary data to.");
    }
    //write parityAut
    std::string prefix = opt->make_path_prefix();
    std::string nondet = prefix + "_nondet";
    std::string det = prefix + "_det";
    
    letter A = parityAut->A;

    auto start = std::chrono::steady_clock::now();
    std::string hoa = parityAut->to_hoa(true);
    if(deleteAut){
        delete parityAut; parityAut = NULL;
    }
    writeToFile(nondet,hoa);
    auto end = std::chrono::steady_clock::now();
    long write = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    long deter = 0L;
    long read = 0L;
    //determinize
    int i = -1;
    if(system(NULL)){
        std::string timeout = "";
        if(opt->timeout>0){
            unsigned int tosec = opt->timeout*60;
            unsigned int killsec = tosec/10;
            timeout = "timeout --kill-after="+to_string(killsec)+" "+to_string(tosec)+" ";
        }
        std::string cmd = timeout+"autfilt --file="+nondet+" -S --colored-parity=\"max even\" -D -C --hoaf=s";
        cmd += " -o "+det;
        if(opt->verbose()){
            std::cout << ">Running SPOT.." << std::endl;
        }
        start = std::chrono::steady_clock::now();
        i = system(cmd.c_str());
        end = std::chrono::steady_clock::now();
        deter = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
        if(opt->verbose()){
            std::cout << ">Determinizing the automaton took: "+took(deter) << std::endl;
        }
    }
    if(i!=0){
        //handle error
        std::string exit = "";
#ifdef __unix__
        if(WIFEXITED(i)) exit = "[exit "+to_string(WEXITSTATUS(i))+"]";
        else if(WIFSIGNALED(i)) exit = "[term "+to_string(WTERMSIG(i))+"]";
#endif
        throw runtime_error("Could not successfully run autfilt to determinize automaton."+exit);
    }

    start = std::chrono::steady_clock::now();
    //read DetParityAut
    string res = readFileFromPath(det);
    //string to DetParityAut
    DetParityAut dpa = hoaToDetParityAut(res,A,alph);
    end = std::chrono::steady_clock::now();
    read = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

    parity P = dpa.maxParity();
    //statistics
    if(opt->logdata()){
        opt->writeToLog(opt->make_prefix()+" det: w="+took(write)+", d="+took(deter)+", r="+took(read)+"\n");
        std::string maxpar = std::to_string(P);
        opt->writeToLog("\t\tresult: N="+std::to_string(dpa.N)+", A="+std::to_string(dpa.A)+", P="+maxpar+"\n");
    }
    if(opt->stats()){
        opt->add_data(SPOT_W, write);
        opt->add_data(SPOT_D, deter);
        opt->add_data(SPOT_R, read);
        opt->add_data(DET_N, dpa.N);
        opt->add_data(DET_P, P);
        opt->add_data(SPOT_CALLS, 1L);
    }

    return dpa;
}

bool isLimitedSTAMINA(const MultiCounterAut& caut, Options* opt, std::string* res){
    auto start = std::chrono::steady_clock::now();
    //****STAMINACODE****//
    UnstableMultiMonoid umm(caut);
    const ExtendedExpression * witness = umm.containsUnlimitedWitness();
    //auto witness = umm.ComputeMonoid();
    //*******************//

    auto end = std::chrono::steady_clock::now();
    long us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

    //statistics
    std::string time = took(us);
    if(opt->logdata()){
        std::stringstream strm;
        if(witness!=NULL) witness->print(strm);
        opt->writeToLog("\n"+opt->make_prefix()+" STAMINA: took: "+time+", res: "+(witness==NULL ? "T":"F ("+strm.str()+")")+"\n");
    }
    if(opt->stats()){
        opt->stamcalls++;
        opt->stamtime += us;
    }

    if(witness==NULL){
        if(opt->stats()) opt->stamlim++;
        return true;
    }
    if(res!=NULL){
        std::stringstream strm;
        witness->print(strm);
        *res = strm.str();
    }
    if(opt->verbose()){
        std::cout << ">STAMINA found an unlimitedness witness:" << std::endl;
    }
    if(opt->verbose()||opt->debug()){
        witness->print();
        std::cout << std::endl;
    }
    return false;
}

int starheightSTAMINA(const ClassicAut& aut, Options* opt){
    UnstableMultiMonoid* umm = NULL;
    const ExtendedExpression* ee = NULL;
    if(opt->output()&&!opt->canWrite()){
        throw std::runtime_error("No directory found to write data to.");
    }
    try{
        ClassicAut nonconst(aut);
        int lc = 0;
        std::string prefix = opt->make_path_prefix() + "_STAMINA_";
        if(opt->verbose()) std::cout << ">Running STAMINA.." << std::endl;
        int sh = computeStarHeight(nonconst,umm,ee,lc,opt->output(),opt->verbose(),prefix,true,true,true);
        delete umm; umm = NULL; delete ee; ee = NULL;
        return sh;
    }
    catch(const std::exception& e){
        delete umm; umm = NULL; delete ee; ee = NULL;
        throw e;
    }
}

MultiCounterAut* reductionSTAMINA(const ClassicAut& aut, unsigned int k, Options* opt){
    ClassicAut nonconst(aut);
    if(!nonconst.iscomplete()){
        nonconst.addsink();
    }
    if(opt->verbose()){
        std::cout<<">To subset automaton.. ";
    }
    ClassicEpsAut* ssa = toSubsetAut(&nonconst);
    if(opt->verbose()){
        std::cout<<ssa->NbStates<< " states" << std::endl;
        std::cout<<">Minimizing subset automaton.. ";
    }
    ClassicEpsAut* ssamin = SubMinPre(ssa);
    if(opt->verbose()){
        std::cout<<ssamin->NbStates<< " states"<<std::endl;
        std::cout<<">Pruning subset automaton.. ";
    }
    ClassicEpsAut* ssaprune = SubPrune(ssamin);
    if(opt->verbose()){
        std::cout<<ssaprune->NbStates<< " states"<<std::endl;
        std::cout<<">Reducing to limitedness.. ";
    }
    if(opt->output()&&!opt->canWrite()){
        if(opt->verbose()) std::cout << std::endl;
        throw std::runtime_error("No directory found to write data to.");
    }
    std::string prefix = opt->make_path_prefix() + "_STAMINA_";
    MultiCounterAut* red = toNestedBaut(ssaprune,k,opt->debug(),opt->output(),prefix);
    if(opt->verbose()){
        std::cout << red->NbStates << " states, " << to_string(red->NbCounters) << " counters"<<std::endl;
    }
    delete ssa; ssa = NULL;
    delete ssamin; ssamin = NULL;
    delete ssaprune; ssaprune = NULL;
    return red;
}

unsigned int maxStarHeight(const ClassicAut& aut){
    ClassicAut ncaut = aut;
    //**********STAMINACODE*****************//
    auto res = LoopComplexity(&ncaut);
    unsigned int LC;
    LC = (int)res.first ;
    list<uint> order = res.second;
    RegExp * regexpr = Aut2RegExp( &ncaut , order );
    
    list<ExtendedExpression *> sharplist = Reg2Sharps(regexpr);
    
    if(regexpr == NULL || sharplist.size()==0)// empty language
    {
        LC = 0;
    }
    else if(regexpr->starheight<LC) LC=regexpr->starheight;
    //for(ExtendedExpression* ee : sharplist){
    //    delete ee; ee = NULL;
    //}
    //delete regexpr; regexpr = NULL;       //memory leak(in STAMINA?)?

    return LC;
}

ClassicEpsAut classicToClassicEps(const ClassicAut& aut_, const bool makeEps){
    ClassicAut aut(aut_);   //map[a] requires non const.. aut_ expected to be of trivial size anyway
    char A = makeEps ? aut.NbLetters-1 : aut.NbLetters; //if makeEps: interpret last letter as epsilon
    uint N = aut.NbStates;
    ClassicEpsAut res(A,N);
    for(uint p = 0; p < N; p++){
        //copy initial/final states
        res.initialstate[p] = aut.initialstate[p];
        res.finalstate[p] = aut.finalstate[p];
        for(uint q = 0; q < N; q++){
            for(char a = 0; a < A; a++){
                res.trans[a][p][q] = aut.trans[a][p][q];
            }
            if(makeEps){
                res.trans_eps[p][q] = aut.trans[A][p][q];
            }
            if(p==q) res.trans_eps[p][q] = true;    //include eps self loops explicitly
        }
    }
    return res;
}

MultiCounterAut* explicitToMultiCounter(const ExplicitAutomaton& eaut){
    MultiCounterAut* caut = new MultiCounterAut(eaut.matrices.size(),eaut.size,eaut.type);
    caut->initialstate[eaut.initialState] = true;
    for(int f : eaut.finalStates){
        caut->finalstate[f] = true;
    }
    for(size_t a = 0; a < eaut.matrices.size(); a++){
        //eaut.matrices[a].print();
        MultiCounterMatrix* mcm = new MultiCounterMatrix(eaut.matrices[a]);
        //mcm->print();
        caut->set_trans(a,mcm);
    }
    return caut;
}

void printNFA(const ClassicEpsAut& eaut_){
    ClassicEpsAut eaut(eaut_);  //map[a] requires non const.. eaut_ expected to be of trivial size anyway
    uint N = eaut.NbStates;
    char A = eaut.NbLetters;
    string res = "NFA with "+to_string(N)+" states and "+to_string(A)+" letters:";
    res += "\nInitial states:";
    bool found = false;
    for(uint p = 0; p < N; p++){
        if(eaut.initialstate[p]){
            found = true;
            res += " "+to_string(p);
        }
    }
    if(!found) res+= " none";
    res += "\nFinal states:";
    found = false;
    for(uint p = 0; p < N; p++){
        if(eaut.finalstate[p]){
            found = true;
            res += " "+to_string(p);
        }
    }
    if(!found) res+= " none";
    for(char a = 0; a < A; a++){
        res += "\n\nLetter "+to_string(a)+":";
        for(uint p = 0; p < N; p++){
            res +="\n";
            for(uint q = 0; q < N; q++){
                if(q>0) res+= " ";
                if(eaut.trans[a][p][q]){
                    res += "1";
                }
                else{
                    res += "_";
                }
            }
        }
    }
    res += "\n\nEpsilon:";
    for(uint p = 0; p < N; p++){
        res += "\n";
        for(uint q = 0; q < N; q++){
            if(q>0) res += " ";
            if(eaut.trans_eps[p][q]){
                res += "1";
            }
            else{
                res += "_";
            }
        }
    }
    cout << res << endl;
}

std::string to_stamina(MultiCounterAut& caut){
    std::string res = "";
    res += to_string(caut.NbStates)+"\n";
    res += to_string(caut.NbCounters)+"\n";
    for(char a = 0; a < caut.NbLetters; a++){
        char a_ = 'a';
        a_ += a;
        res+=a_;
    }
    res += "\n";
    bool first = true;
    for(state p = 0; p < caut.NbStates; p++){
        if(caut.initialstate[p]){
            if(!first) res += " ";
            res += to_string(p);
            first = false;
        }
    }
    res += "\n";
    first = true;
    for(state p = 0; p < caut.NbStates; p++){
        if(caut.finalstate[p]){
            if(!first) res += " ";
            res += to_string(p);
            first = false;
        }
    }
    if(first){
        res += to_string(caut.NbStates);
    }
    for(char a = 0; a < caut.NbLetters; a++){
        const MultiCounterMatrix* mat = caut.get_trans(a);
        char a_ = 'a';
        a_ += a;
        res += "\n\n";
        res += a_;
        for(state p = 0; p < caut.NbStates; p++){
            res += "\n";
            for(state q = 0; q < caut.NbStates; q++){
                if(q>0) res += " ";
                char cact = mat->get(p,q);
                if(MultiCounterMatrix::is_epsilon(cact)) res+="E";
                else if(MultiCounterMatrix::is_omega(cact)) res+="O";
                else if(MultiCounterMatrix::is_reset(cact)) res += "R"+to_string(MultiCounterMatrix::get_reset_counter(cact));
                else if(MultiCounterMatrix::is_inc(cact)) res += "I"+to_string(MultiCounterMatrix::get_inc_counter(cact));
                else res += "_";
            }
        }
    }
    return res;
}