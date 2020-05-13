#include "tests.h"

void checkStronglyConnectedComponents(const state N, const std::vector<std::set<state>>& outgoing, const std::vector<std::set<state>>& incoming, const std::vector<state>& scc){
    if(scc.size()!=N){
        throw std::invalid_argument("Testing error: "+std::to_string(N)+" states but size of given SCC vector is "+std::to_string(scc.size())+". Did not check SCCs.");
    }
    auto sccres = computeSCCs(N,outgoing,incoming);
    std::vector<state> tscc = sccres.second;
    std::vector<state> alias(sccres.first);
    std::vector<bool> aliasSet(sccres.first,false);
    std::set<state> sccSet;

    //check if there is a 1-to-1 mapping from scc to tscc
    for(state p = 0; p < N; p++){
        state cc = tscc[p];
        if(aliasSet[cc]){
            if(alias[cc]!=scc[p]){
                throw std::logic_error("Computed[Expected] SCCs: "+to_string(tscc,scc)+". Computed SCC '"+std::to_string(cc)+"' contains states of expected SCCs '"+std::to_string(alias[cc])+"' and '"+std::to_string(scc[p])+"'.");
            }
        }
        else{
            if(sccSet.count(scc[p])>0){
                throw std::logic_error("Computed[Expected] SCCs: "+to_string(tscc,scc)+". Expected SCC '"+std::to_string(scc[p])+"' contains states of multiple computed SCCs.");
            }
            else{
                alias[cc] = scc[p];
                sccSet.insert(scc[p]);
                aliasSet[cc] = true;
            }
        }
    }
}

bool testStronglyConnectedComponents(const ClassicAut& aut, const std::vector<state>& scc, Options* opt){
    ClassicAut ncaut = aut;
    //ncaut.addsink();

    state N = ncaut.NbStates;
    letter A = ncaut.NbLetters;

    // V -> out/in-neighbors
	std::vector<std::set<state>> out(N);
	std::vector<std::set<state>> in(N);

	for(char a = 0; a < A; a++){
        for(state p = 0; p < N; p++){
            for(state q = 0; q < N; q++){
                if(ncaut.trans[a][p][q]){
                    out[p].insert(q);
                    in[q].insert(p);
                }
            }
        }
    }

    try{
        checkStronglyConnectedComponents(N, out, in, scc);
        return true;
    }
    catch(const logic_error& e){
        if(opt->verbose()){
            std::cout << ">Error: " << e.what() << std::endl;
            //std::cout << ">Automaton of SCC test was:" << std::endl;
            //ncaut.print();
        }
        return false;
    }
}

MultiCounterAut* newRndMultiCounterAut(state N, letter A, cact C){
    MultiCounterAut* caut = new MultiCounterAut(A,N,C);
    cact max = 2*C+2;
    cact max_= max-1;
    caut->initialstate[0] = true;
    for(letter a = 0; a < A; a++){
        ExplicitMatrix mat(N);
        for(state p = 0; p < N; p++){
            if(rand()%2==0){
                caut->finalstate[p] = true;
            }
            for(state q = 0; q < N; q++){
                cact act = rand()%max;
                if(act==max_) act++;
                mat.coefficients[p][q] = act;
            }
        }
        caut->set_trans(a,new MultiCounterMatrix(mat));
    }
    return caut;
}

bool testLimitedness(const testrange states, const testrange letters, const testrange counters, unsigned int tests, Options* opt, const bool modes){
    ResetOPT r(opt);

    unsigned int passed = 0;
    unsigned int total = 0;
    const unsigned int maxtests = (1+states.second-states.first) * (1+letters.second-letters.first) * (1+counters.second-counters.first) * tests;

    const bool output = opt->output()&&opt->canWrite();
    const bool progress = opt->verbose();
    opt->o=FALSE;
    opt->v=FALSE;
    opt->d=FALSE;

    std::srand(tests);
    for(state N = std::max((unsigned int) states.first,1U); N <= states.second; N++){
        for(letter A = std::max((unsigned int) letters.first, 1U); A <= letters.second; A++){
            for(cact C = std::max((unsigned int) counters.first, 0U); C <= counters.second; C++){
                for(unsigned int t = 0; t < tests; t++){
                    opt->setline(total+1);
                    MultiCounterAut* caut = newRndMultiCounterAut(N,A,C);
                    bool failed = false;

                    if(modes){
                        if(progress) opt->v=TRUE;
                        failed = !testModesLim(*caut,opt);
                        opt->v=FALSE;
                        if(failed&&output){
                            try{
                                std::string path = opt->make_path_prefix(true) + "_limMtest";
                                std::string content = "%Modes?\n";
                                content += to_stamina(*caut);
                                writeToFile(path,content);
                            }
                            catch(...){};
                        }
                    }
                    else{
                        std::string witness = "";
                        char lim,stam;
                        lim = -1;
                        stam = -1;
                        try{
                            lim = isLimited(*caut,opt);
                        }
                        catch(...){
                            failed = true;
                        }
                        try{
                            stam = isLimitedSTAMINA(*caut,opt,&witness);
                        }
                        catch(...){
                            failed = true;
                        }
                        if(!failed) failed = (lim!=stam);
                        if(failed){
                            if(output){
                                try{
                                    std::string path = opt->make_path_prefix(true) + "_limtest";
                                    std::string content = "%"+to_string(lim)+"?"+to_string(stam);
                                    if(!stam) content += "("+witness+")";
                                    content += "\n"+to_stamina(*caut);
                                    writeToFile(path,content);
                                }
                                catch(...){}
                            }
                        }
                    }

                    if(!failed){
                        passed++;
                    }
                    total++;
                    delete caut; caut=NULL;

                    if(progress){
                        if(total<maxtests) std::cout << ">Progress: " << total << "/" << maxtests << ".. " << std::endl;
                        else std::cout << ">Done!" << std::endl;
                    }
                }
            }
        }
    }
    opt->setline(0);
    opt->add_result(to_string(passed)+"/"+to_string(total));
    return passed==total;
}

bool testTS(const MultiCounterAut& caut, Options* opt){
    int modeold = opt->g;
    try{
        opt->g=8;
        Alphabet alph(caut,true,opt->compressHoa());
        if(opt->verbose()) alph.print();
        ParityAut wc2 = makeWC2TS(caut,alph,opt);
        if(opt->verbose()||opt->debug()) std::cout << ">Non det automaton: "<<wc2.N << " states." << std::endl;
        DetParityAut dpa = determinizeParityAut(&wc2,opt);
        if(opt->verbose()||opt->debug()) std::cout << ">Det automaton: "<<dpa.N << " states." << std::endl;
        opt->g=modeold;
        return true;
    }
    catch(const std::exception& e){
        opt->g=modeold;
        if(opt->verbose()||opt->debug()) std::cout << ">Error during testing of transition set approach: "<<e.what() << std::endl;
    }
    return false;
}

void testModesDet(const MultiCounterAut& caut, Options* opt){
    ResetOPT r(opt);

    if(opt->logdata()) opt->writeToLog("\nTest modes det:\n");

    int success = 0;

    if(opt->verbose()){
        std::cout << ">Testing modes.." << std::endl;
    }

    for(int m = 0; m < MODES; m++){
        if(!opt->testmodes[m]) continue;
        opt->setline(m+1);
        try{
            opt->g=m;
            ParityAut aut = makeNonDetPartTEST(caut,opt);

            DetParityAut dpa = determinizeParityAut(&aut,opt);
            success++;
        }
        catch(const std::exception& e){
            if(opt->verbose()){
                std::cout << ">Failure of mode "+std::to_string(m)+": "<<e.what() << std::endl;
            }
        }
    }

    if(opt->verbose()){
        std::cout << ">Successfully ran "+std::to_string(success)+"/9 modes." << std::endl;
    }
}

bool testModesLim(MultiCounterAut& caut, Options* opt){
    ResetOPT r(opt);
    const bool V = opt->verbose();
    opt->v=FALSE;
    if(opt->logdata()) opt->writeToLog("\n"+opt->make_prefix()+" Test modes lim:\n");

    std::vector<bool> res(MODES,false);
    std::vector<bool> suc(MODES,false);

    if(V){
        std::cout << ">Testing modes.." << std::endl;
    }

    for(int m = 0; m < MODES; m++){
        if(!opt->testmodes[m]) continue;
        opt->setline(m+1);
        opt->g=m;
        try{
            res[m] = isLimited(caut,opt);
            suc[m] = true;
        }
        catch(const std::exception& e){
            if(V){
                std::cout << ">Failure of mode "+std::to_string(m)+": "<<e.what() << std::endl;
            }
        }
    }

    if(opt->compare()){
        if(V){
            std::cout << ">Comparing to STAMINA.." << std::endl;
        }

        opt->setline(MODES+1);

        res.push_back(false);
        suc.push_back(false);

        try{
            res[MODES] = isLimitedSTAMINA(caut,opt);
            suc[MODES] = true;
        }
        catch(const std::exception& e){
            if(V){
                std::cout << ">Failure of STAMINA: "<<e.what() << std::endl;
            }
        }
    }

    bool ress = false;
    bool found = false;

    bool unan = true;
    bool fail = false;

    for(int i = 0; i < res.size(); i++){
        if(!opt->testmodes[i]) continue;
        if(!found){
            if(suc[i]){
                ress = res[i];
                found = true;
            }
            else{
                fail = true;
            }
        }
        else if(suc[i]){
            unan = unan&&(ress==res[i]);
        }
        else{
            fail = true;
        }
    }

    if(V){
        if(unan){
            std::cout << ">All modes computed the same result '"<<ress<<+"'.";
        }
        else{
            std::cout << ">Not all modes computed the same result.";
        }
        if(fail){
            std::cout << " Some modes failed to compute a result.";
        }
        std::cout << std::endl;
    }

    if(opt->logdata()){
        opt->setline(0);
        opt->writeToLog("\n"+opt->make_prefix()+" Test modes lim: u="+(unan ? "T" : "F")+", s="+(fail ? "F" : "T")+"\n");
    }

    return unan;
}