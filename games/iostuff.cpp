#include "iostuff.h"

int getMacroID(const std::string ID){
    string ID_ = trim(ID);
    if(startsWith(ID_,"#")){
        ID_ = ID_.substr(1,ID_.size()-1);   //remove '#'
        try{
            return stoi(ID_);               //parse int
        }
        catch(const invalid_argument& e){
            throw invalid_argument("Could not parse ID of macro.");
        }
        catch(const out_of_range& e){
            throw invalid_argument("Macro ID is too large.");
        }
    }
    throw invalid_argument("\""+trim(ID)+"\" is not a valid macro identifier.");
}

void updateMacroMap(const std::string line, StringParser& text, macromap& mm){
    string line_ = trim(line);
    if(startsWith(line_,"#")){
        vector<string> words = split(line_,' ',true);
        string ID = words[0];
        if(words.size()==1){
            //macro ID and ':' are not separated by whitespace
            if(ID[ID.size()-1]==':'){
                ID = ID.substr(0,ID.size()-1);
            }
            else{
                //no ':' found, not a valid definition
                throw invalid_argument("Could not parse macro definition.");
            }
        }
        else if(words.size()!=2||words[1]!=":"){
            //input is not macroID + whitespace + ':'
            throw invalid_argument("Could not parse macro definition.");
        }
        int index = getMacroID(ID);
        string content = trim(text.advanceToChar('#',true));
        if(text.lastOp!=VALID) throw invalid_argument("Could not retrieve the content of the macro.");  //no closing '#' found
        mm[index]=content;
    }
    else throw invalid_argument("\""+line_+"\" is not a macro definition.");    //input does not start with '#'
}

void printHelp(){
    string help = ">Commands:"
    "\n\t>h: prints these instructions"
    "\n\t>q: quits the program"
    "\n\t>do <path>: executes the instructions in the file at <path>"
    "\n\t>#<int> <path>: reads the file at <path> and stores the content in the macro #<int>"
    //"\n\t>sh (<regex>): calculate the star height of the language given by the regular expression <regex>"
    "\n\t>sh <path>|#<int> [expected]: calculate the star height of the language given by the DFA in the file at <path> or in the macro #<int>(assumes sh>0)"
    //"\n\t>fp (<regex>): determines if the language given by the regular expression <regex> has the finite power property"
    "\n\t>fp <path>|#<int> [expected]: determines if the language given by the NFA in the file at <path> or in the macro #<int> has the finite power property"
    "\n\t>lim <path>|#<int> [expected]: determines if the cost automaton given by the path <path> or macro #<int> is limited"
    "\n\t\t>If an expected value is given the program compares the computed result to it."
    "\n>Paths may be relative to the file currently worked on or the current working directory using '..' and '.'. Specify <path>s using the file separator '/'! The working directory can be changed using 'cd <path>'."
    "\n>See options using '-', set options using '-<opt>=<value>' or '-<opt>=' to reset them to default. Available options:"
    "\n\t>-v: verbose output"
    "\n\t>-d: debug, prints additional information"
    "\n\t>-c: compare results to results computed by STAMINA"
    "\n\t>-m: use loop complexity heuristic due to STAMINA as an upper bound for the star height computation"
    "\n\t>-s: print statistics after command terminates"
    "\n\t>-l: append some statistics to a log file"
    "\n\t>-o: write interim automata to filesystem"
    "\n>All options may also be changed in a file read by 'do'. The above options are false by default. If any of the above options is explicitly set and not default, a file read using 'do' can not change them."
    "\n\t>-a=<path>: The program needs to write auxiliary files and you may require additional file output. Use this to set the directory. Directory must exist!"
#ifdef __linux__
    "\n\t>-k=<minutes>: kill SPOT/OINK after roughly <minutes> using the timeout command"
#endif
    "\n\t>-g=<mode> set game construction to solve limitedness, 0 <= <mode> <= "+std::to_string(MODES)+""
    "\n>The available modes are: transition sequences(SEQ X Y), state/letter/state sequences (SEQ ALT) or transition sets (TSETS)."
    "\n\t>X/Y are the values of the sinks for transition order/W1, where '-' indicates that the condition is not checked for.";
    for(int m = 0; m < MODES; m++){
        help += "\n\t\t>"+std::to_string(m) + ": [" + mode_to_string(m)+"]";
    }
    cout << help << endl;
}

ExplicitAutomaton* getAut(const std::string arg, const std::string filelocation, const macromap& mm){
    string arg_ = trim(arg);
    if(startsWith(arg_,"#")){
        return macroToAut(arg_,mm);
    }
    else{
        return fileToAut(getPath(arg_,filelocation));
    }
}

ExplicitAutomaton* macroToAut(const std::string macro, const macromap& mm){
    int index = getMacroID(macro);
    if(mm.count(index)>0){
        //macro found
        stringstream autin(mm.at(index));
        try{
            return Parser::parseFile(autin);
        }
        catch(const runtime_error& e){
            throw invalid_argument("#"+to_string(index)+" is not a valid automaton.");
        }
    }
    else{
        throw invalid_argument("Undefined macro with ID "+to_string(index)+".");
    }
}

ExplicitAutomaton* fileToAut(const std::string path){
    string content;
    try{
        content = trim(readFileFromPath(path));
    }
    catch(const runtime_error& e){
        throw runtime_error("Could not open file at path \""+path+"\".");
    }
    stringstream autin(content);
    try{
        return Parser::parseFile(autin);
    }
    catch(const runtime_error& e){
        throw invalid_argument("Could not parse automaton given by file at path \""+path+"\".");
    }
}

std::vector<state> getStateList(const std::string arg, const macromap& mm){
    std::string in = trim(arg);
    if(startsWith(in,"#")){
        int index = getMacroID(in);
        if(mm.count(index)>0){
            try{
                return getStateList("{"+mm.at(index)+"}",mm);
            }
            catch(const invalid_argument& e){
                std::string msg = e.what();
                throw invalid_argument("Given macro is not a valid state list: "+msg);
            }
        }
        else{
            throw invalid_argument("Undefined macro with ID "+to_string(index)+".");
        }
    }
    else{
        if(in.size()>=2&&in[0]=='{'&&in[in.size()-1]=='}'){
            in = extract(in);
            std::vector<std::string> ins = splitWords(in);
            std::vector<state> list;
            for(size_t i = 0; i < ins.size(); i++){
                try{
                    list.push_back(stoi(ins[i]));
                }
                catch(const logic_error& e){
                    throw invalid_argument("Error parsing state list, could not parse state '"+ins[i]+"'.");
                }
            }
            return list;
        }
        else{
            throw invalid_argument("Could not parse, input is not a valid list.");
        }
    }
}

void printComparison(std::string games, std::string expected, std::string stamina){
    bool passed = games==expected;
    if(!stamina.empty()) passed = passed && (games==stamina);
    if(passed){
        std::cout << ">Computed the expected result \""<<expected<<"\"." << std::endl;
    }
    else if(!stamina.empty()){
        if(games==expected){
            std::cout << ">Computed the expected result \""<<expected<<"\", but STAMINA computed \""<<stamina<<"\"." << std::endl;
        }
        else if(stamina==expected){
            std::cout << ">Computed unexpected result \""<<games<<"\", but STAMINA computed the expected result \""<<expected<<"\"."<<std::endl;
        }
        else{
            std::cout << ">Computed \""<<games<<"\" and STAMINA computed \""<<stamina<<"\", but expected result was \""<<expected<<"\"."<<std::endl;
        }
    }
    else{
        std::cout << ">Computed result \""<<games<<"\" is unequal to the expected result \""<<expected<<"\"."<<std::endl;
    }
}

void handleCommand(std::string cmd, Options* opt){
    ResetOPT r(opt);    //restores options after return
    vector<string> args = splitWords(cmd);
    bool noarg = false;     //flag for "not enough arguments" error
    if(args.empty()){
        //cmd consists of whitespaces only
        throw invalid_argument("Could not parse empty input.");
    }
    args[0] = aZToLowerCase(args[0]);

    if(args[0]=="do"){
        if(args.size()>=2){     //additional arguments are ignored
            handleFilePath(args[1],opt);
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="sh"||args[0]=="starheight"){
        if(args.size()>=2){
            unsigned int sh;        //to be computed
            unsigned int stam;      //to be computed using stamina
            unsigned int exp;       //expected value
            if(args.size()>=3){
                //parse expected value if given
                try{
                    exp = stoi(args[2]);
                }
                catch(const invalid_argument& e){
                    throw invalid_argument("Could not parse expected starheight.");
                }
            }
            if(args[1].size()>=2&&args[1][0]=='('&&args[1][args[1].length()-1]==')'){
                //compute starheight from regex
                std::string regex = extract(args[1]);
                auto start = std::chrono::steady_clock::now();
                try{
                    if(opt->verbose()) std::cout << std::endl;
                    sh = starheight(regex,opt);
                    if(opt->verbose()) std::cout << ">Computing the star height took: "+took(start)<<std::endl;
                }
                catch(const exception& e){
                    string msg = e.what();
                    throw runtime_error("Error computing the starheight: "+msg);
                }
                try{
                    if(opt->compare()){
                        if(opt->verbose()) std::cout << std::endl;
                        start = std::chrono::steady_clock::now();
                        stam = starheight(regex,opt,true);
                        if(opt->verbose()) std::cout << ">Computing the star height using STAMINA took: "+took(start)<<std::endl;
                    }
                }
                catch(const exception& e){
                    string msg = e.what();
                    throw runtime_error("Error computing the starheight using STAMINA: "+msg);
                }
            }
            else{
                //compute starheight from automaton
                ExplicitAutomaton* eaut = getAut(args[1], opt->filelocation,opt->mm);
                if(eaut->type!=CLASSICAL){
                    delete eaut; eaut = NULL;
                    throw invalid_argument("Given automaton is not a deterministic finite automaton.");
                }
                ClassicAut aut(*eaut);
                delete eaut; eaut = NULL;
                auto start = std::chrono::steady_clock::now();
                try{
                    if(opt->verbose()) std::cout << std::endl;
                    sh = starheight(aut,opt);
                    if(opt->verbose()) std::cout << ">Computing the star height took: "+took(start)<<std::endl;
                }
                catch(const exception& e){
                    string msg = e.what();
                    throw runtime_error("Error computing the starheight: "+msg);
                }
                try{
                    if(opt->compare()){
                        if(opt->verbose()) std::cout << std::endl;
                        start = std::chrono::steady_clock::now();
                        stam = starheightSTAMINA(aut,opt);
                        if(opt->verbose()) std::cout << ">Computing the star height using STAMINA took: "+took(start)<<std::endl;
                    }
                }
                catch(const exception& e){
                    string msg = e.what();
                    throw runtime_error("Error computing the starheight using STAMINA: "+msg);
                }
            }
            if(args.size()>=3){    
                bool passed = sh==exp;         //compare computed and expected result
                if(opt->compare()) passed = passed && (sh==stam);
                if(opt->verbose()){
                    printComparison(to_string(sh),to_string(exp),(opt->compare() ? to_string(stam) : ""));
                }
                else{
                    std::cout << ">" << passed << std::endl;
                }
                opt->tests++;
                if(passed){
                    opt->passed++;
                }
                else{
                    opt->add_result("Failed");
                }
            }
            else{
                //output result
                string res = to_string(sh);
                if(opt->compare()) res += "?"+to_string(stam);
                if(opt->verbose()){
                    if(opt->compare()){
                        if(stam==sh){
                            std::cout<<">Computed the same star height \""<<sh<<"\" as STAMINA."<<std::endl;
                        }
                        else{
                            std::cout<<">Computed star height \""<<sh<<"\" but STAMINA computed \""<<stam<<"\"."<<std::endl;
                        }
                    }
                    else{
                        std::cout<<">The star height is \""<<sh<<"\"."<<std::endl;
                    }
                }
                else{
                    std::cout << ">" << res << std::endl;
                }
                opt->add_result(res);
            }
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="fp"||args[0]=="finitepower"){
        if(args.size()>=2){
            bool fp;            //to be computed
            bool stam;          //to be computed using stamina
            bool exp;           //expected value
            if(args.size()>=3){         //parse expected value if given
                if(args[2]=="true"||args[2]=="1"){
                    exp = true;
                }
                else if(args[2]=="false"||args[2]=="0"){
                    exp = false;
                }
                else{
                    throw invalid_argument("Could not parse expected finite power property.");
                }
            }
            if(args[1].size()>=2&&args[1][0]=='('&&args[1][args[1].length()-1]==')'){
                //compute finite power from regex
                std::string regex = extract(args[1]);
                auto start = std::chrono::steady_clock::now();
                try{
                    if(opt->verbose()) std::cout << std::endl;
                    fp = finitePower(regex,opt);
                    if(opt->verbose()) std::cout << ">Calculating the finite power property took: "+took(start)<<std::endl;
                }
                catch(const exception& e){
                    string msg = e.what();
                    throw runtime_error("Error computing the finite power property: "+msg);
                }
                try{
                    if(opt->compare()){
                        if(opt->verbose()) std::cout << std::endl;
                        start = std::chrono::steady_clock::now();
                        stam = finitePower(regex,opt,true);
                        if(opt->verbose()) std::cout << ">Calculating the finite power property using STAMINA took: "+took(start)<<std::endl;
                    }
                }
                catch(const exception& e){
                    string msg = e.what();
                    throw runtime_error("Error computing the finite power property using STAMINA: "+msg);
                }
            }
            else{
                //compute finite power from automaton
                ExplicitAutomaton* eaut = getAut(args[1],opt->filelocation,opt->mm);
                ClassicAut caut(*eaut);
                auto start = std::chrono::steady_clock::now();
                try{
                    if(opt->verbose()) std::cout << std::endl;
                    fp = finitePower(classicToClassicEps(caut,eaut->type==0),opt);
                    if(opt->verbose()) std::cout << ">Calculating the finite power property took: "+took(start)<<std::endl;
                }
                catch(const exception& e){
                    string msg = e.what();
                    delete eaut; eaut = NULL;
                    throw runtime_error("Error computing the finite power property: "+msg);
                }
                try{
                    if(opt->compare()){
                        if(opt->verbose()) std::cout << std::endl;
                        start = std::chrono::steady_clock::now();
                        stam = finitePower(classicToClassicEps(caut,eaut->type==0),opt,true);
                        if(opt->verbose()) std::cout << ">Calculating the finite power property using STAMINA took: "+took(start)<<std::endl;
                    }
                }
                catch(const exception& e){
                    string msg = e.what();
                    delete eaut; eaut = NULL;
                    throw runtime_error("Error computing the finite power property using STAMINA: "+msg);
                }
                delete eaut; eaut = NULL;
            }
            if(args.size()>=3){
                //compare to expected result
                bool passed = (fp==exp);
                if(opt->compare()) passed = passed && (fp==stam);
                if(opt->verbose()){
                    printComparison(to_string(fp),to_string(exp),(opt->compare() ? to_string(stam) : ""));
                }
                else{
                    std::cout << ">" << passed << std::endl;
                }
                opt->tests++;
                if(passed){
                    opt->passed++;
                }
                else{
                    opt->add_result("Failed");
                }
            }
            else{
                //output result
                string res = to_string(fp);
                if(opt->compare()) res += "?" + to_string(stam);
                if(opt->verbose()){
                    if(!opt->compare()||(fp==stam)){
                        if(fp){
                            std::cout << ">The language has the finite power property." << std::endl;
                        }
                        else{
                            std::cout << ">The language does not have the finite power property." << std::endl;
                        }
                    }
                    else{
                        std::cout <<">Calculated that the language "<<(fp? "has":"does not have")<<" the finite power property, but STAMINA says otherwise."<<std::endl;
                    }
                }
                else{
                    std::cout << ">" << res << std::endl;
                }
                opt->add_result(res);
            }
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="lim"){
        if(args.size()>=2){
            bool lim;               //to be computed
            bool stam;              //to be computed using stamina
            bool exp;               //expected value
            if(args.size()>=3){         //parse expected value if given
                if(args[2]=="true"||args[2]=="1"){
                    exp = true;
                }
                else if(args[2]=="false"||args[2]=="0"){
                    exp = false;
                }
                else{
                    throw invalid_argument("Could not parse expected limitedness property.");
                }
            }
            ExplicitAutomaton* eaut = getAut(args[1],opt->filelocation,opt->mm);
            if(eaut->type<=0){
                delete eaut; eaut = NULL;
                throw invalid_argument("Given automaton is not a cost automaton.");
            }
            MultiCounterAut* caut = explicitToMultiCounter(*eaut);

            delete eaut; eaut = NULL;
            auto start = std::chrono::steady_clock::now();
            try{
                lim = isLimited(*caut,opt);
                if(opt->verbose()) std::cout << ">Deciding limitedness took: "+took(start)<<std::endl;
            }
            catch(const exception& e){
                std::string msg = e.what();
                delete caut; caut = NULL;
                throw runtime_error("Error computing the limitedness property: "+msg);
            }
            try{
                if(opt->compare()){
                    if(opt->verbose()) std::cout << std::endl << ">Running STAMINA.." << std::endl;
                    start = std::chrono::steady_clock::now();
                    stam = isLimitedSTAMINA(*caut,opt);
                    if(opt->verbose()) std::cout << ">Deciding limitedness using STAMINA took: "+took(start)<<std::endl;
                }
            }
            catch(const exception& e){
                delete caut; caut = NULL;
                std::string msg = e.what();
                throw runtime_error("Error computing the limitedness property using STAMINA: "+msg);
            }
            delete caut; caut = NULL;
            if(args.size()>=3){
                //compare to expected result
                bool passed = (lim==exp);
                if(opt->compare()) passed = passed && (lim==stam);
                if(opt->verbose()){
                    printComparison(to_string(lim),to_string(exp),(opt->compare()?to_string(stam):""));
                }
                else{
                    std::cout << ">" << passed << std::endl;
                }
                opt->tests++;
                if(passed){
                    opt->passed++;
                }
                else{
                    opt->add_result("Failed");
                }
            }
            else{
                //output result
                string res = to_string(lim);
                if(opt->compare()) res += "?" + to_string(stam);
                if(opt->verbose()){
                    if(!opt->compare()||(lim==stam)){
                        std::cout << ">The cost automaton is "<<(lim ? "": "not ")<<"limited." << std::endl;
                    }
                    else{
                        std::cout << ">Calculated that the cost automaton is "<<(lim ? "": "not ")<<"limited, but STAMINA says otherwise." << std::endl;
                    }
                }
                else{
                    std::cout << ">" << res << std::endl;
                }
                opt->add_result(res);
            }
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="limstam"||args[0]=="limstamina"){
        if(args.size()>=2){
            bool stam;              //to be computed using stamina
            bool exp;               //expected value
            if(args.size()>=3){         //parse expected value if given
                if(args[2]=="true"||args[2]=="1"){
                    exp = true;
                }
                else if(args[2]=="false"||args[2]=="0"){
                    exp = false;
                }
                else{
                    throw invalid_argument("Could not parse expected limitedness property.");
                }
            }
            ExplicitAutomaton* eaut = getAut(args[1],opt->filelocation,opt->mm);
            if(eaut->type<=0){
                delete eaut; eaut = NULL;
                throw invalid_argument("Given automaton is not a cost automaton.");
            }
            MultiCounterAut* caut = explicitToMultiCounter(*eaut);

            delete eaut; eaut = NULL;
            auto start = std::chrono::steady_clock::now();
            try{
                if(opt->verbose()) std::cout << std::endl << ">Running STAMINA.." << std::endl;
                start = std::chrono::steady_clock::now();
                stam = isLimitedSTAMINA(*caut,opt);
                if(opt->verbose()) std::cout << ">Deciding limitedness using STAMINA took: "+took(start)<<std::endl;
            }
            catch(const exception& e){
                std::string msg = e.what();
                delete caut; caut = NULL;
                throw runtime_error("Error computing the limitedness property using STAMINA: "+msg);
            }
            delete caut; caut = NULL;

            if(args.size()>=3){
                opt->tests++;
                if(stam==exp){
                    opt->passed++;
                    if(opt->verbose()) std::cout << ">STAMINA computed the expected result "+std::to_string(stam)+"."<<std::endl;
                }
                else{
                    opt->add_result("Failed");
                    if(opt->verbose()) std::cout << ">STAMINA computed "+std::to_string(stam)+", but "+std::to_string(exp)+ " was the expected result."<<std::endl;
                }
            }
            else{
                opt->add_result(std::to_string(stam));
                if(opt->verbose()) std::cout << ">According to STAMINA, the automaton is "<<(stam ? "" : "not ")<<"limited."<<std::endl;
            }
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="redstam"||args[0]=="redstamina"){
        if(args.size()>=2){
            ExplicitAutomaton* eaut = getAut(args[1], opt->filelocation,opt->mm);
            if(eaut->type!=CLASSICAL){
                delete eaut; eaut = NULL;
                throw invalid_argument("Given automaton is not a deterministic finite automaton.");
            }
            ClassicAut aut(*eaut);
            delete eaut; eaut = NULL;

            int max;
            int min = 1;
            if(args.size()>=3){
                max = stoi(args[2]);
                min = max;
                if(min<1) throw invalid_argument("Star height to check must be greater than 0.");
            }
            else{
                max = maxStarHeight(aut);
            }

            if(!opt->canWrite()) throw std::runtime_error("No directory found to write data to.");

            MultiCounterAut* redk = NULL;
            for(int k = min; k <= max; k++){
                if(opt->verbose()) std::cout << ">Reduction for star height "<<k<<".." << std::endl;
                opt->setline(k);
                redk = reductionSTAMINA(aut,k,opt);
                std::string path = opt->make_path_prefix(true)+(k==0?"_0":"")+"_red";

                try{
                    writeToFile(path,to_stamina(*redk));
                    delete redk; redk=NULL;
                }
                catch(const std::exception& e){
                    delete redk; redk=NULL;
                    std::string msg = e.what();
                    throw runtime_error("Failed to write cost automaton of star height reduction to file: "+msg);
                }
            }
            if(opt->verbose()) std::cout << "Done!" << std::endl;
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="redfp"){
        if(args.size()>=2){
            ExplicitAutomaton* eaut = getAut(args[1],opt->filelocation,opt->mm);
            ClassicAut caut(*eaut);
            ClassicEpsAut ceaut = classicToClassicEps(caut,eaut->type==0);
            delete eaut; eaut = NULL;
            MultiCounterAut* red = finitePowerCaut(ceaut);

            if(!opt->canWrite()) throw std::runtime_error("No directory found to write data to.");

            std::string path = opt->make_path_prefix(true)+"_red";
            try{
                writeToFile(path,to_stamina(*red));
                delete red; red=NULL;
            }
            catch(const std::exception& e){
                delete red; red=NULL;
                std::string msg = e.what();
                throw runtime_error("Failed to write cost automaton of finite power reduction to file: "+msg);
            }
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="gen"){
        if(args.size()>=5){
            int N, A, C;
            int M;
            int seed;

            N = stoi(args[1]);
            A = stoi(args[2]);
            C = stoi(args[3]);
            M = stoi(args[4]);
            if(args.size()>=6){
                seed = stoi(args[5]);
            }
            else{
                seed = M;
            }
            srand(seed);

            if(!opt->canWrite()) throw std::runtime_error("No directory found to write data to.");

            if(opt->verbose()) std::cout << ">Generating and writing random cost automata.. ";

            MultiCounterAut* rnd = NULL;
            for(int m = 0; m < M; m++){
                opt->setline(m+1);
                rnd = newRndMultiCounterAut(N,A,C);

                std::string path = opt->make_path_prefix(true)+"_gen";
                try{
                    writeToFile(path,to_stamina(*rnd));
                    delete rnd; rnd=NULL;
                }
                catch(const std::exception& e){
                    delete rnd; rnd=NULL;
                    std::string msg = e.what();
                    if(opt->verbose()) std::cout << std::endl;
                    throw runtime_error("Failed to write random cost automaton to file: "+msg);
                }
            }
            if(opt->verbose()) std::cout << "Done!" << std::endl;
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="testsccs"||args[0]=="testscc"){
        if(args.size()>=3){
            ExplicitAutomaton* eaut = getAut(args[1],opt->filelocation,opt->mm);
            ClassicAut aut(*eaut);
            std::vector<state> list = getStateList(args[2],opt->mm);
            if(opt->verbose()) std::cout << std::endl;
            bool passed = testStronglyConnectedComponents(aut,list,opt);
            delete eaut; eaut = NULL;
            if(opt->verbose()){
                if(passed){
                    std::cout << ">Passed SCC test!" << std::endl;
                }
                else{
                    std::cout << ">Failed SCC test!" << std::endl;
                }
            }
            else{
                std::cout << ">" << passed << std::endl;
            }
            opt->tests++;
            if(passed){
                opt->passed++;
            }
            else{
                opt->add_result("Failed");
            }
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="testlim"){
        if(args.size()>1){
            std::vector<state> list;
            std::string mstr = "";
            if(startsWith(trim(args[1]),"#")){
                list = getStateList(args[1],opt->mm);    //ranges for rnd aut gen
                if(list.size()<7) throw invalid_argument("Needed 7 values, only got "+std::to_string(list.size())+".");
                if(args.size()>2){
                    mstr = trim(args[2]);
                }
            }
            else{
                if(args.size()>7){
                    for(int i = 1; i < 8; i++){
                        list.push_back(stoi(trim(args[i])));
                    }

                    if(args.size()>8){
                        mstr = trim(args[8]);
                    }
                }
                else{
                    noarg = true;
                }
            }

            if(!list.empty()){
                bool modes = false;
                if(!mstr.empty()){
                    if(mstr=="true"||mstr=="1"){
                        modes = true;
                    }
                    else if(mstr=="false"||mstr=="0"){
                        modes = false;
                    }
                    else{
                        throw invalid_argument("Could not parse modes flag '"+mstr+"'. Must be either true or false.");
                    }
                }

                bool passed = testLimitedness({list[0],list[1]},{list[2],list[3]},{list[4],list[5]},list[6],opt,modes);
                if(opt->verbose()){
                    std::string limtest = (modes ? "modes " : "");
                    if(passed){
                        std::cout << ">Passed "<<limtest<<"limitedness test!" << std::endl;
                    }
                    else{
                        std::cout << ">Failed "<<limtest<<"limitedness test!" << std::endl;
                    }
                }
                else{
                    std::cout << ">" << passed << std::endl;
                }
                opt->tests++;
                if(passed){
                    opt->passed++;
                }
                else{
                    opt->add_result("Failed");
                }
                return;
            }
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="testts"){
        if(args.size()>1){
            ExplicitAutomaton* eaut = getAut(args[1],opt->filelocation,opt->mm);
            MultiCounterAut* caut = explicitToMultiCounter(*eaut);
            bool passed = testTS(*caut,opt);
            delete eaut; eaut = NULL;
            delete caut; caut = NULL;
            if(opt->verbose()){
                if(passed){
                    std::cout << ">Passed transition set test!" << std::endl;
                }
                else{
                    std::cout << ">Failed transition set test!" << std::endl;
                }
            }
            else{
                std::cout << ">" << passed << std::endl;
            }
            opt->tests++;
            if(passed){
                opt->passed++;
            }
            else{
                opt->add_result("Failed");
            }
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="testmdet"){
        if(args.size()>1){
            ExplicitAutomaton* eaut = getAut(args[1],opt->filelocation,opt->mm);
            MultiCounterAut* caut = explicitToMultiCounter(*eaut);
            testModesDet(*caut,opt);
            delete eaut; eaut = NULL;
            delete caut; caut = NULL;
            return;
        }
        else{
            noarg = true;
        }
    }
    else if(args[0]=="testmlim"){
        if(args.size()>1){
            ExplicitAutomaton* eaut = getAut(args[1],opt->filelocation,opt->mm);
            MultiCounterAut* caut = explicitToMultiCounter(*eaut);
            bool passed = testModesLim(*caut,opt);
            delete eaut; eaut = NULL;
            delete caut; caut = NULL;
            if(opt->verbose()){
                if(passed){
                    std::cout << ">Passed modes lim test!" << std::endl;
                }
                else{
                    std::cout << ">Failed modes lim test!" << std::endl;
                }
            }
            else{
                std::cout << ">" << passed << std::endl;
            }
            opt->tests++;
            if(passed){
                opt->passed++;
            }
            else{
                opt->add_result("Failed");
            }
            return;
        }
        else{
            noarg = true;
        }
    }
    if(args[0]=="set"||args[0]=="settestmodes"){
        std::vector<int> setmodes;
        for(int i = 1; i < args.size(); i++){
            int m = stoi(args[i]);
            if(m>=0&&m<MODES){
                setmodes.push_back(m);
            }
            else{
                throw invalid_argument("Mode does not exist: "+std::to_string(i));
            }
        }
        if(setmodes.empty()) r.o.testmodes = std::vector<bool>(MODES,true);
        else{
            r.o.testmodes = std::vector<bool>(MODES,false);
            for(int i : setmodes){
                r.o.testmodes[i] = true;
            }
        }

        return;
    }
    else if(!opt->filelocation.empty()&&args[0]=="stats"){
        if(args.size()>=2){
            std::string path = getPath(args[1],opt->filelocation);
            std::vector<std::string> data = r.o.make_data_results();
            std::string content = "\nStatistics["+opt->make_prefix()+"]:";
            for(size_t i = 0; i < data.size(); i++){
                content += "\n"+data[i];
            }
            content += "\n";
            writeToFile(path,content,true);
            r.o.reset_data();
            return;
        }
        else{
            noarg = true;
        }
    }
    if(noarg){
        throw invalid_argument("Ignored command \""+args[0]+"\" because not all mandatory arguments were given.");
    }
    throw invalid_argument("Ignored unknown command \""+args[0]+"\".");
}

void handleFilePath(std::string path, Options* opt){
    ResetOPT r(opt);
    opt->filelocation = getPath(path,opt->filelocation);
    std::ifstream file(opt->filelocation , std::ios::in);
    if(file.is_open()){
        string content;
        bool read = false;
        try{
            content = readFile(file);
            read = true;
        }
        catch(const runtime_error& e){
        }
        file.close();
        if(read){
            handleFile(content,opt);
            return;
        }
    }
    throw runtime_error("Could not open file at path \""+opt->filelocation+"\".");
}

void handleFile(std::string fileContent, Options* opt){
    ResetOPT r(opt);

    if(opt->verbose()){
        std::cout << ">Parsing file at path \""<<opt->filelocation<<"\"..." << std::endl;
    }

    size_t linecount;

    //handle task
    StringParser taskp(fileContent);
    while(taskp.remaining()>0){
        linecount = 1+taskp.line;
        std::string line = trim(taskp.advanceLine());
        if(line.empty()) continue;
        if(startsWith(line,"-")){       //update options
            try{
                opt->update(line);
            }
            catch(const invalid_argument& e){
                //invalid option update
                if(opt->verbose()){
                    std::cout << ">Ignored invalid option update at line "<<linecount<<": " <<e.what()<< " ("<<opt->filelocation<<")"<< std::endl;
                }
            }
        }
        else if(startsWith(line,"#")){      //macro definition
            try{
                updateMacroMap(line,taskp,opt->mm);
            }
            catch(const invalid_argument& e){
                //invalid macro definition
                if(opt->verbose()){
                    std::cout << ">Ignored invalid macro definition starting at line "<<linecount<<": " << e.what() << " ("<<opt->filelocation<<")"<< std::endl;
                }
            }
        }
        else if(startsWith(line,"%")){
            //comment
            continue;
        }
        else{     
            //command
            opt->setline(linecount);
            try{
                handleCommand(line,opt);
            }
            catch(const exception& e){
                std::string msg = e.what();
                opt->add_result("ERROR("+msg+")");
                if(opt->verbose()){
                    std::cout << ">Error running command at line "<<linecount<<": " << e.what() << " ("<<opt->filelocation<<")"<< std::endl;
                }
            }
        }
    }
}