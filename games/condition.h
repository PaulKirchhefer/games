#pragma once
#include "Automata.hpp"
#include "parity.h"
#include "wrapper.h"
#include "games.h"
#include <limits.h>
#include <unordered_map>
#include <stack>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>

//io/debug purposes, returns a representation of the states in the set
std::string stateset_to_string(std::vector<bool> stateset);

//states of the parity game during construction in makeParityGame
struct gamestate{
	//state of the deterministic part
	state det;
	//reachable states of the cost automaton reading the letters picked by player A
	std::vector<bool> reach;	//TODO: determinize, minimise cost automaton -> state reach
	//reachable states of the cost automaton using transitions picked by player B
	std::vector<bool> reachrestricted;
	//the last letter picked by player A (or A)
	letter last;				//last = a -> player B's turn, picks a transition set; last = A -> player A's turn, sets the letter last

	gamestate();
	gamestate(state det, std::vector<bool> reach, std::vector<bool> reachrestricted, letter last);

	bool operator==(const gamestate other) const;

	std::string to_string(const letter A = -1) const;
};

//states used in the subgame of makeParityGame
struct seqstate{
	//state of the deterministic part
	state det;
	//reachable states of the cost automaton using transitions picked by player B after reading the current transition sequence
	std::vector<bool> reachnew;

	seqstate();
	seqstate(state det, std::vector<bool> reachnew);

	bool operator==(const seqstate other) const;

	std::string to_string() const;
};

//index type of transitions of the cost automaton
typedef unsigned long trans;
//type representing sets of transitions of the cost automaton; these tsets will be transitions of the winning condition game
typedef unsigned long tset;
//type representing the counter actions of the cost automaton
typedef unsigned char cact;

//calls STAMINA code to determine if the given code ct is a reset/increment of counter c
bool isReset(const cact ct, const char c);
bool isInc(const cact ct, const char c);

//the alphabet of the winning condition game
//mainly maps the transitions to a number and back
//transitions (from,over,to) are lexicographically ordered by over,from,to
// thus (0,a,0) < (0,a,1) < (1,a,0) < (0,b,0) .. < (N-1,A-1,N-1)
class Alphabet {
public:
	//number of transition sets
	tset TS;
	//number of transitions of the cost automaton
	trans T;
	//number of letters of the cost automaton
	tset A;
	//number of states of the cost automaton
	state N;
	//number of counters
	cact C;

	//lookup by using binary search
	// T -> Q[to]
	std::vector<state> transitions;
	// T -> C
	std::vector<cact> counterActions;
	// A -> TS
	std::vector<tset> letterSetIndices;
	// A -> T
	std::vector<trans> letterIndices;
	// A x Q[from] -> T
	std::vector<std::vector<trans>> fromIndices;

	//A -> offset
	std::vector<tset> letterOffset;
	//number of transitions over a given letter
	std::vector<trans> letterT;
	//used for hint
	trans bits;
	trans tsbits;

	Alphabet();
	Alphabet(const MultiCounterAut& aut, const bool computeSets = false, const bool compress = false);

	//index of the first transition of the next letter - or T
	trans nextLetter(const letter a) const;
	//index of the first transition from the next state - or nextLetter
	trans nextFrom(const letter over, const state from) const;
	//nextLetter but for transition sets
	tset nextLetterSet(const letter a) const;

	//t may be a letter(t<A) or a transition set (t>=A)
	bool isLetter(const tset t) const;

	//return the letter t is over
	letter over(const trans t) const;
	//over for transition sets
	letter setover(const tset t) const;
	//return the start state of t
	state from(const trans t) const;
	//from but avoids computation - use if it is know over which letter a transition t is
	state from(const trans t, const letter a) const;
	//return the end state of t
	state to(const trans t) const;

	//looks up the ID of the given transition, assumes it exists
	trans toTransition(const letter over, const state from, const state to) const;

	//checks whether the given transition t resets/icrements counter c
	bool isResetTrans(const trans t, const char c) const;
	bool isIncTrans(const trans t, const char c) const;

	//reachable states from P using ts over letter a
	std::vector<bool> deltaRestricted(const std::vector<bool>& P, const letter a, const tset ts) const;

	//io/debug
	std::string trans_to_string(const trans t) const;
	std::string to_string() const;
	std::string to_string_short() const;

	std::string hint(const trans t) const;

	void print() const;
	void printShort() const;
};

std::size_t hashcombine(std::size_t hash1, std::size_t hash2);

namespace std{
template<>
struct hash<gamestate>{
	std::size_t operator()(const gamestate& p) const;
};
}

namespace std{
template<>
struct hash<seqstate>{
	std::size_t operator()(const seqstate& p) const;
};
}