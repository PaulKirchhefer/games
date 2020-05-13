#include "output.h"




std::string nondetstate_to_string(const state p, const Alphabet alph, const bool alt){
	if(alt)
	{
		std::string res = "(";
		state base = (5+alph.A)*alph.N;
		state p_;
		if(p < base){
			//(q,E) = (5+A)*q
			//(q,E') = (q,E)+1
			//(q,S) = (q,E)+2
			//(q,S')= (q,E)+3
			//(q,q) = (q,E)+4
			//(q,a) = (q,E)+5+a
			p_ = p/(5+alph.A);
			res += to_string(p_)+",";
			auto mod = p_*(5+alph.A);
			mod = p - mod;
			switch(mod){
				case 0: return res+"E)";
				case 1: return res+"E')";
				case 2: return res+"S)";
				case 3: return res+"S')";
				case 4: return res+to_string(p_)+")";
				default: char a = 'a'+(mod-5); return res + a + ")";
			}
		}
		else{
			//(q,E,c) = (5+A)*N+((6+A)*N)*c+(6+A)*q
			//(q,E',c) = (q,E,c)+1
			//(q,S,c) = (q,E,c)+2
			//(q,S',c)= (q,E,c)+3
			//(q,q,c) = (q,E,c)+4
			//(q,a,c) = (q,E,c)+5+a
			//(q,I,c) = (q,E,c)+5+A
			p_ = p - base;
			auto c = p_/((6+alph.A)*alph.N);
			auto mod = p_ - c*((6+alph.A)*alph.N);
			p_ = mod/(6+alph.A);
			res += to_string(p_)+",";
			mod = mod - (6+alph.A)*p_;
			switch(mod){
				case 0: return res+"E,c"+to_string(c)+")";
				case 1: return res+"E',c"+to_string(c)+")";
				case 2: return res+"S,c"+to_string(c)+")";
				case 3: return res+"S',c"+to_string(c)+")";
				case 4: return res+to_string(p_)+",c"+to_string(c)+")";
				default: if(mod==(5+alph.A)){return res+"I,c"+to_string(c)+")";} char a = 'a'+(mod-5); return res + a + ",c"+to_string(c)+")";
			}
		}
	}
	else{
		//(q,E) = 2*q
		//(q,S) = 2*q + 1
		std::string res = "(";
		state base = 2*alph.N;
		state p_;
		if(p<base){
			p_ = p / 2;
			res += to_string(p_)+",";
			if((p%2)==0){
				return res + "E)";
			}
			else{
				return res + "S)";
			}
		}
		else{
			//(q,E,c) = 2*N + c*3*N + 3*q
			//(q,S,c) = 2*N + c*3*N + 3*q + 1
			//(q,I,c) = 2*N + c*3*N + 3*q + 2
			p_ = p-base;
			auto c = p_/(3*alph.N);
			auto mod = p_ - c*3*alph.N;
			p_ = mod/3;
			res += to_string(p_)+",";
			mod = mod - 3*p_;
			switch(mod){
				case 0: return res + "E,c"+to_string(c)+")";
				case 1: return res + "S,c"+to_string(c)+")";
				default: return res + "I,c"+to_string(c)+")";
			}
		}
	}
}

std::string nondetpart_to_dot(const ParityAut nondet, const Alphabet alph, const bool alt, const bool labelTransitions, std::vector<state>* oldindices){
	const bool OLD = (oldindices!=NULL);
	
	std::string res = "digraph \"\" {\n  rankdir=LR\n  label=\"Non-Det. Part";
    if(alt) res+= "[alt.]";
    res +="\"\n  labelloc=\"t\"\n  node [shape=\"circle\"]\n";
	//initial states
	for(state p : nondet.getInitial()){
		res += "  I"+std::to_string(p)+" [label=\"\", style=invis, width=0]\n";
		res += "  I"+std::to_string(p)+" -> "+std::to_string(p)+"\n";
	}
	
	//all states
    res += "subgraph cluster_0 {\n  style=filled\n  color=white\n  label=\"\"\n";
    int c = 1;
    state base;
    if(alt){
        base = (5+alph.A)*alph.N;
    }
    else{
        base = 2*alph.N;
    }
    for(state p = 0; p < nondet.N; p++){
        if(p==base){
            if(alt){
                base += (6+alph.A) * alph.N;
            }
            else{
                base += 3 * alph.N;
            }
            res += "}\nsubgraph cluster_"+to_string(c)+" {\n  style=filled\n  color=white\n  label=\"\"\n";
            c++;
        }
        res += "  "+std::to_string(p)+" [label=\""+(OLD ? nondetstate_to_string((*oldindices)[p],alph,alt) : nondetstate_to_string(p,alph,alt))+"\"";
        if((nondet.par[p]%2)==0) res += ", shape=doublecircle";
        res += "]\n";
    }
    res += "}\n";
	for(state p = 0; p < nondet.N; p++){
		for(state q = 0; q < nondet.N; q++){
			std::string pq = "  "+std::to_string(p) + " -> "+std::to_string(q);
			if(labelTransitions){
				pq += " [label=\"";
			}
			bool empty = true;
			if(alt){
				std::string apart = "";
				std::string qpart = "";
				bool notA = false;
				bool notQ = false;
				for(letter a = 0; a < alph.A; a++){
					if(nondet.isTrans(p,a,q)){
						if(!labelTransitions){
							empty = false;
							break;
						}
						if(!empty) apart += ",";
						empty = false;
						char aa = 'a'+a;
						apart += aa;
					}
					else{
						notA = true;
					}
				}
				bool empty_ = empty;
				for(letter a = alph.A; a < nondet.A; a++){
					if(nondet.isTrans(p,a,q)){
						if(!labelTransitions){
							empty = false;
							break;
						}
						if(!empty) qpart += ",";
						empty = false;
						qpart += std::to_string(a-alph.A);
					}
					else{
						notQ = true;
					}
				}
				if(notA){
					pq +=apart;
				}
				else{
					pq +="A";
				}
				if(notQ){
					pq += qpart;
				}
				else{
					if(!empty_) pq += ",";
					pq += "Q";
				}
			}
			else{
				std::string apart = "";
				std::string tpart = "";
				bool notA = false;
				bool notT = false;
				for(letter a = 0; a < alph.A; a++){
					if(nondet.isTrans(p,a,q)){
						if(!labelTransitions){
							empty = false;
							break;
						}
						if(!empty) apart += ",";
						empty = false;
						char aa = 'a'+a;
						apart += aa;
					}
					else{
						notA = true;
					}
				}
				bool empty_ = empty;
				for(letter t = alph.A; t < nondet.A; t++){
					if(nondet.isTrans(p,t,q)){
						if(!labelTransitions){
							empty = false;
							break;
						}
						if(!empty) tpart += ",";
						empty = false;
						tpart += alph.trans_to_string(t-alph.A);
					}
					else{
						notT = true;
					}
				}
				if(notA){
					pq +=apart;
				}
				else{
					pq +="A";
				}
				if(notT){
					pq += tpart;
				}
				else{
					if(!empty_) pq += ",";
					pq += "T";
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

std::vector<letter> allLettersTSETS(const Alphabet* alph){
	std::vector<letter> res(alph->A);
	for(letter a = 0; a < alph->A; a++){
		tset ts = alph->nextLetterSet(a);
		if(ts==0) continue;
		ts = ts-1;
		res[a] = ts;
	}
	return res;
}

void parsehoapropositions(const std::string in, DetParityAut& dpa, const std::vector<letter>& alias, const unsigned char P, const state from, const state to, const Alphabet* alph){
	if(in=="f"){
		return;
	}
	if(in=="t"){
		if(alph!=NULL){
			auto all = allLettersTSETS(alph);
			for(letter a = 0; a < alph->A; a++){
				auto& mm = dpa.hoatrans[from][a];
				mm[to] = {all[a]};
			}
			return;
		}

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

	if(alph!=NULL){
		//used to check for valid letters
		letter amin = 0;
		letter anot = 0;
		for(letter a = alph->tsbits; a<alph->bits; a++){
			if(set[a]){
				letter pow = (1L << (a-alph->tsbits));
				if(val[a]){
					amin += pow;
				}
				else{
					anot += pow;
				}
			}
		}

		for(letter a = 0; a < alph->A; a++){
			//set[i]&&!val[i] -> a[i] == 0
			//set[i]&&val[i] -> a[i] == 1
			if(((amin&a)==amin)&&((anot&&a)==0)){
				bool valid = true;
				//invalid if a bit is set to 1 between letterT[a] and tsbits, corresponding to a set containing a not existing transition
				for(size_t i = alph->letterT[a]; i < alph->tsbits; i++){
					if(set[i]&&val[i]){
						valid = false;
						break;
					}
				}
				if(!valid) continue;
				letter ts = 0;
				letter pow = 1;
				for(size_t i = 0; i < alph->letterT[a]; i++){
					//get maximal possible transition set
					//		--->value not set -> include corresponding transition
					if(!set[i]||val[i]) ts += pow;
					pow = pow << 1;
				}
				//ts now: represents the ts'th transition set _over_ a
				//--> adjust so that it represents the ts'th transition set
				ts += alph->letterSetIndices[a];
				auto& mm = dpa.hoatrans[from][a];
				if(mm.count(to)>0){
					mm[to].insert(ts);
				}
				else{
					mm[to] = {ts};
				}
			}
		}

		return;
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

void parsehoatransition(const std::string in, DetParityAut& dpa, const std::vector<letter>& alias, const unsigned char P, const state from, const Alphabet* alph){
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
			parsehoapropositions(strprop,dpa,alias,P,from,to,alph);
		}
	}
	catch(const std::exception& e){
		std::string msg = e.what();
		throw std::runtime_error("Could not parse hoa transition \""+in+"\""+(msg.empty()? "." : ": "+msg));
	}
}

DetParityAut sink(bool accepting, letter alphabetsize, const Alphabet* alph){
	DetParityAut sink(1,alphabetsize);
	sink.init = 0;
	sink.par[0] = (accepting ? 2 : 1);
	if(alph==NULL){
		for(letter a = 0; a < alphabetsize; a++) sink.trans[0][a] = 0;
	}
	else{
		sink.trans.clear();
		sink.hoatrans = std::vector<std::vector<std::map<state,std::set<letter>>>>(1,std::vector<std::map<state,std::set<letter>>>(alph->A));
		auto all = allLettersTSETS(alph);
		for(letter a = 0; a < alph->A; a++){
			sink.hoatrans[0][a][0] = {all[a]};
		}
	}
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

DetParityAut hoaToDetParityAut(std::string hoa, letter alphabetsize, const Alphabet* alph){
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
				if(words[1]=="none") return sink(false,alphabetsize,alph);
			}
			else if(words[0]=="Acceptance:"){
				words.erase(words.begin());
				if(!hoaIsParityMaxEvenCondition(words)) throw std::runtime_error("Automaton does not have parity acceptance.");
				if(words.size()==2&&words[0]=="0"&&words[1]=="f") return sink(false,alphabetsize,alph);
				if(N==1&&P==0){
					if(!words.empty()&&words[0]=="2") return sink(false,alphabetsize,alph);
					else return sink(true,alphabetsize,alph);
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
		DetParityAut res = (alph==NULL ? DetParityAut(N,alphabetsize) : DetParityAut(N,1));
		if(alph==NULL){
			for(state p = 0; p < res.N; p++){
				for(letter a = 0; a < res.A; a++){
					res.trans[p][a] = -1;
				}
			}
		}
		else{
			res.trans.clear();
			res.hoatrans = std::vector<std::vector<std::map<state,std::set<letter>>>>(res.N,std::vector<std::map<state,std::set<letter>>>(alph->A));
			res.A = alph->TS;
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
					parsehoatransition(line,res,alias,AP,c,alph);
				}
				else{
					throw std::runtime_error("");
				}
			}
		}
		if(alph==NULL){
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
		}
		//std::cout << "Transitions changed: "<<count<<", sink state: "<<sink<<std::endl;
		return res;
	}
	catch(const std::exception& e){
		std::string msg = e.what();
		throw std::invalid_argument("Could not parse automaton from hoa"+(msg.empty() ? "." : (": "+msg)));
	}
}