#pragma once
#include "Automata.hpp"
#include "parity.h"
#include "wrapper.h"
#include "condition.h"
#include <limits.h>
#include <unordered_map>
#include <stack>
#include <iostream>
#include <stdexcept>
#include "options.h"
#include "output.h"

class Alphabet;

//reduces the finitepower problem to the limitedness problem
//caller is responsible for deleting the result
MultiCounterAut* finitePowerCaut(const ClassicEpsAut& in);

//transition set approach
ParityAut makeWC2TS(const MultiCounterAut& costAut, const Alphabet& alph, Options* opt);

//construct a parity game for the winning condition of the gale-steward game, according to "Mikolaj Bojanczyk. Star height via games, 2017"
//uses sequences of transitions as input instead of transition sets over letters
//assumes costAut contains no omega transitions
ParityGame makeParityGame(const MultiCounterAut& costAut, Options* opt, const Alphabet& alph);

/*solve the limitedness problem for the input cost automaton, using the approach of "Mikolaj Bojanczyk. Star height via games, 2017"
* uses the tools: OINK, SPOT, STAMINA; see wrapper.h for the sources
*/
bool isLimited(MultiCounterAut& costAut, Options* opt);

//REGEX: not implemented!
unsigned int starheight(const std::string regex, Options* opt, bool useSTAMINA = false);

//computes the starheight of aut, using the approach of "Mikolaj Bojanczyk. Star height via games, 2017"
//uses the tools: OINK, SPOT, STAMINA; see wrapper.h for the sources
unsigned int starheight(ClassicAut& aut, Options* opt);

//REGEX: not implemented!
bool finitePower(const std::string regex, Options* opt, bool useSTAMINA = false);

//decides if aut has the finite power property, using the approach of "Mikolaj Bojanczyk. Star height via games, 2017"
//uses the tools: OINK, SPOT, STAMINA; see wrapper.h for the sources
//if useSTAMINA is true, the STAMINA is used to decide limitedness instead
bool finitePower(const ClassicAut& aut, Options* opt, bool useSTAMINA = false);

//decides if aut has the finite power property, using the approach of "Mikolaj Bojanczyk. Star height via games, 2017"
//uses the tools: OINK, SPOT, STAMINA; see wrapper.h for the sources
//if useSTAMINA is true, the STAMINA is used to decide limitedness instead
bool finitePower(const ClassicEpsAut& aut, Options* opt, bool useSTAMINA = false);

//makes the non det part of the given mode
//for statistics/testing purposes
ParityAut makeNonDetPartTEST(const MultiCounterAut& costAut, Options* opt);


//binary search..
template<typename comparable>
std::size_t binarySearch(const std::vector<comparable> vec, const comparable t) {
	std::size_t min, max, c;
	std::size_t N = vec.size();
	min = 0;
	max = N - 1;
	while (min != max) {
		c = (min + max) / 2;
		if (t < vec[c]) {
			if (c == 0) return 0;
			if (vec[c - 1] <= t) return c - 1;
			max = c-1;
		}
		else {
			if (c == N - 1) return c;
			if (t < vec[c + 1]) return c;
			min = c+1;
		}
	}
	return min;
}
