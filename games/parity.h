#pragma once
#include "parser.h"
#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <set>
#include <stdexcept>
#include <set>
#include <stack>
#include <algorithm>

typedef unsigned int parity;

//returns the optimal parity out of the two for player even
parity optEven(const parity p1, const parity p2);

typedef unsigned long letter;
typedef unsigned int state;

//base for det/nondet parity aut
class AbsParityAut
{
public:
	virtual ~AbsParityAut() {}

	//number of states
	state N;

	//size of the alphabet
	letter A;

	//parity of each state
	std::vector<parity> par;

	virtual bool isInitial(state p) const = 0;

	virtual std::string to_string() const = 0;

	virtual std::string to_dot(bool labelTransitions = true) const = 0;

	virtual std::string to_hoa(const bool buchi = false) const = 0;

	void print() const;

	virtual bool isDet() const = 0;

	parity maxParity() const;

protected:

	void initialize(const state numberOfStates, const letter sizeOfAlphabet);

	std::string hoa_preamble(const bool buchi = false) const;
};

//possibly nondet parity aut
class ParityAut : public AbsParityAut
{
public:
	ParityAut(const state numberOfStates, const letter sizeOfAlphabet);

	//string for outgoing transitions in hoa format(compressed)
	//V -> hint
	std::vector<std::string> hoa_hint;

	//transitions V x A x V
	//std::vector<std::vector<std::vector<bool>>> trans;
	std::vector<std::vector<std::set<state>>> trans;

	//initial states
	//std::vector<bool> init;
	std::set<state> initial;

	//decides wether this parity automaton is deterministic
	bool isDet() const;

	//returns true iff p is an initial state
	bool isInitial(state p) const;

	//removes unreachable states and states that can not repeatedly reach an even parity state
	//oldindices used for io
	//return true iff the initial state has not been removed
	bool prune(std::vector<state>* oldindices = NULL);

	bool prune(const std::vector<std::set<state>>& out, const std::vector<std::set<state>>& in, std::vector<state>* oldindices = NULL);

	//computes the percentage of entries that are true in the transition matrix
	double transMatrixFilled() const;

	//IO

	std::string to_string() const;

	std::string to_dot(bool labelTransitions = true) const;

	std::string to_hoa(const bool buchi = false) const;

	inline bool isTrans(const state from, const letter over, const state to) const{
		return trans[from][over].count(to)>0;
	}

	inline void addTrans(const state from, const letter over, const state to){
		trans[from][over].insert(to);
	}

	inline void addTrans(const state from, const letter over, const std::set<state>& to){
		trans[from][over].insert(to.begin(),to.end());
	}

	inline std::set<state> getSuccessors(const state from, const letter over) const{
		return trans[from][over];
	}

	inline void addInitial(const state i){
		initial.insert(i);
	}

	inline std::set<state> getInitial() const{
		return initial;
	}

	//different representation of connectivity
	void calculateNeighbors(std::vector<std::set<state>>& out, std::vector<std::set<state>>& in) const;
};

class DetParityAut : public AbsParityAut
{
public:
	//true if reject is valid
	bool sinkIdentified = false;
	//>=N if there is no rejecting sink, otherwise index of rejecting sink state
	state reject;

	DetParityAut(const state numberOfStates, const letter sizeOfAlphabet);

	//deterministic transitions V x A
	std::vector<std::vector<state>> trans;

	//used for TSETS game construction
	std::vector<std::vector<std::map<state,std::set<letter>>>> hoatrans;

	//initial state
	state init;

	//transform this automaton to accept the complement
	void complement();

	//true
	bool isDet() const;

	std::pair<state,std::vector<state>> stronglyConnectedComponents() const;

	//computes rejecting sink states and merges them into one
	//using Kosarajus Algorithm: https://en.wikipedia.org/wiki/Kosaraju%27s_algorithm
	void identifyAndMergeRejectingSinks();

	//returns true iff p is an initial state
	bool isInitial(state p) const;

	//adds a single state to the automaton
	state addState();

	//IO

	std::string to_string() const;

	std::string to_dot(bool labelTransitions = true) const;

	std::string to_hoa(const bool buchi = false) const;

	//different representation of connectivity
	void calculateNeighbors(std::vector<std::set<state>>& out, std::vector<std::set<state>>& in) const;
};

//parses the hoa string and returns the det parity automaton
//DetParityAut hoaToDetParityAut(std::string hoa, letter alphabetsize);

//compute SCCs using Kosarajus Algorithm: https://en.wikipedia.org/wiki/Kosaraju%27s_algorithm
std::pair<state,std::vector<state>> computeSCCs(const state N, const std::vector<std::set<state>>& outgoing, const std::vector<std::set<state>>& incoming);

//player even, player odd
enum Player {
	P0, P1
};

//P0 wins iff max inf. parity is even!
class ParityGame
{
public:
	ParityGame(const state numberOfStatesP0, const state numberOfStatesP1);

	//V = v_0, .. v_(N0-1), v_(N0+0), .. v_(N0+N1) = V0,V1
	//number of states
	state N;
	//number of states of player 1
	state N0;
	//number of states of player 2
	state N1;

	//parity of each state
	std::vector<parity> par;

	//transitions
	//std::vector<std::vector<bool>> trans;
	std::vector<std::set<state>> edges;

	void addSinks();

	parity maxParity() const;

	//IO

	std::string to_string() const;

	std::string to_dot(std::vector<std::string> statenames = {}) const;

	std::string to_pgsolver() const;

	void print() const;

	//computes the percentage of entries that are true in the transition matrix
	double transMatrixFilled() const;

	inline void addEdge(const state from, const state to){
		edges[from].insert(to);
	}

	inline void addEdges(const state from, const std::set<state>& to){
		edges[from].insert(to.begin(),to.end());
	}

	inline std::set<state> getSuccessors(const state from) const{
		return edges[from];
	}

	inline state addStateP1(const parity p){
		edges.push_back({});
		par.push_back(p);
		state res = N;
		N1++;
		N++;
		return res;
	}
};