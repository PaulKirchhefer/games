#include "condition.h"

Alphabet::Alphabet(){
}

Alphabet::Alphabet(const MultiCounterAut& aut, const bool computeSets, const bool compress)
{
	A = aut.NbLetters;
	N = aut.NbStates;
	C = aut.NbCounters;

	letterOffset = std::vector<tset>(A,0);

	//compute transitions
	trans ct = 0;
	letterIndices.resize(A);
	for (letter a = 0; a < A; a++) {
		auto mat = aut.get_trans(a);
		letterIndices[a] = ct;
		std::vector<trans> from;
		from.resize(N);
		for (state p = 0; p < N; p++) {
			from[p] = ct;
			for (state q = 0; q < N; q++) {
				cact cc = mat->get(p, q);
				//assuming omega is not contained.
				if (!MultiCounterMatrix::is_bottom(cc)) {
					counterActions.push_back(cc);
					transitions.push_back(q);
					ct++;
				}
			}
		}
		fromIndices.push_back(from);
	}
	T = transitions.size();

	letterT = std::vector<trans>(A,0);
	for(letter a = 0; a < A; a++){
		letterT[a] = nextLetter(a) - letterIndices[a];
	}

	TS = 0;
	if(!computeSets) return;
	//compute transition sets

	if(compress){
		//adapt alphabet to allow for compressed hoa file for determinization
		//transitions will be over letters with bit representation: [[bit repr. of 'a'] ? ? .. t_i=1 .. ?], where the transition set if over 'a' and contains transition t_i
		trans maxnt = 0;
		for (letter a = 0; a < A; a++) {
			trans nt = letterT[a];
			maxnt = std::max(nt,maxnt);
		}
		trans na_ = 1;
		trans na = 0;
		while(na_<A){
			na_ = na_<<1;
			na++;
		}
		if(maxnt+na > CHAR_BIT * sizeof(tset)){
			throw overflow_error("ALPHABET OVERFLOW: bits for transitions: " + std::to_string(maxnt)+", bits for letters: "+std::to_string(na));
		}
		tsbits = maxnt;
		bits = maxnt+na;
		tset off = 1;
		off = off<<maxnt;
		for(letter a = 0; a < A; a++){
			letterOffset[a] = a * off;
		}
	}
	
	tset cts = 0;
	letterSetIndices.resize(A);
	for (letter a = 0; a < A; a++) {
		letterSetIndices[a] = cts;
		trans nt = nextLetter(a)-letterIndices[a];
		if (nt > CHAR_BIT * sizeof(tset)) {	//a single transition set is larger than tset can store
			throw overflow_error("ALPHABET OVERFLOW: transition set of size " + std::to_string(nt));
		}
		if (nt == 0) continue;
		tset nts = 1;
		nts = (nts << nt);
		tset cts_ = cts + nts;
		if (cts_ < cts) {		//overflow; the transition sets up to now are larger than tset can store
			throw overflow_error("ALPHABET OVERFLOW: too many transitions sets: " + std::to_string(cts) + " + " + std::to_string(nts));
		}
		cts = cts_;
	}
	TS = cts;
}

trans Alphabet::nextLetter(const letter a) const{
	if(a + 1 < A){
		return letterIndices[a+1];
	}
	return T;
}

trans Alphabet::nextFrom(const letter over, const state from) const{
	trans t1;
	if (from + 1 < N) {
		t1 = fromIndices[over][from + 1];
	}
	else {
		if (over + 1 < A) {
			t1 = fromIndices[over + 1][0];
		}
		else {
			t1 = T;
		}
	}
	return t1;
}

tset Alphabet::nextLetterSet(const letter a) const{
	if(a+1>=A) return TS;
	return letterSetIndices[a+1];
}

bool Alphabet::isLetter(const tset t) const
{
	return t < A;
}

letter Alphabet::over(const trans t) const
{
	return binarySearch(letterIndices, t);
}

letter Alphabet::setover(const tset t) const
{
	return binarySearch<tset>(letterSetIndices, t);
}

state Alphabet::from(const trans t) const
{
	return from(t, over(t));
}

state Alphabet::from(const trans t, const letter a) const
{
	return binarySearch<trans>(fromIndices[a], t);
}

state Alphabet::to(const trans t) const
{
	return transitions[t];
}

trans Alphabet::toTransition(const letter over, const state from, const state to) const
{
	trans t0 = fromIndices[over][from];
	trans t1 = nextFrom(over, from);
	for (trans t = t0; t < t0; t++) {
		if (this->to(t) == to) return t;
	}
	return T;	//doesnt exist
}

bool isReset(const cact ct, const char c){//USES STAMINA CODE!
	if(MultiCounterMatrix::is_reset(ct)){
		return MultiCounterMatrix::get_reset_counter(ct)<=c;
	}
	else if(MultiCounterMatrix::is_inc(ct)){
		return MultiCounterMatrix::get_inc_counter(ct)<c;
	}
	return false;
}
bool isInc(const cact ct, const char c){//USES STAMINA CODE!
	return MultiCounterMatrix::get_inc_counter(ct)==c;
}

bool Alphabet::isResetTrans(const trans t, const char c) const{
	return isReset(counterActions[t],c);
}

bool Alphabet::isIncTrans(const trans t, const char c) const{
	return isInc(counterActions[t],c);
}

std::vector<bool> Alphabet::deltaRestricted(const std::vector<bool>& P, const letter a, const tset ts) const{
	std::vector<bool> res(N,false);
	tset offset = letterSetIndices[a];
	tset cts = ts - offset;
	tset max = nextLetterSet(a) - offset;
	trans t = letterIndices[a];
	//for each transition t over a
	while(cts!=0&&cts<max){
		//if t is contained in ts
		if(cts%2==1){
			//if from(t) is contained in P
			if(P[from(t,a)]){
				//add to(t) to res
				res[to(t)] = true;
			}
		}
		t++;
		cts = cts >> 1;
	}
	return res;
}

std::string Alphabet::to_string_short() const{
	return "Alphabet: "+std::to_string(T)+" transitions over "+std::to_string(A)+" letters and " + std::to_string(N) + " states" + (TS==0?".":", resulting in "+std::to_string(TS)+" transition sets.");
}

std::string Alphabet::hint(const trans t) const{
	letter a = over(t);
	std::string res = std::to_string(t-letterIndices[a]);
	tset off = letterOffset[a];
	for(trans b = 0; b < bits; b++){
		if(b<letterT[a]){
			off = off >> 1;
			continue;
		}
		res+="&";
		if((off%2)==1){
			res+=std::to_string(b);
		}
		else{
			res+="!"+std::to_string(b);
		}
		off = off >> 1;
	}
	return res;
}

std::string Alphabet::trans_to_string(const trans t) const{
	letter a = over(t);
	state p = from(t,a);
	state q = to(t);
	cact cact = counterActions[t];
	char aa = 'a'+a;
	std::string res = "("+std::to_string(p)+","+aa+",";
	if(MultiCounterMatrix::is_epsilon(cact)) res+="E";	//USES STAMINA CODE!
	else if(MultiCounterMatrix::is_omega(cact)) res+="O";
    else if(MultiCounterMatrix::is_reset(cact)) res += "R"+std::to_string(MultiCounterMatrix::get_reset_counter(cact));
    else if(MultiCounterMatrix::is_inc(cact)) res += "I"+std::to_string(MultiCounterMatrix::get_inc_counter(cact));
    else res += "_";
	res += ","+std::to_string(q)+")";
	return res;
}

std::string Alphabet::to_string() const{
	std::string res = "";
	res += to_string_short()+"\n";
	for(letter a = 0; a < A; a++){
		res += "" + std::to_string((nextLetter(a)-letterIndices[a])) + " transitions over letter " + std::to_string(a) + ":\n";
		for(state p = 0; p < N; p++){
			res += "" +std::to_string((nextFrom(a,p)-fromIndices[a][p]))+ " transitions["+std::to_string(fromIndices[a][p])+"+] from state "+std::to_string(p)+" over letter "+std::to_string(a)+" to state:";
			for(trans t = fromIndices[a][p]; t < nextFrom(a,p); t++){
				res += " "+std::to_string(transitions[t]);
			}
			res += "\n";
		}
	}
	res += "" + std::to_string(TS) + " transition sets in total.";
	return res;
}

void Alphabet::print() const{
	std::cout << to_string() << std::endl;
}

void Alphabet::printShort() const{
	std::cout <<">"<< to_string_short() << std::endl;
}

std::size_t hashcombine(std::size_t hash1, std::size_t hash2){
	return hash1 ^ hash2;
}

std::string stateset_to_string(std::vector<bool> stateset){
	std::string res = "{";
	bool found = false;
	for(state p = 0; p < stateset.size(); p++){
		if(stateset[p]){
			if(found){
				res += ",";
			}
			res += to_string(p);
			found = true;
		}
	}
	res += "}";
	return res;
}

gamestate::gamestate(){
	det = 0; reach = {}; reachrestricted = {}; last = 0;
}
gamestate::gamestate(state det, std::vector<bool> reach, std::vector<bool> reachrestricted, letter last){
	this->det = det; this->reach = reach; this->reachrestricted = reachrestricted; this->last = last;
}

bool gamestate::operator==(const gamestate other) const{
	return (det==other.det)&&(reach==other.reach)&&(reachrestricted==other.reachrestricted)&&(last==other.last);
}

std::string gamestate::to_string(const letter A) const{
	std::string res = "(";
	res += std::to_string(det);
	res += "," + stateset_to_string(reach);
	res += "," + stateset_to_string(reachrestricted) + ",";
	if(last<A){
		char a = 'a'+last;
		res += a;
	}
	else{
		res += "A";
	}
	res += ")";
	return res;
}

std::size_t hash<gamestate>::operator()(const gamestate& p) const{
	std::size_t hash;
	std::hash<state> h1;
	hash = h1(p.det);
	std::hash<std::vector<bool>> h2;
	hash = hashcombine(hash, h2(p.reach));
	hash = hashcombine(hash, h2(p.reachrestricted));
	std::hash<letter> h3;
	hash = hashcombine(hash, h3(p.last));
	return hash;
}

seqstate::seqstate(){
	det = 0; reachnew = {};
}

seqstate::seqstate(state det, std::vector<bool> reachnew){
	this->det = det; this->reachnew = reachnew;
}

bool seqstate::operator==(const seqstate other) const{
	return (det==other.det)&&(reachnew==other.reachnew);
}

std::string seqstate::to_string() const{
	std::string res = "(";
	res += std::to_string(det);
	res += "," + stateset_to_string(reachnew) + ")";
	return res;
}

std::size_t hash<seqstate>::operator()(const seqstate& p) const{
	std::size_t hash;
	std::hash<state> h1;
	hash = h1(p.det);
	std::hash<std::vector<bool>> h2;
	hash = hashcombine(hash, h2(p.reachnew));
	return hash;
}