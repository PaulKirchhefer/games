#pragma once
#include "condition.h"
#include "parity.h"

class Alphabet;

//specific to_dot method for ParityAut that were constructed for wc2
std::string nondetpart_to_dot(const ParityAut nondet, const Alphabet alph, const bool alt, const bool labelTransitions = true, std::vector<state>* oldindices = NULL);

//parses the hoa string and returns the det parity automaton
//if alph!=NULL the returned DetParityAut has INVALID DPA.trans, ie DPA.A does not match DPA.trans[.].size()!
DetParityAut hoaToDetParityAut(std::string hoa, letter alphabetsize, const Alphabet* alph = NULL);