#pragma once
#include "parity.h"
#include <stdexcept>
#include <set>
#include "Automata.hpp"
#include "options.h"
#include <string>
#include "games.h"
#include <cstdlib>

template<typename A>
std::string to_string(const std::vector<A>& actual, const std::vector<A>& expected){
    std::string res = "";
    std::vector<A> exp = expected;
    exp.resize(actual.size());

    for(size_t i = 0; i < actual.size(); i++){
        if(i>0) res += " ";
        res += std::to_string(actual[i])+"["+std::to_string(expected[i])+"]";
    }
    return res;
}

void checkStronglyConnectedComponents(const state N, const std::vector<std::set<state>>& outgoing, const std::vector<std::set<state>>& incoming, const std::vector<state>& scc);

//tests if aut has the given SCCs, returns true iff test is passed
bool testStronglyConnectedComponents(const ClassicAut& aut, const std::vector<state>& scc, Options* opt);

typedef std::pair<unsigned char, unsigned char> testrange;

//generates a random cost automaton
MultiCounterAut* newRndMultiCounterAut(state N, letter A, cact C);

//generates and tests tests amount of random cost automatons for each combination of N,A,C in the given ranges
//tests is also used as a seed for random generation
bool testLimitedness(const testrange states, const testrange letters, const testrange counters, unsigned int tests, Options* opt, const bool modes = false);

//tries to build and determinize the automaton for the complement of wc2 - using the transition set approach
bool testTS(const MultiCounterAut& caut, Options* opt);

//tests each available mode for the nondet part and determinizes
void testModesDet(const MultiCounterAut& caut, Options* opt);

//tests each available mode for calculating the limitedness
//returns true iff all methods computed the same
bool testModesLim(MultiCounterAut& caut, Options* opt);

MultiCounterAut* newRndMultiCounterAut(state N, letter A, cact C);