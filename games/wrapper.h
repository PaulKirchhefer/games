#pragma once
#include "Automata.hpp"
#include "parity.h"
#include "RegExp.hpp"
#include "StarHeight.hpp"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "options.h"
#include "parser.h"
#include <chrono>
#include "output.h"
#include "condition.h"

class Alphabet;

/*
SOURCES:
	OINK		https://github.com/trolando/oink
	SPOT:		https://spot.lrde.epita.fr/
	STAMINA:	https://stamina.labri.fr/
*/

//solve the parity game using OINK, returns for each state of the game graph the player that wins on it
std::vector<Player> solveParityGame(const ParityGame& parityGame, Options* opt);

//determinizes the parity automaton using SPOT
//if deleteAut is true, deletes parityAut when it is no longer needed
DetParityAut determinizeParityAut(ParityAut* parityAut, Options* opt, bool deleteAut = false, const Alphabet* alph = NULL);

//uses STAMINA to determine limitedness of caut
bool isLimitedSTAMINA(const MultiCounterAut& caut, Options* opt, std::string* witness = NULL);

//uses STAMINA to determine the starheight of aut
int starheightSTAMINA(const ClassicAut& aut, Options* opt);

//uses STAMINA to reduce the star height problem to the limitedness problem
//caller is responsible for deleting the result
MultiCounterAut* reductionSTAMINA(const ClassicAut& aut, unsigned int k, Options* opt);

//uses STAMINA to give an upper bound on the starheight
unsigned int maxStarHeight(const ClassicAut& aut);

//transforms input ClassicAut into a ClassicEpsAut
//if makeEps is true, interprets the last letter of input ClassicAut as epsilon
//uses data structures of STAMINA
ClassicEpsAut classicToClassicEps(const ClassicAut& aut, const bool makeEps = false);

//transform input ExplicitAutomaton into a MultiCounterAut
//caller is responsible for deleting the result
//uses data structures of STAMINA
MultiCounterAut* explicitToMultiCounter(const ExplicitAutomaton& eaut);

//IO
void printNFA(const ClassicEpsAut& eaut);

//returns a string readable by STAMINA, so that STAMINA may reobtain caut by parsing it
//uses data structures of STAMINA
std::string to_stamina(MultiCounterAut& caut);