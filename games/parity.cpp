#include "parity.h"

parity optEven(const parity p1, const parity p2){
	if(p1%2==0){
		if(p2%2==0){
			return std::max(p1,p2);
		}
		return p1;
	}
	else{
		if(p2%2==0){
			return p2;
		}
		return std::min(p1,p2);
	}
}

std::string makeLabel(letter a, letter A){
    std::string label = "";
	unsigned char bit = 0;
	bool first = true;
	for(letter abit = 1; abit < A; abit = abit << 1){
		if(!first){
			label += "&";
		}
		if((a&abit)==0){
			label += "!";
		}
		label += std::to_string(bit);
		first = false;
		bit++;
	}
	return label;
}

void AbsParityAut::initialize(const state numberOfStates, const letter sizeOfAlphabet) {
		N = numberOfStates;
		A = sizeOfAlphabet;
		par = std::vector<parity>(N, 0);
}

void AbsParityAut::print() const{
	//if(N*A <= 500){
	//	std::cout << to_string() << std::endl;
	//}
	//else{
	//	std::cout << "Parity automaton: " << N << " states and over " << A << " letters." << std::endl;
	//}
	std::cout << to_string() << std::endl;
}

parity AbsParityAut::maxParity() const{
	parity max = 0;
	for(state p = 0; p < N; p++){
		max = std::max(max,par[p]);
	}
	return max;
}

std::string AbsParityAut::hoa_preamble(const bool buchi) const{
	std::string res = "HOA: v1\nStates: "+std::to_string(N)+"\nStart: ";
	//find initial states
	bool found = false;
	for(state p = 0; p < N; p++){
		if(isInitial(p)){
			if(found){
				res+="&";
			}
			found = true;
			res += std::to_string(p);
		}
	}
	if(!found){
		//error: no initial state
	}
	//letters
	res +="\nAP: ";
	unsigned char AP = 0;
	for(letter a = A-1; a>0; a = a>>1){
		AP++;
	}
	res += std::to_string(AP);
	for(unsigned char a = 0; a < AP; a++){
		res += " \""+std::to_string(a)+"\"";
	}
	//acceptance
	if(buchi){
		res+= "\nacc-name: Buchi";
		res+= "\nAcceptance: 2 Inf(1)";
	}
	else{
		res += "\nacc-name: parity max even ";
		//find max parity
		parity max = 0;
		for(parity p : par){
			max = std::max(max,p);
		}
		res += std::to_string(max+1)+"\nAcceptance: "+std::to_string(max+1);
		bool first = true;
		parity close = 0;
		for(parity p = max; p<=max&&p>=0; p--){
			if(p%2==0){
				if(!first){
					res += " & (";
					close++;
				}
				res += " Inf("+std::to_string(p)+")";
			}
			else{
				if(!first){
					res += " | (";
					close++;
				}
				res += " Fin("+std::to_string(p)+")";
			}
			first = false;
		}
		for(parity c = close; c > 0; c--){
			res += ")";
		}
	}
	//properties
	res +="\nproperties: state-acc colored";
	if(isDet()){
		res += " deterministic";
	}
	res +="\n--BODY--";
	return res;
}

ParityAut::ParityAut(const state numberOfStates, const letter sizeOfAlphabet) {
		initialize(numberOfStates, sizeOfAlphabet);
		trans = std::vector<std::vector<std::set<state>>>(N,std::vector<std::set<state>>(A,std::set<state>()));
		initial = std::set<state>();
		hoa_hint = std::vector<std::string>();
}

bool ParityAut::isDet() const
{
	if(getInitial().size()!=1) return false;
	for (int i = 0; i < N; i++) {
		for (int a = 0; a < A; a++) {
			if(getSuccessors(i,a).size()!=1) return false;
		}
	}
	return true;
}

bool ParityAut::isInitial(state p) const
{
	return initial.count(p)>0;
}

bool ParityAut::prune(std::vector<state>* oldindices){
	std::vector<std::set<state>> out;
	std::vector<std::set<state>> in;
	calculateNeighbors(out,in);
	return prune(out,in,oldindices);
}

bool ParityAut::prune(const std::vector<std::set<state>>& out, const std::vector<std::set<state>>& in, std::vector<state>* oldindices){

	auto nscc = computeSCCs(N,out,in);
	state M = nscc.first;						//number of SCCs
	std::vector<state> scc = nscc.second;		//state -> SCC

	std::vector<std::set<state>> sccout(M);
	std::vector<std::set<state>> sccin(M);
	std::vector<bool> haseven(M,false);
	//std::vector<bool> singleton(M,true);		//in case an even parity state has no successors
												//		and thus is in a SCC with an even parity, but no acc run is possible

	std::vector<bool> selfloop(M,false);

	for(state p = 0; p < N; p++){
		state pscc = scc[p];
		//if(haseven[pscc]) singleton[pscc] = false;
		if((par[p]%2)==0){
			haseven[pscc] = true;
		}
		for(state q : out[p]){
			state qscc = scc[q];
			if(pscc==qscc){
				selfloop[pscc] = true;
			}
			else{
				sccout[pscc].insert(qscc);
				sccin[qscc].insert(pscc);
			}
		}
	}

	std::vector<bool> removescc(M,true);
	//SCCs that are not reachable are removed
	std::set<state> initscc;
	std::set<state> newinitscc;
	for(state i : getInitial()){
		state scci = scc[i];
		newinitscc.insert(scci);
	}
	while(!newinitscc.empty()){
		initscc.insert(newinitscc.begin(),newinitscc.end());
		newinitscc.clear();

		for(state is : initscc){
			for(state js : sccout[is]){
				if(initscc.count(js)==0){
					newinitscc.insert(js);
				}
			}
		}
	}
	for(state is : initscc) removescc[is] = false;

	bool changed = true;
	while(changed){
		changed = false;
		for(state pscc = 0; pscc < M; pscc++){
			if(removescc[pscc]) continue;
			if(sccout[pscc].size()==0){
				//pscc is a leaf in the tree corresponding to sccout
				//		(a cycle in sccout contradicts the SCC property)
				if(!haseven[pscc]||!selfloop[pscc]){
					//this is leaf allows no accepting run, will be removed
					removescc[pscc] = true;
					changed = true;
					for(state qscc : sccin[pscc]){
						sccout[qscc].erase(pscc);		//might become leaf
					}
				}
			}
		}
	}

	//remove states
	std::vector<state> newindex(N,-1);
	const bool OLD = (oldindices!=NULL);
	if(OLD) oldindices->clear();
	state newN = 0;
	std::vector<parity> newpar;
	std::set<state> newinit;
	for(state p = 0; p < N; p++){
		if(!removescc[scc[p]]){
			newindex[p] = newN;
			if(OLD) oldindices->push_back(p);
			newpar.push_back(par[p]);
			if(isInitial(p)) newinit.insert(newN);
			newN++;
		}
	}
	std::vector<std::vector<std::set<state>>> newtrans(newN,std::vector<std::set<state>>(A,std::set<state>()));
	for(state p = 0; p < N; p++){
		if(removescc[scc[p]]) continue;
		state newp = newindex[p];
		for(letter a = 0; a < A; a++){
			for(state q : getSuccessors(p,a)){
				if(!removescc[scc[q]]) newtrans[newp][a].insert(newindex[q]);
			}
		}
	}

	if(newN==0){
		N = 1;
		initial = {0};
		par = {1};
		trans = std::vector<std::vector<std::set<state>>>(1,std::vector<std::set<state>>(A,{0}));
		oldindices = {0};
		return false;
	}
	else{
		N = newN;
		initial = newinit;
		par = newpar;
		trans = newtrans;
		return true;
	}
}

double ParityAut::transMatrixFilled() const{
	double res = 0;

	for(state p = 0; p < N; p++){
		double res_ = 0;
		for(letter a = 0; a < A; a++){
			double count = getSuccessors(p,a).size();

			res_ += count/N;
		}

		res += res_/A;
	}

	return res/N;
}

std::string ParityAut::to_string() const{
	std::string res = "Parity Automaton:\n";
	res += "" + std::to_string(N) + " states and " + std::to_string(A) + " letters.";
	res += "\nInitial states\\parities:";
	bool found = false;
	for(state p : getInitial()){
		if(found) res += ",";
		found = true;
		res += " "+std::to_string(p)+"["+ std::to_string(par[p]) + "]";
	}
	//res += "\nTransitions over epsilon:";
	//for(state p = 0; p < N; p++){
	//	res +="\n";
	//	for(state q = 0; q < N; q++){
	//		if(epsTrans[p][q]){
	//			res += "1";
	//		}
	//		else{
	//			res += "_";
	//		}
	//		if(q<N-1){
	//			res += " ";
	//		}
	//	}
	//}
	for(letter a = 0; a < A; a++){
		res += "\nTransitions over letter "+std::to_string(a)+":";
		for(state p = 0; p < N; p++){
			res += "\n";
			bool found = false;
			for(state q : getSuccessors(p,a)){
				if(found) res +=", ";
				found = true;
				res += std::to_string(q);
			}
		}
	}
	return res;
}

std::string ParityAut::to_dot(bool labelTransitions) const{
	std::string res = "digraph \"\" {\n  rankdir=LR\n  label=\"ParityAut\"\n  labelloc=\"t\"\n  node [shape=\"circle\"]\n";
	//initial states
	for(state p : getInitial()){
		res += "  I"+std::to_string(p)+" [label=\"\", style=invis, width=0]\n";
		res += "  I"+std::to_string(p)+" -> "+std::to_string(p)+"\n";
	}
	//all states
	for(state p = 0; p < N; p++){
		res += "  "+std::to_string(p)+" [label=\""+std::to_string(p)+"["+std::to_string(par[p])+"]\"]\n";

		for(state q = 0; q < N; q++){
			std::string pq = "  "+std::to_string(p) + " -> "+std::to_string(q);
			if(labelTransitions){
				pq += " [label=\"";
			}
			bool empty = true;
			//if(epsTrans[p][q]){
			//	empty = false;
			//	pq += "e";
			//}
			for(letter a = 0; a < A; a++){
				if(isTrans(p,a,q)){
					if(!labelTransitions){
						empty = false;
						break;
					}
					if(!empty){
						pq += ", ";
					}
					empty = false;
					pq += std::to_string(a);
				}
			}
			if(!empty){
				if(labelTransitions){
					pq += "\"]";
				}
				res += pq+"\n";
			}
		}
	}
	res += "}";
	return res;
}

std::string ParityAut::to_hoa(const bool buchi) const{
	std::string res = hoa_preamble(buchi);
	//states/transitions
	for(state p = 0; p < N; p++){
		res+="\nState: "+std::to_string(p) + "{"+(buchi ? ((par[p]%2)==0 ? "1" : "0") : std::to_string(par[p]))+"}";
		if(p<hoa_hint.size()){
			res+= hoa_hint[p];
			continue;
		}
		for(letter a = 0; a < A; a++){
			std::string label = makeLabel(a,A);
			for(state q : getSuccessors(p,a)){
				res += "\n["+label+"] "+std::to_string(q);
			}
		}
	}
	res += "\n--END--";
	return res;
}

void ParityAut::calculateNeighbors(std::vector<std::set<state>>& out, std::vector<std::set<state>>& in) const{
	out.resize(N);
	in.resize(N);

	for (state p = 0; p < N; p++) {
		for (letter a = 0; a < A; a++) {
			for(state q : getSuccessors(p,a)){
				out[p].insert(q);
				in[q].insert(p);
			}
		}
	}
}

DetParityAut::DetParityAut(const state numberOfStates, const letter sizeOfAlphabet) {
		initialize(numberOfStates, sizeOfAlphabet);
		trans = std::vector<std::vector<state>>(N);
		for (state i = 0; i < N; i++) {
			trans[i] = std::vector<state>(A,-1);
		}
}

void DetParityAut::complement() {
	for (state i = 0; i < N; i++) {
		par[i]++;
	}
	sinkIdentified = false;
	reject = N;
}

bool DetParityAut::isDet() const{
	return true;
}

void DetParityAut::calculateNeighbors(std::vector<std::set<state>>& out, std::vector<std::set<state>>& in) const{
	out.resize(N);
	in.resize(N);

	for (state p = 0; p < N; p++) {
		for (letter a = 0; a < A; a++) {
			state q = trans[p][a];
			if(q>=N) continue;		//transition is not set
			out[p].insert(q);
			in[q].insert(p);
		}
	}
}

std::pair<state,std::vector<state>> DetParityAut::stronglyConnectedComponents() const{
	//compute cc using Kosarajus Algorithm: https://en.wikipedia.org/wiki/Kosaraju%27s_algorithm

	// V -> out/in-neighbors
	std::vector<std::set<state>> out;
	std::vector<std::set<state>> in;

	calculateNeighbors(out,in);

	return computeSCCs(N, out, in);
}

void DetParityAut::identifyAndMergeRejectingSinks()
{

	if (sinkIdentified) {
		return;
	}

	//compute cc using Kosarajus Algorithm: https://en.wikipedia.org/wiki/Kosaraju%27s_algorithm
	std::vector<std::set<state>> out;
	std::vector<std::set<state>> in;
	calculateNeighbors(out,in);
	std::pair<state,std::vector<state>> sccres = computeSCCs(N,out,in);
	// V -> SCC
	std::vector<state> scc = sccres.second;
	//number of strongly connected components
	state Nscc = sccres.first;
	// SCC -> rejecting?
	std::vector<bool> rscc(Nscc,true);
	//number of rejecting sink states
	state rc = 0;
	std::vector<std::set<state>> inscc(Nscc);
	for(state p = 0; p < N; p++){
		state pscc = scc[p];
		if((par[p]%2)==0) rscc[pscc] = false;
		for(state q : in[p]){
			inscc[pscc].insert(scc[q]);
		}
	}

	bool changed = true;
	while(changed){
		changed = false;
		for(state pscc = 0; pscc < Nscc; pscc++){
			if(!rscc[pscc]){
				for(state qscc : inscc[pscc]){
					if(rscc[qscc]){
						rscc[qscc] = false;
						changed = true;
					}
				}
			}
		}
	}

	for(state p = 0; p < N; p++){
		if(rscc[scc[p]]) rc++;
	}

	//std::cout << "Rejecting sink states: " << rc << std::endl;

	if (rc == 0) {
		//no rejecting sink states at all
		reject = N;
		sinkIdentified = true;
		return;
	}
	if (rscc[scc[init]]) {
		//initial state is rejecting -> aut is equivalent to trivial rejecting aut
		init = 0;
		reject = 0;
		sinkIdentified = true;
		par.clear();
		par.push_back(1);
		trans.clear();
		trans.push_back(std::vector<state>(A, 0));
		return;
	}
	if (rc == 1) {
		//only one rejecting sink state -> no merge needed
		for (state i = 0; i < N; i++) {
			if (rscc[scc[i]]) {
				reject = i;
				sinkIdentified = true;
				return;
			}
		}
	}

	//merge all states i with rscc[scc[i]]
	// V -> V'
	std::vector<state> nv(N);
	// V'\{reject} -> V;
	std::vector<state> ov(N - rc);

	//current index
	state cv = 0;
	//fill nv/ov
	//@PAR
	for (state i = 0; i < N; i++) {
		if (rscc[scc[i]]) {
			nv[i] = N - rc;
		}
		else {
			nv[i] = cv;
			ov[cv] = i;
			cv++;
		}
	}

	//merge
	state nN = N - rc + 1;
	std::vector<std::vector<state>> ntrans(nN);
	std::vector<parity> npar(nN);
	//@PAR
	for (state i = 0; i < nN-1; i++) {
		npar[i] = par[ov[i]];
		for (letter a = 0; a < A; a++) {
			ntrans[i] = std::vector<state>(A);
			ntrans[i][a] = nv[trans[ov[i]][a]];
		}
	}
	reject = nN - 1;
	npar[reject] = 1;
	sinkIdentified = true;
	init = nv[init];
	trans = ntrans;
	par = npar;
	N = nN;
}

bool DetParityAut::isInitial(state p) const
{
	return p==init;
}

state DetParityAut::addState(){
	state res = N;
	N++;
	par.resize(N);
	trans.resize(N);
	trans[res].resize(A,-1);
	return res;
}

std::string DetParityAut::to_string() const{
	std::string res = "Deterministic Parity Automaton:\n";
	res += "" + std::to_string(N) + " states and " + std::to_string(A) + " letters.";
	res += "\nInitial state: " + std::to_string(init);
	res += "\nParities:";
	for(state p = 0; p < N; p++){
		res += "\n" + std::to_string(par[p]);
	}
	for(letter a = 0; a < A; a++){
		res += "\nTransitions over letter "+std::to_string(a)+":";
		for(state p = 0; p < N; p++){
			res += "\n"+std::to_string(trans[p][a]);
		}
	}
	return res;
}

std::string DetParityAut::to_dot(bool labelTransitions) const{
	std::string res = "digraph \"\" {\n  rankdir=LR\n  label=\"DetParityAut\"\n  labelloc=\"t\"\n  node [shape=\"circle\"]\n";
	//initial state
	res += "  I [label=\"\", style=invis, width=0]\n";
	res += "  I -> "+std::to_string(init)+"\n";
	//all states
	for(state p = 0; p < N; p++){
		res += "  "+std::to_string(p)+" [label=\""+std::to_string(p)+"["+std::to_string(par[p])+"]\"]\n";
		for(state q = 0; q < N; q++){
			std::string pq = "  "+std::to_string(p) + " -> "+std::to_string(q);
			if(labelTransitions){
				pq += " [label=\"";
			}
			bool empty = true;
			for(letter a = 0; a < A; a++){
				if(trans[p][a]==q){
					if(!labelTransitions){
						empty = false;
						break;
					}
					if(!empty){
						pq += ", ";
					}
					empty = false;
					pq += std::to_string(a);
				}
			}
			if(!empty){
				if(labelTransitions){
					pq += "\"]";
				}
				res += pq+"\n";
			}
		}
	}
	res += "}";
	return res;
}

std::string DetParityAut::to_hoa(const bool buchi) const{
	std::string res = hoa_preamble(buchi);
	for(state p = 0; p < N; p++){
		res+="\nState: "+std::to_string(p) + "{"+(buchi ? ((par[p]%2)==0 ? "1" : "0") : std::to_string(par[p]))+"}";
		for(letter a = 0; a < A; a++){
			std::string label = makeLabel(a,A);
			res += "\n["+label+"] "+std::to_string(trans[p][a]);
		}
	}
	res +="\n--END--";
	return res;
}

/*
void parsehoapropositions(const std::string in, DetParityAut& dpa, const std::vector<letter>& alias, const unsigned char P, const state from, const state to){
	if(in=="f"){
		return;
	}
	if(in=="t"){
		for(letter a = 0; a < dpa.A; a++){
			state old = dpa.trans[from][a];
			if(old!=-1&&old!=to){
				throw std::runtime_error("Transition is not deterministic.");
			}
			dpa.trans[from][a] = to;
		}
		return;
	}
	
	std::vector<bool> set(P,false);
	std::vector<bool> val(P);

	std::vector<std::string> strprop = split(in,'&',true);
	for(std::string strp : strprop){
		unsigned char p;
		if(startsWith(strp,"!")){
			p = std::stoi(strp.substr(1));
			val[alias[p]] = false;
		}
		else{
			p = std::stoi(strp);
			val[alias[p]] = true;
		}
		set[alias[p]] = true;
	}

	letter base = 0;
	std::vector<letter> add;
	std::vector<letter> check;
	for(unsigned char p = 0; p < P; p++){
		letter v = 1;
		v = v<<p;
		if(!set[p]){
			add.push_back(v);
			check.push_back(((letter) 1)<<check.size());
		}
		else{
			if(val[p]) base += v;
		}
	}
	letter maxvar = 1;
	maxvar = maxvar << check.size();
	for(letter a = 0; a < maxvar; a++){
		letter a_ = base;
		for(unsigned char p = 0; p < check.size(); p++){
			if(a&check[p]) a_ += add[p];
		}
		if(a_>=dpa.A) break;
		state old = dpa.trans[from][a_];
		if(old!=-1&&old!=to){
			throw std::runtime_error("Transition is not deterministic.");
		}
		dpa.trans[from][a_] = to;
	}
}

void parsehoatransition(const std::string in, DetParityAut& dpa, const std::vector<letter>& alias, const unsigned char P, const state from){
	try{
		StringParser tr(in);
		tr.advanceToChar('[');
		if(tr.lastOp!=VALID) throw std::runtime_error("");
		std::string strtrans = tr.advanceToChar(']');
		if(tr.lastOp!=VALID) throw std::runtime_error("");
		std::string strto = in.substr(tr.next);
		state to = std::stoi(trim(strto));
		std::vector<std::string> strt = split(strtrans,'|',true);
		for(std::string strprop : strt){
			parsehoapropositions(strprop,dpa,alias,P,from,to);
		}
	}
	catch(const std::exception& e){
		std::string msg = e.what();
		throw std::runtime_error("Could not parse hoa transition \""+in+"\""+(msg.empty()? "." : ": "+msg));
	}
}

DetParityAut sink(bool accepting, letter alphabetsize){
	DetParityAut sink(1,alphabetsize);
	sink.init = 0;
	sink.par[0] = (accepting ? 2 : 1);
	for(letter a = 0; a < alphabetsize; a++) sink.trans[0][a] = 0;
	sink.sinkIdentified = true;
	sink.reject = (accepting ? 1 : 0);
	return sink;
}

bool hoaIsParityMaxEvenCondition(std::vector<std::string> hoa){
	if(hoa.size()<2) return false;
	parity sets = stoi(hoa[0]);
	if(sets == 0 && hoa[1]=="f") return true;	//empty language
	std::string cond = "";
	for(size_t i = 1; i < hoa.size(); i++) cond += hoa[i];
	StringParser parser(cond);
	while(sets>0){
		bool even;
		std::string infin = trim(parser.advanceToChar('('));
		if(parser.lastOp!=VALID) return false;
		if(infin=="Inf") even = true;
		else if(infin=="Fin") even = false;
		else return false;
		parity par = stoi(parser.advanceToChar(')'));
		if(parser.lastOp!=VALID) return false;
		if((even)!=(par%2==0)) return false;
		sets--;
		if(sets>1){
			std::string op = trim(parser.advanceToChar('('));
			if(parser.lastOp!=VALID) return false;
			if(even){
				if(op!="|") return false;
			}
			else{
				if(op!="&") return false;
			}
		}
		else if(sets>0){
			std::string op;
			if(even){
				op = trim(parser.advanceToChar('|'));
			}
			else{
				op = trim(parser.advanceToChar('&'));
			}
			if(op!=""||parser.lastOp!=VALID) return false;
		}
	}
	return true;
}

DetParityAut hoaToDetParityAut(std::string hoa, letter alphabetsize){
	StringParser sp(hoa);
	state N = 0;
	//letter A = 0;
	unsigned char P = 0;
	bool initfound = false;
	state init = 0;
	std::vector<letter> alias;
	//letter max = 0;
	try{
		//HEADER
		while(sp.remaining()>0){
			std::string line = sp.advanceLine();
			std::vector<std::string> words = split(line, ' ', true);
			if(words.empty()) continue;
			if(words[0]=="--BODY--"){
				break;
			}
			if(words[0]=="--END--"){
				throw std::runtime_error("");
			}
			if(words[0]=="States:"&&words.size()>1){
				N = std::stoi(words[1]);
			}
			else if(words[0]=="AP:"&&words.size()>1){
				P = stoi(words[1]);
				std::vector<std::string> props = split(line,'"',true);
				if(props.size()!=P+1) throw std::runtime_error("");
				alias.resize(P);
				for(letter p = 0; p < P; p++){
					alias[p] = stoi(props[p+1]);
				}
			}
			else if(words[0]=="acc-name:"&&words.size()>1){
				if(words[1]=="none") return sink(false,alphabetsize);
			}
			else if(words[0]=="Acceptance:"){
				words.erase(words.begin());
				if(!hoaIsParityMaxEvenCondition(words)) throw std::runtime_error("Automaton does not have parity acceptance.");
				if(words.size()==2&&words[0]=="0"&&words[1]=="f") return sink(false,alphabetsize);
				if(N==1&&P==0){
					if(!words.empty()&&words[0]=="2") return sink(false,alphabetsize);
					else return sink(true,alphabetsize);
				}
			}
			else if(words[0]=="Start:"&&words.size()>1){
				if(words.size()>2||initfound){
					throw std::runtime_error("Automaton has multiple initial states.");
				}
				init = std::stoi(words[1]);
				initfound = true;
			}
		}
		//BODY
		if(N==0){
			throw std::runtime_error("Automaton size is unknown.");
		}
		if((((letter) 1<<P)>=2*alphabetsize)||alias.empty()){
			throw std::runtime_error("Alphabet of automaton could not be read.");
		}
		unsigned char AP = 0;
		for(letter a = alphabetsize-1; a>0; a = a>>1){
			AP++;
		}
		if(!initfound){
			throw std::runtime_error("Did not read definition of initial state.");
		}
		DetParityAut res(N,alphabetsize);
		for(state p = 0; p < res.N; p++){
			for(letter a = 0; a < res.A; a++){
				res.trans[p][a] = -1;
			}
		}
		res.init = init;
		state c;
		bool statemode = false;
		while(sp.remaining()>0){
			std::string line = trim(sp.advanceLine());
			std::vector<std::string> words = split(line, ' ', true);
			if(words.empty()) continue;
			if(words[0]=="--END--") break;
			if(words[0]=="State:"){
				statemode = false;
				for(int i = 1; i < words.size(); i++){
					if(isdigit(words[i][0])){
						c = std::stoi(words[i]);
						statemode = true;
					}
					else if(words[i][0]=='{'&&words[i][words[i].length()-1]=='}'&&statemode){
						res.par[c]=std::stoi(extract(words[i]));
					}
				}
				if(!statemode){
					throw std::runtime_error("Could not parse state.");
				}
			}
			else{
				if(statemode){
					parsehoatransition(line,res,alias,AP,c);
				}
				else{
					throw std::runtime_error("");
				}
			}
		}
		//explicitly add sink
		state sink = -1;
		state count = 0;
		for(state p = 0; p < res.N; p++){
			for(letter a = 0; a < res.A; a++){
				if(res.trans[p][a]>=res.N){
					if(count==0){
						sink = res.addState();
						res.par[sink] = 1;
					}
					count++;
					res.trans[p][a] = sink;
					//std::cout << "Trans was not set: "<<p<<" ---- "<<a<<" ---> "<<res.trans[p][a]<<std::endl;
				}
			}
		}
		//std::cout << "Transitions changed: "<<count<<", sink state: "<<sink<<std::endl;
		return res;
	}
	catch(const std::exception& e){
		std::string msg = e.what();
		throw std::invalid_argument("Could not parse automaton from hoa"+(msg.empty() ? "." : (": "+msg)));
	}
}*/

std::pair<state,std::vector<state>> computeSCCs(const state N, const std::vector<std::set<state>>& outgoing, const std::vector<std::set<state>>& incoming){
	//compute cc using Kosarajus Algorithm: https://en.wikipedia.org/wiki/Kosaraju%27s_algorithm
	auto out = outgoing;
	out.resize(N);
	auto in = incoming;
	in.resize(N);

	std::vector<bool> visited(N,false);
	std::stack<state> L;

	std::stack<std::pair<state,std::vector<state>>> Q;

	state p = 0;
	//fill L
	while(!Q.empty()||p<N){
		if(Q.empty()){
			//iterate over all states
			//visit state
			if(!visited[p]){
				visited[p] = true;
				if(out[p].empty()){
					//no recursion, just push this state on L
					L.push(p);
				}
				else{
					//recursion by putting items on stack
					for(auto it = out[p].rbegin(); it!=out[p].rend(); it++){
						//visit all out neighbors by putting them on Q
						state q = *it;
						if(it!=out[p].rbegin()){
							Q.push(std::pair<state,std::vector<state>>(q,{}));
						}
						else{
							//after the last visit of out neighbors returns, push this state on L
							Q.push(std::pair<state,std::vector<state>>(q,{p}));
						}
					}
				}
			}
			p++;
		}
		else{
			//recursion
			auto pp = Q.top();
			Q.pop();
			//visit p_
			state p_ = pp.first;
			if(visited[p_]){
				//no further recursion
				//push states on L that are waiting for this recursive call to end
				if(pp.second.empty()) continue;
				for(size_t i = pp.second.size()-1; ; i--){
					L.push(pp.second[i]);
					if(i==0) break;
				}
			}
			else{
				visited[p_] = true;
				if(out[p_].empty()){
					//no further recursion, just push this state on L
					L.push(p_);
				}
				else{
					//deeper recursion
					for(auto it = out[p_].rbegin(); it!=out[p_].rend(); it++){
						//visit all out neighbors by putting them on Q
						state q = *it;
						if(it!=out[p_].rbegin()){
							Q.push(std::pair<state,std::vector<state>>(q,{}));
						}
						else{
							//after the last visit of out neighbors returns, push this state on L
							pp.second.push_back(p_);
							Q.push(std::pair<state,std::vector<state>>(q,pp.second));
						}
					}
				}
			}
		}
	}
	//assign SCCs
	// V -> SCC
	std::vector<state> scc(N, N);
	auto stack = std::stack<state>();
	state cscc = 0;
	while(!L.empty()) {
		state i = L.top();
		L.pop();
		if(scc[i]==N){
			stack.push(i);
			scc[i] = cscc;
			while (!stack.empty()) {
				state top = stack.top();
				stack.pop();
				for (state j : in[top]) {
					if (scc[j] == N) {
						scc[j] = cscc;
						stack.push(j);
					}
				}
			}
			cscc++;
		}
	}
	return std::pair<state,std::vector<state>>(cscc,scc);
}

ParityGame::ParityGame(const state numberOfStatesP0, const state numberOfStatesP1) {
		N0 = numberOfStatesP0;
		N1 = numberOfStatesP1;
		N = N0 + N1;
		par = std::vector<parity>(N, 0);
		edges = std::vector<std::set<state>>(N);
}

std::string ParityGame::to_string() const{
	return "";
}

void ParityGame::print() const{
	std::cout << to_string() << std::endl;
}

std::string ParityGame::to_dot(std::vector<std::string> statenames) const{
	std::string res = "digraph \"\" {\n  rankdir=LR\n  label=\"ParityGame\"\n  labelloc=\"t\"\n  node []\n";
	for(state p = 0; p < N; p++){
		res += "  "+std::to_string(p)+" [label=\""+(p<statenames.size() ? statenames[p] : std::to_string(p))+"["+std::to_string(par[p])+"]\", shape=\"";
		if(p<N0){
			res += "circle";
		}
		else{
			res += "square";
		}
		res += "\"]\n";
		for(state q : getSuccessors(p)){
			res += "  "+std::to_string(p)+" -> "+std::to_string(q)+"\n";
		}
	}
	res += "}";
	return res;
}

std::string ParityGame::to_pgsolver() const{
	std::string res = "parity "+std::to_string(N-1)+";";
	for(state p = 0; p < N; p++){
		res += "\n"+std::to_string(p)+" "+std::to_string(par[p])+" "+(p<N0 ? "0" : "1")+" ";
		//assumes every state has atleast one successor
		bool found = false;
		for(state q : getSuccessors(p)){
			if(found){
				res+=",";
			}
			found = true;
			res += std::to_string(q);
		}
		if(!found){
			//is this valid? specification says it is not
			throw std::invalid_argument("Every state must have at least one successor.");
		}
		res += ";";
	}
	return res;
}

void ParityGame::addSinks(){
	//check if sink for P0 is needed
	bool addsink0 = false;
	bool addsink1 = false;
	std::vector<bool> transToSink(N,false);
	for(state p = 0; p < N; p++){
		if(getSuccessors(p).empty()){
			if(p<N0) addsink0 = true;
			else addsink1 = true;
			transToSink[p] = true;
		}
	}
	state sink;
	if(addsink0){
		//add sink for P0
		sink = addStateP1(1);
		for(state p = 0; p < N0; p++){
			if(transToSink[p]) addEdge(p,sink);
		}
		addEdge(sink,sink);
	}
	if(addsink1){
		//add sink for P1
		sink = addStateP1(0);
		for(state p = N0; p < N; p++){
			if(transToSink[p]) addEdge(p,sink);
		}
		addEdge(sink,sink);
	}
}

parity ParityGame::maxParity() const{
	parity max = 0;
	for(state p = 0; p < N; p++){
		max = std::max(max,par[p]);
	}
	return max;
}

double ParityGame::transMatrixFilled() const{
	double res = 0;

	for(state p = 0; p < N; p++){
		double res_ = getSuccessors(p).size();

		res += res_/N;
	}

	return res/N;
}