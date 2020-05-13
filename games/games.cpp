#include "games.h"

//reduces finitepower to limitedness
//uses STAMINA CODE!
MultiCounterAut* finitePowerCaut(const ClassicEpsAut& in_){
	//assume in has only 1 initial state
	//assume in.trans and in.trans_eps are valid
	//ignores in.transdet
	ClassicEpsAut in(in_);
	char A = in.NbLetters;
	uint N = in.NbStates;
	MultiCounterAut* res = new MultiCounterAut(A,N,1);

	//initial state, copy final states
	bool found = false;
	for(uint p = 0; p < N; p++){
		res->finalstate[p] = in.finalstate[p];
		if(in.initialstate[p]){
			if(found){
				throw invalid_argument("Only automata with one initial state are supported.");
			}
			else{
				found = true;
				res->initialstate[p] = true;
			}
		}
	}
	if(!found){
		throw invalid_argument("Invalid automaton, no initial state.");
	}

	//copy epsilon transitions, add eps transitions to self and add eps transitions for finitepower
	ExplicitMatrix reseps(N);
	for(uint p = 0; p < N; p++){
		for(uint q = 0; q < N; q++){
			if(in.trans_eps[p][q]||p==q){
				//copy / self loop
				reseps.coefficients[p][q] = MultiCounterMatrix::epsilon();
			}
			else{
				if(res->finalstate[p]&&res->initialstate[q]){
					//finitepower transition
					reseps.coefficients[p][q] = MultiCounterMatrix::inc(0);
				}
				else{
					//no eps transition
					reseps.coefficients[p][q] = MultiCounterMatrix::bottom();
				}
			}
		}
	}

	/**********************************************/
	// COPIED (and adapted) FROM STAMINAs epsRemoval()!
	//stabilize epsilon-transitions
    const MultiCounterMatrix * prev_eps = new MultiCounterMatrix(reseps);
    const MultiCounterMatrix * new_eps = MultiCounterMatrix(reseps)*MultiCounterMatrix(reseps);
    while (! (*new_eps == *prev_eps) )
    {
        delete prev_eps; prev_eps = NULL;
        prev_eps = new_eps;
        new_eps =  (*new_eps) * (*new_eps);
    }
    delete prev_eps; prev_eps = NULL;
    
    //update matrices of each letter : new_a=e*ae*
    for(char a=0;a<A;a++){
		//copy transition matrix of a
		ExplicitMatrix atrans(N);
		for(uint p = 0; p < N; p++){
			for(uint q = 0; q < N; q++){
				atrans.coefficients[p][q] = (in.trans[a][p][q] ? MultiCounterMatrix::epsilon() : MultiCounterMatrix::bottom());
			}
		}
		//eps * atrans * eps
        auto ae = MultiCounterMatrix(atrans) * (*new_eps);
        res->set_trans(a, (*new_eps) * (*ae));
        //cleanup
        delete ae; ae = NULL;
    }
    delete new_eps; new_eps = NULL;
	/**********************************************/

	return res;
}

//uses sequences of transitions, but transitions are a sequence of from, over, to
ParityAut makeNonDetPartAlt(const MultiCounterAut& costAut, Options* opt, std::vector<state>* oldindices = NULL){

	const char A = costAut.NbLetters;
	const uint N = costAut.NbStates;
	const char C = costAut.NbCounters;

	const letter Ares = N + A;
	const state Nres = N*(5+A+6*C+A*C);		//no explicit sink state

	//encoding of states of res:
	//(q,E) = (5+A)*q
	//(q,E') = (q,E)+1
	//(q,S) = (q,E)+2
	//(q,S')= (q,E)+3
	//(q,q) = (q,E)+4
	//(q,a) = (q,E)+5+a

	//(q,E,c) = (5+A)*N+((6+A)*N)*c+(6+A)*q
	//(q,E',c) = (q,E,c)+1
	//(q,S,c) = (q,E,c)+2
	//(q,S',c)= (q,E,c)+3
	//(q,q,c) = (q,E,c)+4
	//(q,a,c) = (q,E,c)+5+a
	//(q,I,c) = (q,E,c)+5+A

	ParityAut res(Nres,Ares);

	//initial state(s)
	bool found = false;
	for(uint p = 0; p < N; p++){
		if(costAut.initialstate[p]){
			if(found){
				throw invalid_argument("Input cost automaton must have a single initial state.");
			}
			else{
				found = true;
				state initialres = (5+A)*p;		// = (q_0, E)
				res.addInitial(initialres);
			}
		}
	}
	if(!found) throw invalid_argument("Input cost automaton must have an initial state.");

	//final states
	for(state p = 0; p < Nres; p++){
		res.par[p] = 1;
	}
	for(uint p = 0; p < N; p++){
		for(char c = 0; c < C; c++){
			state pres = (5+A)*N+((6+A)*N)*c+(6+A)*p+5+A;	// = (p,I,c)
			res.par[pres] = 2;
		}
	}

	//transitions
	state pE,pE_,pS,pS_,pp,pa;
	letter ares;
	state qE;
	for(uint p = 0; p < N; p++){
		pE = (5+A)*p;
		pE_ = pE+1;
		pS = pE_+1;
		pS_= pS+1;
		pp = pS_+1;

		ares = A+p;

		res.addTrans(pS,ares,pp);

		for(uint q = 0; q < N; q++){
			ares = A+q;
			qE = (5+A)*q;

			res.addTrans(pE,ares,pE_);
			res.addTrans(pE_,ares,pE);
			res.addTrans(pS,ares,pS_);
			res.addTrans(pS_,ares,pS);

			for(char a = 0; a < A; a++){
				pa = pE+5+a;

				if(!MultiCounterMatrix::is_bottom(costAut.get_trans(a)->get(p,q))){
					res.addTrans(pa,ares,qE);
				}
			}
		}

		for(char a = 0; a < A; a++){
			ares = a;
			pa = pE+5+a;

			res.addTrans(pE_,ares,pE_);
			res.addTrans(pS_,ares,pS_);
			res.addTrans(pE,ares,pS);
			res.addTrans(pp,ares,pa);

			for(char c = 0; c < C; c++){
				state pSc = (5+A)*N+((6+A)*N)*c+(6+A)*p+2;

				res.addTrans(pE,ares,pSc);
			}
		}
	}

	state pI,qI;
	for(char c = 0; c < C; c++){
		for(uint p = 0; p < N; p++){
			pE = (5+A)*N+((6+A)*N)*c+(6+A)*p;
			pE_ = pE+1;
			pS = pE_+1;
			pS_= pS+1;
			pp = pS_+1;
			pI = pE+5+A;

			ares = A+p;

			res.addTrans(pS,ares,pp);

			for(uint q = 0; q < N; q++){
				ares = A+q;
				qE = (5+A)*N+((6+A)*N)*c+(6+A)*q;
				qI = qE+5+A;

				res.addTrans(pE,ares,pE_);
				res.addTrans(pE_,ares,pE);
				res.addTrans(pS,ares,pS_);
				res.addTrans(pS_,ares,pS);
				res.addTrans(pI,ares,pE_);

				for(char a = 0; a < A; a++){
					pa = pE+5+a;

					cact ct = costAut.get_trans(a)->get(p,q);

					if(!MultiCounterMatrix::is_bottom(ct)&&!isReset(ct,c)){
						if(isInc(ct,c)){
							res.addTrans(pa,ares,qI);
						}
						else{
							res.addTrans(pa,ares,qE);
						}
					}
				}
			}

			for(char a = 0; a < A; a++){
				ares = a;
				pa = pE+5+a;

				res.addTrans(pE_,ares,pE_);
				res.addTrans(pS_,ares,pS_);
				res.addTrans(pE,ares,pS);
				res.addTrans(pI,ares,pS);
				res.addTrans(pp,ares,pa);
			}
		}
	}

	res.prune(oldindices);

	//statistics
	if(opt->logdata()){
		double filled = res.transMatrixFilled();
		std::string log = opt->make_prefix()+" nondetALT: N="+std::to_string(res.N)+", "
		" A="+std::to_string(res.A)+", %="+std::to_string(100.0*filled)+"\n";
		opt->writeToLog(log);
	}
	if(opt->stats()){
		opt->add_data(NDET_N,res.N);
		opt->add_data(NDET_NMAX, Nres);
		opt->add_data(Data::A, Ares);
		opt->add_data(NDET_CALLS,1L);
	}

	return res;
}

//the other nondetpart automata accept -wc2 if the input word is well formed
//	- if not their behavior is undefined. this might make the automaton harder to determinize
//		TEST: transition to T if word is not of the form a t < t' < t'' ... a t ...
ParityAut makeNonDetPartSink(const MultiCounterAut& costAut, const Alphabet& alph, Options* opt, std::vector<state>* oldindices = NULL){

	parity sinkSPar = (opt->g%2) + 1;	//parity of sink for transitions that are not in increasing order
	bool checkWC1 = (opt->g >= 4);	//whether or not to check WC1
	parity sinkWC1Par = (opt->g==6||opt->g==7) ? 2 : 1;		//parity of sink for WC1 failure

	unsigned int cS = 0;		//#of times a transition has violated order
	unsigned int cWC1 = 0;		//#of times a transition has not violated order but WC1

	const char A = costAut.NbLetters;
	const uint N = costAut.NbStates;
	const char C = costAut.NbCounters;
	const trans T = alph.T;

	const letter Ares = T + A;
	const state Nres = N*(2*T+A+3*T*C+A*C)+1+(checkWC1&&(sinkSPar!=sinkWC1Par) ? 1 : 0);

	const state sinkS = Nres-1;
	const state sinkWC1 = (!checkWC1 || (sinkSPar==sinkWC1Par) ? sinkS : sinkS-1);

	ParityAut res(Nres,Ares);

	//encoding of states of res:
	//(q,E,t) = 2*q*T + 2*t
	//(q,S,t) = 2*q*T + 2*t + 1
	//(q,E,t,c) = 2*N*T + c*3*N*T + 3*q*T + 3*t
	//(q,S,t,c) = (q,E,t,c) + 1
	//(q,I,t,c) = (q,E,t,c) + 2
	//(q,S,a) = 2*N*T + C*3*N*T + q*A + a
	//(q,S,a,c) = 2*N*T + C*3*N*T + N*A + q*C*A + c*A + a

	//initial state(s)
	bool found = false;
	for(uint p = 0; p < N; p++){
		if(costAut.initialstate[p]){
			if(found){
				throw invalid_argument("Input cost automaton must have a single initial state.");
			}
			else{
				found = true;
				state initialres = 2*p*T + 2*(T-1);		// = (q_0, E, T-1)
				res.addInitial(initialres);
			}
		}
	}
	if(!found) throw invalid_argument("Input cost automaton must have an initial state.");

	//final states
	for(state p = 0; p < Nres; p++){
		res.par[p] = 1;
	}
	for(uint p = 0; p < N; p++){
		for(char c = 0; c < C; c++){
			for(trans t = 0; t < T; t++){
				state pIct = 2*N*T + c*3*N*T + 3*p*T + 3*t + 2;	//(p,I,c,t)
				res.par[pIct] = 2;
			}
		}
	}

	//sink(s)
	res.par[sinkWC1] = sinkWC1Par;
	res.par[sinkS] = sinkSPar;
	for(letter a = 0; a < Ares; a++){
		letter ares = a;

		res.addTrans(sinkS,ares,sinkS);
		res.addTrans(sinkWC1,ares,sinkWC1);
	}

	//transitions
	//guess counter part = Q_A
	for(state p = 0; p < N; p++){

		for(trans t = 0; t < T; t++){
			state pEt = 2*p*T + 2*t;
			state pSt = pEt + 1;

			letter tover = alph.over(t);

			for(trans r = 0; r < T; r++){
				letter ares = A + r;
				letter rover = alph.over(r);

				if(r <= t){		//transition sequence shall be increasing
					res.addTrans(pEt,ares,sinkS);
					res.addTrans(pSt,ares,sinkS);

					cS += 2;
				}
				else{
					if(checkWC1&&tover!=rover){	//WC1
						res.addTrans(pEt,ares,sinkWC1);
						res.addTrans(pSt,ares,sinkWC1);

						cWC1 += 2;
					}
					else{
						state pEr = 2*p*T + 2*r;
						state pSr = pEr + 1;

						res.addTrans(pEt,ares,pEr);
						res.addTrans(pSt,ares,pSr);

						if(alph.from(r,rover)==p){
							state toEr = 2*alph.to(r)*T + 2*r;

							res.addTrans(pSt,ares,toEr);
						}
					}
				}
			}

			for(char a = 0; a < A; a++){
				letter ares = a;
				state pSa = 2*N*T + C*3*N*T + p*A + a;

				res.addTrans(pEt,ares,pSa);

				for(char c = 0; c < C; c++){
					state pSca = 2*N*T + C*3*N*T + N*A + p*C*A + c*A + a;

					res.addTrans(pEt,ares,pSca);
				}
			}
		}

		for(char a = 0; a < A; a++){
			state pSa = 2*N*T + C*3*N*T + p*A + a;

			for(trans t = 0; t < T; t++){
				letter ares = A + t;
				state pSt = 2*p*T + 2*t + 1;
				letter tover = alph.over(t);

				if(checkWC1&&tover!=a){		//WC1
					res.addTrans(pSa,ares,sinkWC1);

					cWC1++;
				}
				else{
					res.addTrans(pSa,ares,pSt);

					if(alph.from(t)==p){
						state toEt = 2*alph.to(t)*T + 2*t;
						
						res.addTrans(pSa,ares,toEt);
					}
				}
			}
		}
	}

	//no reset part = Q_c
	for(char c = 0; c < C; c++){

		for(state p = 0; p < N; p++){

			for(trans t = 0; t < T; t++){

				state pEt = 2*N*T + c*3*N*T + 3*p*T + 3*t;
				state pSt = pEt + 1;
				state pIt = pEt + 2;

				letter tover = alph.over(t);

				for(trans r = 0; r < T; r++){
					letter ares = A + r;
					letter rover = alph.over(r);

					if(r <= t){		//transition sequence shall be increasing
						res.addTrans(pEt,ares,sinkS);
						res.addTrans(pSt,ares,sinkS);
						res.addTrans(pIt,ares,sinkS);

						cS += 3;
					}
					else{

						if(checkWC1&&tover!=rover){		//WC1
							res.addTrans(pEt,ares,sinkWC1);
							res.addTrans(pSt,ares,sinkWC1);
							res.addTrans(pIt,ares,sinkWC1);

							cWC1 += 3;
						}
						else{
							state pEr = 2*N*T + c*3*N*T + 3*p*T + 3*r;
							state pSr = pEr + 1;

							res.addTrans(pEt,ares,pEr);
							res.addTrans(pSt,ares,pSr);
							res.addTrans(pIt,ares,pEr);

							if(alph.from(r)==p&&!alph.isResetTrans(r,c)){
								if(alph.isIncTrans(r,c)){
									state toIr = 2*N*T + c*3*N*T + 3*alph.to(r)*T + 3*r + 2;

									res.addTrans(pSt,ares,toIr);
								}
								else{
									state toEr = 2*N*T + c*3*N*T + 3*alph.to(r)*T + 3*r;

									res.addTrans(pSt,ares,toEr);
								}
							}
						}
					}
				}

				for(char a = 0; a < A; a++){
					letter ares = a;
					state pSa = 2*N*T + C*3*N*T + N*A + p*C*A + c*A + a;

					res.addTrans(pEt,ares,pSa);
					res.addTrans(pIt,ares,pSa);
				}
			}

			for(char a = 0; a < A; a++){
				state pSa = 2*N*T + C*3*N*T + N*A + p*C*A + c*A + a;

				for(trans t = 0; t < T; t++){
					letter ares = A + t;
					state pSt = 2*N*T + c*3*N*T + 3*p*T + 3*t + 1;
					letter tover = alph.over(t);

					if(checkWC1&&tover!=a){		//WC1
						res.addTrans(pSa,ares,sinkWC1);

						cWC1++;
					}
					else{
						res.addTrans(pSa,ares,pSt);

						if(alph.from(t)==p&&!alph.isResetTrans(t,c)){
							if(alph.isIncTrans(t,c)){
								state toIt = 2*N*T + c*3*N*T + 3*alph.to(t)*T + 3*t + 2;

								res.addTrans(pSa,ares,toIt);
							}
							else{
								state toEt = 2*N*T + c*3*N*T + 3*alph.to(t)*T + 3*t;

								res.addTrans(pSa,ares,toEt);
							}
						}
					}
				}
			}
		}
	}

	res.prune(oldindices);

	//statistics
	if(opt->logdata()){
		double filled = res.transMatrixFilled();
		std::string log = opt->make_prefix()+" nondetSink"
		", "+(sinkSPar%2==0 ? "T" : "F")+" "+(checkWC1 ? (sinkWC1Par%2==0 ? "T" : "F") : "-")+""
		": N="+std::to_string(res.N)+", "
		" A="+std::to_string(res.A)+", %="+std::to_string(100.0*filled)+" "
		" #S="+std::to_string(cS) + (checkWC1 ? " #1="+std::to_string(cWC1) : "")+"\n";
		opt->writeToLog(log);
	}
	if(opt->stats()){
		opt->add_data(NDET_N, res.N);
		opt->add_data(NDET_NMAX, Nres);
		opt->add_data(Data::A, Ares);
		opt->add_data(NDET_CALLS,1L);
	}

	return res;
}

//uses sequences of transitions as input instead of transition sets over letters
//guesses a counter
//guesses a transition in each sequence
ParityAut makeNonDetPart(const MultiCounterAut& costAut, const Alphabet& alph, Options* opt, std::vector<state>* oldindices = NULL){

	const char A = costAut.NbLetters;
	const uint N = costAut.NbStates;
	const char C = costAut.NbCounters;

	const letter Ares = alph.T + A;
	const state Nres = N*(2 + 3*C);		//no explicit sink state

	ParityAut res(Nres,Ares);

	//encoding of states of res:
	//(q,E) = 2*q
	//(q,S) = 2*q + 1
	//(q,E,c) = 2*N + c*3*N + 3*q
	//(q,S,c) = 2*N + c*3*N + 3*q + 1
	//(q,I,c) = 2*N + c*3*N + 3*q + 2

	//initial state(s)
	bool found = false;
	for(uint p = 0; p < N; p++){
		if(costAut.initialstate[p]){
			if(found){
				throw invalid_argument("Input cost automaton must have a single initial state.");
			}
			else{
				found = true;
				state initialres = 2*p;		// = (q_0, E)
				res.addInitial(initialres);
			}
		}
	}
	if(!found) throw invalid_argument("Input cost automaton must have an initial state.");

	//final states
	for(state p = 0; p < Nres; p++){
		res.par[p] = 1;
	}
	for(uint p = 0; p < N; p++){
		for(char c = 0; c < C; c++){
			state pres = 2*N + 3*N*c + 3*p + 2;	// = (p,I,c)
			res.par[pres] = 2;
		}
	}

	//transitions
	//guess counter part = Q_A
	letter ares;
	state pE, pS, pSc, toE;
	for(uint p = 0; p < N; p++){
		state pE = 2*p;			// = (p, E)
		state pS = pE + 1;		// = (p, S)
		for(char a = 0; a < A; a++){
			ares = a;

			res.addTrans(pE,ares,pS);		// = ((p, E), a, (p, S))

			//guess counter
			for(char c = 0; c < C; c++){
				pSc = 2*N + c*3*N + 3*p + 1;

				res.addTrans(pE,ares,pSc);		// = ((p, E), a, (p, S, c))
			}
		}
		for(trans t = 0; t < alph.T; t++){
			ares = A + t;

			res.addTrans(pE,ares,pE);		// = ((p, E), t, (p, E))
			res.addTrans(pS,ares,pS);		// = ((p, S), t, (p, S))

			if(alph.from(t)==p){
				//guess run
				toE = 2*alph.to(t);	// = (t.to, E)

				res.addTrans(pS,ares,toE);		// = ((p, S), t, (t.to, E))
			}
		}
	}
	//no reset part = Q_c
	state pI, toI;
	for(char c = 0; c < C; c++){
		for(uint p = 0; p < N; p++){
			pE = 2*N + c*3*N + 3*p;			// = (p, E, c)
			pS = pE + 1;					// = (p, S, c)
			pI = pE + 2;					// = (p, I, c)
			for(char a = 0; a < A; a++){
				ares = a;

				res.addTrans(pE,ares,pS);		// = ((p, E, c), a, (p, S, c))
				res.addTrans(pI,ares,pS);		// = ((p, I, c), a, (p, S, c))
			}
			for(trans t = 0; t < alph.T; t++){
				ares = A + t;

				res.addTrans(pE,ares,pE);			// = ((p, E, c), t, (p, E, c))
				res.addTrans(pI,ares,pE);			// = ((p, I, c), t, (p, E, c))
				res.addTrans(pS,ares,pS);			// = ((p, S, c), t, (p, S, c))

				if(alph.from(t)==p){
					//guess run
					if(!alph.isResetTrans(t,c)){
						toE = 2*N + c*3*N + 3*alph.to(t);	// = (t.to, E, c)
						toI = toE + 2;						// = (t.to, I, c)

						if(alph.isIncTrans(t,c)){
							res.addTrans(pS,ares,toI);		// = ((p, S, c), t, (t.to, I, c))
						}
						else{
							res.addTrans(pS,ares,toE);		// = ((p, S, c), t, (t.to, E, c))
						}
					}
				}
			}
		}
	}

	//DEBUG
	if(opt->output()&&opt->canWrite()&&opt->debug()){
		std::string path = opt->make_path_prefix()+"_nondetfull.dot";
		try{
			writeToFile(path,nondetpart_to_dot(res,alph,opt->useAlt(),true));
		}
		catch(const std::exception& e){
			if(opt->verbose()||opt->debug()){
				std::string msg = e.what();
				std::cout << ">Failed to write dot file of non det. part of the winning condition: "+msg;
			}
		}
	}

	res.prune(oldindices);

	//statistics
	if(opt->logdata()){
		double filled = res.transMatrixFilled();
		std::string log = opt->make_prefix()+" nondet"
		": N="+std::to_string(res.N)+", "
		" A="+std::to_string(res.A)+", %="+std::to_string(100.0*filled)+"\n";
		opt->writeToLog(log);
	}
	if(opt->stats()){
		opt->add_data(NDET_N, res.N);
		opt->add_data(NDET_NMAX, Nres);
		opt->add_data(Data::A,Ares);
		opt->add_data(NDET_CALLS,1L);
	}

	return res;
}

//uses different bit representation of transition sets
//doesnt explicitly compute all transitions; instead computes hoa hint
//the returned NPA has INVALID NPA.trans, ie NPA.A does not match NPA.trans[.].size()!
ParityAut makeWC2TSC(const MultiCounterAut& costAut, const Alphabet& alph, Options* opt){
	state C = costAut.NbCounters;
	state N = costAut.NbStates;
	letter A = costAut.NbLetters;

	state N_ = N * (1 + 2 * C);
	letter A_ = alph.TS;

	//encoding of states:
	//(p) = p
	//(p,E,c) = N + 2*c*N + p
	//(p,I,c) = N + 2*c*N + N + p = 2*(c+1)*N + p
	
	//dont initialize transitions as they will not be filled anyway
	ParityAut res(N_, 1);

	std::vector<std::set<state>> out(N_,std::set<state>());
	std::vector<std::set<state>> in(N_,std::set<state>());

	std::vector<std::vector<std::set<trans>>> hint(N_,std::vector<std::set<trans>>(N_,std::set<trans>()));

	//initial states
	for (state p = 0; p < N; p++) {
		if (costAut.initialstate[p]) res.addInitial(p);
	}
	//parities
	for (state p = 0; p < N_; p++) {
		//'Q'
		if (p < N) {
			res.par[p] = 1;
		}
		else {
			//no change to counter
			if ((p - N) % (2 * N) < N) {
				res.par[p] = 1;
			}
			//increment
			else {
				res.par[p] = 2;
			}
		}
	}

	trans ta = 0;
	trans tanext = 0;
	for(letter a = 0; a < A; a++){
		ta = tanext;
		tanext = alph.nextLetter(a);
		for(trans t = ta; t < tanext; t++){
			state p = alph.from(t,a);
			state q = alph.to(t);

			hint[p][q].insert(t);
			res.addTrans(p,0,q);
			out[p].insert(q);
			in[q].insert(p);

			for(state c = 0; c < C; c++){
				state pE = p + 2*c*N + N;
				state pI = pE + N;
				state qE = q + 2*c*N + N;
				state qI = qE + N;

				hint[p][qE].insert(t);
				res.addTrans(p,0,qE);
				out[p].insert(qE);
				in[qE].insert(p);

				if(!alph.isResetTrans(t,c)){
					if(alph.isIncTrans(t,c)){
						hint[pE][qI].insert(t);
						hint[pI][qI].insert(t);
						res.addTrans(pE,0,qI);
						out[pE].insert(qI);
						in[qI].insert(pE);
						res.addTrans(pI,0,qI);
						out[pI].insert(qI);
						in[qI].insert(pI);
					}
					else{
						hint[pE][qE].insert(t);
						hint[pI][qE].insert(t);
						res.addTrans(pE,0,qE);
						out[pE].insert(qE);
						in[qE].insert(pE);
						res.addTrans(pI,0,qE);
						out[pI].insert(qE);
						in[qE].insert(pI);
					}
				}
			}
		}
	}

	std::vector<state>* oldind = new std::vector<state>();

	bool rejsink = !res.prune(out,in,oldind);

	if(!rejsink){
		//not all states have been pruned
		res.hoa_hint = std::vector<std::string>(res.N,"");
		for(state p = 0; p < res.N; p++){
			state pold = (*oldind)[p];
			for(state q = 0; q < res.N; q++){
				state qold = (*oldind)[q];
				std::string hintstr = "";
				bool found = false;
				for(trans t : hint[pold][qold]){
					if(found) hintstr += " | ";
					else{
						hintstr += "\n[";
					}
					found = true;
					hintstr += alph.hint(t);
				}
				if(found){
					hintstr += "] "+std::to_string(q);
					res.hoa_hint[p] += hintstr;
				}
			}
		}
	}
	delete oldind; oldind = NULL;
	
	//res.A = alph.letterOffset[A-1] + (alph.TS - alph.letterSetIndices[A-1]);
	res.A = 1;
	res.A = res.A << alph.bits;

	if(rejsink){
		//initial state was removed, res is rejecting all words
		//no hint computed, thus fill transitions of res
		res.trans[0].resize(res.A);
		for(letter a = 0; a < res.A; a++){
			res.trans[0][a] = {0};
		}
	}

	//statistics
	if(opt->logdata()){
		double filled = (opt->compressHoa() ? 0 : res.transMatrixFilled());
		std::string log = opt->make_prefix()+" nondetTS"
		": N="+std::to_string(res.N)+", "
		" A="+std::to_string(res.A)+", %="+(opt->compressHoa() ? "?" : std::to_string(100.0*filled))+"\n";
		opt->writeToLog(log);
	}
	if(opt->stats()){
		opt->add_data(NDET_N, res.N);
		opt->add_data(NDET_NMAX, N_);
		opt->add_data(Data::A, res.A);
		opt->add_data(NDET_CALLS,1L);
	}

	return res;
}

//make -wc2 using transition sets
ParityAut makeWC2TS(const MultiCounterAut& costAut, const Alphabet& alph, Options* opt) {
	if(opt->compressHoa()) return makeWC2TSC(costAut,alph,opt);
	state C = costAut.NbCounters;
	state N = costAut.NbStates;
	letter A = costAut.NbLetters;

	state N_ = N * (1 + 2 * C);
	letter A_ = alph.TS;

	//encoding of states:
	//(p) = p
	//(p,E,c) = N + 2*c*N + p
	//(p,I,c) = N + 2*c*N + N + p = 2*(c+1)*N + p
	
	ParityAut res(N_, A_);
	//initial states
	for (state p = 0; p < N; p++) {
		if (costAut.initialstate[p]) res.addInitial(p);
	}
	//parities
	for (state p = 0; p < N_; p++) {
		//'Q'
		if (p < N) {
			res.par[p] = 1;
		}
		else {
			//no change to counter
			if ((p - N) % (2 * N) < N) {
				res.par[p] = 1;
			}
			//increment
			else {
				res.par[p] = 2;
			}
		}
	}

	//transitions
	//index of first transition set over a
	letter ta = alph.letterSetIndices[0];
	//index of first transition set over next letter
	letter tan;
	//for all letters a of the original alphabet A
	for (letter a = 0; a < A; a++) {
		tan = alph.nextLetterSet(a);
		//for all transition sets a_ over letter a
		for (letter a_ = ta; a_ < tan; a_++) {
			//a_ is the a_n'th transition set over a -> a_n bit representation is used to determine if a_ contains some transition
			letter a_n = a_ - ta;
			//index of first transition over a
			letter t0 = alph.letterIndices[a]; //alph.fromIndices[a][0]; 
			//index of first transition from p over a
			letter t = t0;
			//index of first transition from next state over a
			letter tn;
			//make outgoing transitions for all states p in the part without fixed counter of the resulting ParityAut ('Q')
			for (state p = 0; p < N; p++) {
				tn = alph.nextFrom(a,p);
				//bit representation of transition tt -> a transition set ts over a contains tt iff ts&ttbit>0
				letter ttbit = ((letter)1) << (t - t0);
				//for all transitions tt that start in p
				for (letter tt = t; tt < tn; tt++ ) {
					//if transition set a_ contains transition tt
					if ((a_n & ttbit) > 0) {
						//transition tt is to q
						state q = alph.transitions[tt];
						// Q -> Q
						res.addTrans(p,a_,q);
						for (state c = 0; c < C; c++) {
							// p,a -> (q,?)
							// (q,e) of counter c
							// Q -> Q_c
							state qce = q + N + c * 2 * N;
							res.addTrans(p,a_,qce);
							// (p,E),a,(q,?)
							// (p,I),a,(q,?)
							// Q_c -> Q_c
							state pce = p + N + c * 2 * N;
							state pci = p + (c + 1) * 2 * N;
							cact ttcact = alph.counterActions[tt];
							//ttcact is unsigned -> have to check > C before substraction
							if (alph.isIncTrans(tt,c)) {
								//tt increments c
								state qci = q + (c + 1) * 2 * N;
								res.addTrans(pce,a_,qci);
								res.addTrans(pci,a_,qci);
							}
							else if(!alph.isResetTrans(tt,c)){
								res.addTrans(pce,a_,qce);
								res.addTrans(pci,a_,qce);
							}
						}
					}
					//representation of next transition is this one shifted to the left by one
					ttbit = ttbit << 1;
				}
				t = tn;
			}
		}
		ta = tan;
	}

	res.prune();

	//statistics
	if(opt->logdata()){
		double filled = res.transMatrixFilled();
		std::string log = opt->make_prefix()+" nondetTS"
		": N="+std::to_string(res.N)+", "
		" A="+std::to_string(res.A)+", %="+std::to_string(100.0*filled)+"\n";
		opt->writeToLog(log);
	}
	if(opt->stats()){
		opt->add_data(NDET_N, res.N);
		opt->add_data(NDET_NMAX, N_);
		opt->add_data(Data::A, A_);
		opt->add_data(NDET_CALLS,1L);
	}

	return res;
}

//uses DPA.hoatrans instead of DPA.trans!
ParityGame makeParityGameTS(const MultiCounterAut& costAut, const Alphabet& alph, const DetParityAut& det, std::vector<std::string>* statenames, Options* opt){
	//****************DEBUG*********//
	auto start = std::chrono::steady_clock::now();

	bool DEBUG = opt->debug();
	bool LOG = opt->logdata();

	const state costset = 1 << costAut.NbStates;
	const state NMaxRes = det.N * costset * costset * (costAut.NbLetters+1) + 2;

	std::vector<bool> visiteddet(det.N,false);
	unsigned int wc3violations = 0;
	unsigned int p0alreadyexisted = 0;
	unsigned int p1alreadyexisted = 0;
	unsigned long outtranschecked = 0;
	unsigned int fbufferhits = 0;
	unsigned int fbuffersize = 0;
	//********DEBUG*****************//
	//IO: saves names of states for later IO
	const bool NAMES = (statenames != NULL);
	std::vector<std::string> names0;
	std::vector<std::string> names1;

	state Ncost = costAut.NbStates;
	letter Acost = costAut.NbLetters;

	//precompute transitions of costAut
	//(P x A) -> reachable states of costAut
	vector<vector<vector<state>>> transcost(Ncost,vector<vector<state>>(Acost,vector<state>()));
	for(letter a = 0; a < Acost; a++){
		const MultiCounterMatrix* mat = costAut.get_trans(a);
		for(state p = 0; p < Ncost; p++){
			for(state q = 0; q < Ncost; q++){
				int coeff = mat->get(p,q);
				if(coeff!=MultiCounterMatrix::bottom()) transcost[p][a].push_back(q);
			}
		}
	}

	//states
	state N0 = 0;
	state N1 = 0;
	vector<parity> par0, par1;
	vector<set<state>> out0, out1;

	vector<bool> cinit = costAut.initialstate;
	vector<bool> cacc = costAut.finalstate;

	//states that already have an ID, but still need to have their out-neighbors calculated
	stack<gamestate> todo;
	//gamestate -> ID (index in par0/par1, out0/out1)
	unordered_map<gamestate, state, hash<gamestate>> ID;

	//used to iterate over transition sets of transitions starting in a set of states
	tset maxts,mints;
	std::vector<tset> check;
	std::vector<tset> add;

	//buffers for each state set whether or not it contains a final state of costAut
	unordered_map<vector<bool>,bool> F;

	//initialize
	state init = 0;
	gamestate initgs;
	initgs.det = det.init;
	initgs.last = Acost;
	initgs.reach = cinit;
	initgs.reachrestricted = cinit;
	ID[initgs] = init;
	par1.push_back(det.par[det.init]);
	out1.push_back({});
	N1 = 1;
	bool finit = false;
	for(state p = 0; p < Ncost; p++){
		if(cinit[p]&&cacc[p]){
			finit = true;
			break;
		}
	}
	F[cinit] = finit;

	visiteddet[det.init] = true;		//DEBUG

	if(NAMES) names1.push_back(initgs.to_string(Acost));

	todo.push(initgs);

	//while there are still states with outgoing edges to be calculated
	//an outgoing edge may lead to a new state/states, adding them to todo
	while(!todo.empty()){
		gamestate gs = todo.top();
		todo.pop();
		state g = ID[gs];

		if(DEBUG) std::cout << "Doing: " << gs.to_string(Acost) << ":" << g << std::endl;		//DEBUG

		if(gs.last<Acost){
			//Player 0's turn

			for(auto pp : det.hoatrans[gs.det][gs.last]){
				//for all transition sets over gs.last that transition det from gs.det to pp.first
				//uses the hoa output of SPOT to avoid 'subsets'
				for(letter ts : pp.second){

					outtranschecked++;		//DEBUG

					gamestate gsnew(pp.first,gs.reach,alph.deltaRestricted(gs.reachrestricted,gs.last,ts),Acost);		//successor gamestate

					if(ID.count(gsnew)>0){
						p1alreadyexisted++;		//DEBUG

						if(out0[g].count(ID[gsnew])>0) continue;

						out0[g].insert(ID[gsnew]);
						continue;
					}

					bool gsf = F[gs.reach];				//true iff current a_0 a_1 ... a_i is accepted by the cost automaton

					visiteddet[gsnew.det] = true;		//DEBUG
					fbufferhits++;						//DEBUG

					//does not exist yet

					//check if gsnew violates WC3
					if(gsf){
						if(F.count(gsnew.reachrestricted)>0){

							fbufferhits++;	//DEBUG

							if(!F[gsnew.reachrestricted]){

								wc3violations++;	//DEBUG

								if(DEBUG) std::cout << "\tDid not add new gamestate " << gsnew.to_string(Acost) << " because it violates WC3." << std::endl;		//DEBUG

								continue;	//gsnew violates WC3
							}
						}
						else{
							//make buffer entry for gsnew.reachrestricted
							bool fgsnew = false;
							for(state pc = 0; pc < Ncost; pc++){
								if(cacc[pc]&&gsnew.reachrestricted[pc]){
									fgsnew = true;
									break;
								}
							}
							F[gsnew.reachrestricted] = fgsnew;
							if(!fgsnew){

								wc3violations++;	//DEBUG

								if(DEBUG) std::cout << "\tDid not add new gamestate " << gsnew.to_string(Acost) << " because it violates WC3." << std::endl;		//DEBUG

								continue;	//gsnew violates WC3
							}
						}
					}

					//add new state
					ID[gsnew] = N1;
					out0[g].insert(N1);
					par1.push_back(det.par[gsnew.det]);
					out1.push_back({});
					todo.push(gsnew);

					if(DEBUG) std::cout << "\tAdded transition to new gamestate " << gsnew.to_string(Acost) << "[" << det.par[gsnew.det] << "]:"<<N1<<"." << std::endl;		//DEBUG
					if(NAMES) names1.push_back(gsnew.to_string(Acost));

					N1++;
				}
			}
		}
		else{
			//Player 1's turn
			for(letter a = 0; a < Acost; a++){

				outtranschecked++;		//DEBUG

				//calculate reachable states in costAut
				vector<bool> reachnew(Ncost,false);
				bool empty = true;
				for(state p = 0; p < Ncost; p++){
					if(gs.reach[p]){
						for(state q : transcost[p][a]) {
							reachnew[q] = true;
							empty = false;
						}
					}
				}

				if(empty) continue;		//set of reachable states from gs.reach reading a is empty -> p0 wins from this state

				//update buffer
				if(F.count(reachnew)==0){
					bool gsnewf = false;
					for(state p = 0; p < Ncost; p++){
						if(cacc[p]&&reachnew[p]){
							gsnewf = true;
							break;
						}
					}
					F[reachnew] = gsnewf;
				}

				//add state (gs.det, reachnew, gs.reachrestricted, a) if it doesnt exist yet
				gamestate gsnew(gs.det, reachnew, gs.reachrestricted, a);
				state gnew;

				visiteddet[gsnew.det] = true;		//DEBUG

				if(ID.count(gsnew)>0){

					p0alreadyexisted++;	//DEBUG

					//already exists
					gnew = ID[gsnew];

					if(DEBUG) std::cout << "\tAdded transition to existing gamestate " << gsnew.to_string(Acost) << ":"<<gnew<<"." << std::endl;		//DEBUG
				}
				else{
					//new state
					gnew = N0;
					N0++;
					par0.push_back(det.par[gsnew.det]);
					out0.push_back({});

					ID[gsnew] = gnew;
					todo.push(gsnew);

					if(DEBUG) std::cout << "\tAdded transition to new gamestate " << gsnew.to_string(Acost) << "[" << det.par[gsnew.det] << "]:"<<gnew<<"." << std::endl;		//DEBUG
				
					if(NAMES) names0.push_back(gsnew.to_string(Acost));

				}
				//add edge from g to gnew
				out1[g].insert(gnew);
			}
		}
	}

	fbuffersize = F.size();		//DEBUG

	if(NAMES){
		statenames->clear();
		statenames->reserve(N0+N1);
	}

	//transform into game
	ParityGame res(N0,N1);
	for(state p = 0; p < N0; p++){
		res.par[p] = par0[p];
		for(state q : out0[p]){
			res.addEdge(p,q+N0);
		}

		if(NAMES){
			statenames->push_back(names0[p]);
		}
	}
	for(state p = 0; p < N1; p++){
		res.par[p+N0] = par1[p];
		for(state q : out1[p]){
			res.addEdge(p+N0,q);
		}

		if(NAMES){
			statenames->push_back(names1[p]);
		}
	}

	res.addSinks();

	//statistics
	auto end = std::chrono::steady_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

	unsigned int viscount = 0;

	for(size_t i = 0; i < visiteddet.size(); i++){
		if(visiteddet[i]) viscount++;
	}
	if(DEBUG){
		std::cout << "Visited "<<viscount<<"/"<<det.N<<" states of the determinized automaton." << std::endl;	//DEBUG
		std::cout << "Total gamestates: " << res.N0 << "+" << res.N1 << std::endl;		//DEBUG
	}

	parity max = res.maxParity();	//(= det.maxParity())
	if(LOG){
		opt->writeToLog(opt->make_prefix() + " makeGame: took: "+took(time)+", "
		"vis="+std::to_string(viscount)+"/"+std::to_string(det.N)+", #3="+std::to_string(wc3violations)+", "
		"#out="+std::to_string(outtranschecked)+"\n");
		opt->writeToLog("\t\tresult: N="+std::to_string(res.N)+"/"+std::to_string(NMaxRes)+", P="+std::to_string(max)+"\n");		
	}
	if(opt->stats()){
		opt->add_data(GAME_N,res.N);
		opt->add_data(GAME_NMAX, NMaxRes);
		opt->add_data(GAME_P,max);
		opt->add_data(GAME_VIS,viscount);
		opt->add_data(GAME_VISMAX,det.N);
		opt->add_data(GAME_T, time);
		opt->add_data(GAME_CALLS,1L);
	}

	return res;
}

//the returned NPA has INVALID NPA.trans if opt->g=8 and opt->compressHoa(), see makeWC2TSC!
ParityAut chsndet(const MultiCounterAut& costAut, const Alphabet& alph, Options* opt, std::vector<state>* oldindices = NULL){
	switch(opt->g){
		case 1: return makeNonDetPartAlt(costAut,opt,oldindices);
		case 2: case 3: case 4: case 5: case 6: case 7: return makeNonDetPartSink(costAut,alph,opt,oldindices);
		case 8: return makeWC2TS(costAut,alph,opt);
		default: return makeNonDetPart(costAut,alph,opt,oldindices);
	}
}

//directly make the parity game given the cost aut, the alphabet and a DPA for wc2
//does not explicitly construct a DPA for the whole wc
//uses DPA.hoatrans instead of DPA.trans if opt->g==8!
ParityGame makeParityGame(const MultiCounterAut& costAut, const Alphabet& alph, const DetParityAut& det, std::vector<std::string>* statenames, Options* opt){
	if(opt->g==8) return makeParityGameTS(costAut,alph,det,statenames,opt);
	
	//****************DEBUG*********//
	auto start = std::chrono::steady_clock::now();

	bool ALT = opt->useAlt();
	bool DEBUG = opt->debug();
	bool LOG = opt->logdata();

	const state costset = 1 << costAut.NbStates;
	const state NMaxRes = det.N * costset * costset * (costAut.NbLetters+1) + 2;

	std::vector<bool> visiteddet(det.N,false);
	unsigned int wc3violations = 0;
	unsigned int proxiescreated = 0;
	unsigned int p0alreadyexisted = 0;
	unsigned int p1alreadyexisted = 0;
	unsigned long outtranschecked = 0;
	unsigned int fbufferhits = 0;
	unsigned int fbuffersize = 0;
	//********DEBUG*****************//


	//IO: saves names of states for later IO
	const bool NAMES = (statenames != NULL);
	std::vector<std::string> names0;
	std::vector<std::string> names1;

	state Ncost = costAut.NbStates;
	letter Acost = costAut.NbLetters;
	trans Tcost = alph.T;

	//precompute transitions of costAut
	//(P x A) -> reachable states of costAut
	vector<vector<vector<state>>> transcost(Ncost,vector<vector<state>>(Acost,vector<state>()));
	for(letter a = 0; a < Acost; a++){
		const MultiCounterMatrix* mat = costAut.get_trans(a);
		for(state p = 0; p < Ncost; p++){
			for(state q = 0; q < Ncost; q++){
				int coeff = mat->get(p,q);
				if(coeff!=MultiCounterMatrix::bottom()) transcost[p][a].push_back(q);
			}
		}
	}

	//states
	state N0 = 0;
	state N1 = 0;
	vector<parity> par0, par1;
	vector<set<state>> out0, out1;

	//a state of p1 may exist with different parities
	//lookup of state IDs is not dependent on parities 
	//	-> par[ID[state]] is the parity of the 'real' state, the others are 'proxies'
	//		->since outgoing edges are equal for all of them, only edges of 'real' states are computed
	//		->later copy outgoing edges of 'real' states for each 'proxy'
	//Q1 -> (P -> Q1)
	vector<unordered_map<parity,state,hash<parity>>> proxies;

	vector<bool> cinit = costAut.initialstate;
	vector<bool> cacc = costAut.finalstate;

	//states that already have an ID, but still need to have their out-neighbors calculated
	stack<gamestate> todo;
	//gamestate -> ID (index in par0/par1, out0/out1)
	unordered_map<gamestate, state, hash<gamestate>> ID;

	//buffers for each state set whether or not it contains a final state of costAut
	unordered_map<vector<bool>,bool> F;

	//********subgame**********//
	//calculate states that are reachable by some transition sequence by iteration over all transitions
	//	and calculating states that are reachable by taking or not taking the current transition
	//		a transition sequence can be empty and even player can still win from that state
	//			(if uneven player can not reach a final state of costAut; see wc3)

	//number of seqstates of current subgame
	state Nseq;
	//for each seqstate of current subgame:
	//the best parity with which the seqstate is reachable
	vector<parity> optpar;
	vector<parity> optparnext;

	//seqstate -> ID (index in maxeven/minodd)
	unordered_map<seqstate,state,hash<seqstate>> IDseq;

	//reachable seqstates given transitions up to the current transition
	unordered_set<seqstate> reachseq;
	vector<seqstate> addreach;

	//initial seqstate, corresponds to some gamestate of p0
	seqstate initseq;
	//*************************//

	//initialize
	state init = 0;
	gamestate initgs;
	initgs.det = det.init;
	initgs.last = Acost;
	initgs.reach = cinit;
	initgs.reachrestricted = cinit;
	ID[initgs] = init;
	par1.push_back(det.par[det.init]);
	out1.push_back({});
	proxies.push_back({});
	proxies[init][par1[init]] = init;
	N1 = 1;
	bool finit = false;
	for(state p = 0; p < Ncost; p++){
		if(cinit[p]&&cacc[p]){
			finit = true;
			break;
		}
	}
	F[cinit] = finit;

	visiteddet[det.init] = true;		//DEBUG

	if(NAMES) names1.push_back(initgs.to_string(Acost));

	todo.push(initgs);

	//while there are still states with outgoing edges to be calculated
	//an outgoing edge may lead to a new state/states, adding them to todo
	while(!todo.empty()){
		gamestate gs = todo.top();
		todo.pop();
		state g = ID[gs];

		if(DEBUG) std::cout << "Doing: " << gs.to_string(Acost) << ":" << g << std::endl;		//DEBUG

		if(gs.last<Acost){
			//player 0's turn

			//determine out-neighbors by "subgame"
			//init subgame
			initseq = seqstate(gs.det, vector<bool>(Ncost,false));
			Nseq = 1;
			reachseq = {initseq};
			addreach = {};
			IDseq.clear();
			IDseq[initseq] = 0;
			optpar = {0};
			optparnext = optpar;

			if(DEBUG) std::cout << "\t\tSubgame: initial state: " << initseq.to_string() << std::endl;	//DEBUG
			
			//for each transition over current letter gs.last (see wc1!)
			for(state p = 0; p < Ncost; p++){

				if(checkfrom) if(!gs.reachrestricted[p]) continue;

				for(trans t = alph.fromIndices[gs.last][p]; t < alph.nextFrom(gs.last,p); t++){

					if(DEBUG) std::cout << "\t\tCurrent transition: from " << p << " over " << gs.last << " to " << alph.transitions[t] << std::endl;		//DEBUG 

					//apply transition to each currently reachable seqstate
					for(seqstate ssold : reachseq){

						outtranschecked++;		//DEBUG

						state detnew;
						parity pnew;
						if(ALT){		//transitions are encoded by from-over-to sequence
							detnew = det.trans[ssold.det][Acost+p];
							pnew = det.par[detnew];
							detnew = det.trans[detnew][gs.last];
							pnew = std::max(pnew,det.par[detnew]);
							detnew = det.trans[detnew][Acost+alph.transitions[t]];
							pnew = std::max(pnew,det.par[detnew]);
						}
						else{
							detnew = det.trans[ssold.det][Acost+t];
							pnew = det.par[detnew];
						}

						visiteddet[detnew] = true;		//DEBUG

						//add new seqstate if it doesnt exist yet
						seqstate ssnew(detnew,ssold.reachnew);
						state sold = IDseq[ssold];
						if(gs.reachrestricted[p]) ssnew.reachnew[alph.transitions[t]] = true;
						state snew;
						pnew = std::max(optpar[sold],pnew);

						if(DEBUG) std::cout << "\t\t\tAdded edge from " << ssold.to_string() << " to " << ssnew.to_string() << std::endl;	//DEBUG

						if(IDseq.count(ssnew)>0){	//already exists
							snew = IDseq[ssnew];

							if(DEBUG) std::cout << "\t\t\tAlready exists: old parity: "<<optparnext[snew] <<" , new parity: ";	//DEBUG

							optparnext[snew] = optEven(pnew,optparnext[snew]);

							if(DEBUG) std::cout << optparnext[snew] << std::endl;			//DEBUG
						}
						else{			//new state
							IDseq[ssnew] = Nseq;
							optparnext.push_back(pnew);
							Nseq++;
							addreach.push_back(ssnew);

							if(DEBUG) std::cout << "\t\t\tAdded new seqstate with parity "<<pnew <<std::endl;		//DEBUG
						}
					}
					for(seqstate ssnew : addreach){		//add new states to reachable seqstates
						reachseq.insert(ssnew);
					}
					addreach.clear();
					optpar = optparnext;
				}
			}

			if(DEBUG) std::cout << "\t\tTotal reachable seqstates: " << reachseq.size() << "." << std::endl;		//DEBUG

			fbufferhits++;	//DEBUG

			bool gsf = F[gs.reach];
			//transform each seqstate in reachseq into a gamestate
			for(seqstate ssnew : reachseq){
				gamestate gsnew(ssnew.det,gs.reach,ssnew.reachnew,Acost);
				state snew = IDseq[ssnew];
				state gnew;
				if(ID.count(gsnew)>0){

					p1alreadyexisted++;	//DEBUG

					//gamestate gsnew already exists
					gnew = ID[gsnew];
					if(proxies[gnew].count(optpar[snew])>0){
						//gsnew exists with same parity
						out0[g].insert(proxies[gnew][optpar[snew]]);

						if(DEBUG) std::cout << "\tAdded transition to existing gamestate " << gsnew.to_string(Acost) << ":"<<proxies[gnew][optpar[snew]]<<"." << std::endl;	//DEBUG
					}
					else{

						proxiescreated++;	//DEBUG

						//make new proxy
						proxies[gnew][optpar[snew]] = N1;
						out0[g].insert(N1);
						par1.push_back(optpar[snew]);
						out1.push_back({});		//fix outgoing edges of proxy later, are just copies of original gamestate
						proxies.push_back({});

						if(DEBUG) std::cout << "\tAdded transition to proxy of existing gamestate " << gsnew.to_string(Acost) << "[" << optpar[snew] << "]:"<<N1<<"." << std::endl;		//DEBUG
					
						if(NAMES) names1.push_back(gsnew.to_string(Acost));

						N1++;
					}
				}
				else{
					//gamestate gsnew does not yet exist

					//check if gsnew violates WC3
					if(gsf){
						if(F.count(gsnew.reachrestricted)>0){

							fbufferhits++;	//DEBUG

							if(!F[gsnew.reachrestricted]){

								wc3violations++;	//DEBUG

								if(DEBUG) std::cout << "\tDid not add new gamestate " << gsnew.to_string(Acost) << " because it violates WC3." << std::endl;		//DEBUG

								continue;	//gsnew violates WC3
							}
						}
						else{
							//make buffer entry for gsnew.reachrestricted
							bool fgsnew = false;
							for(state pc = 0; pc < Ncost; pc++){
								if(cacc[pc]&&gsnew.reachrestricted[pc]){
									fgsnew = true;
									break;
								}
							}
							F[gsnew.reachrestricted] = fgsnew;
							if(!fgsnew){

								wc3violations++;	//DEBUG

								if(DEBUG) std::cout << "\tDid not add new gamestate " << gsnew.to_string(Acost) << " because it violates WC3." << std::endl;		//DEBUG

								continue;	//gsnew violates WC3
							}
						}
					}

					//add new state gsnew
					parity pnew = optpar[snew];
					ID[gsnew] = N1;
					out0[g].insert(N1);
					par1.push_back(pnew);
					out1.push_back({});
					proxies.push_back({});
					proxies[N1][pnew] = N1;

					todo.push(gsnew);

					if(DEBUG) std::cout << "\tAdded transition to new gamestate " << gsnew.to_string(Acost) << "[" << pnew << "]:"<<N1<<"." << std::endl;		//DEBUG
				
					if(NAMES) names1.push_back(gsnew.to_string(Acost));

					N1++;
				}
			}
		}
		else{
			//player 1's turn
			//pick a letter
			for(letter a = 0; a < Acost; a++){

				outtranschecked++;		//DEBUG

				//calculate reachable states in costAut
				vector<bool> reachnew(Ncost,false);
				bool empty = true;
				for(state p = 0; p < Ncost; p++){
					if(gs.reach[p]){
						for(state q : transcost[p][a]) {
							reachnew[q] = true;
							empty = false;
						}
					}
				}

				if(empty) continue;	//p0 can always win from here by always playing the empty transition set

				//update buffer
				if(F.count(reachnew)==0){
					bool found = false;
					for(state p = 0; p < Ncost; p++){
						if(cacc[p]&&reachnew[p]){
							F[reachnew] = true;
							found = true;
							break;
						}
					}
					if(!found) F[reachnew] = false;
				}

				//add state (det.trans[gs.det][a], reachnew, gs.reachrestricted, a) if it doesnt exist yet
				gamestate gsnew(det.trans[gs.det][a], reachnew, gs.reachrestricted, a);
				state gnew;

				visiteddet[gsnew.det] = true;		//DEBUG

				if(ID.count(gsnew)>0){

					p0alreadyexisted++;	//DEBUG

					//already exists
					gnew = ID[gsnew];

					if(DEBUG) std::cout << "\tAdded transition to existing gamestate " << gsnew.to_string(Acost) << ":"<<gnew<<"." << std::endl;		//DEBUG
				}
				else{
					//new state
					gnew = N0;
					N0++;
					par0.push_back(det.par[gsnew.det]);
					out0.push_back({});

					ID[gsnew] = gnew;
					todo.push(gsnew);

					if(DEBUG) std::cout << "\tAdded transition to new gamestate " << gsnew.to_string(Acost) << "[" << det.par[gsnew.det] << "]:"<<gnew<<"." << std::endl;		//DEBUG
				
					if(NAMES) names0.push_back(gsnew.to_string(Acost));

				}
				//add edge from g to gnew
				out1[g].insert(gnew);
			}
		}
	}

	fbuffersize = F.size();		//DEBUG

	//fix outgoing edges of proxies
	for(state p = 0; p < N1; p++){
		if(proxies[p].size()>1){
			for(pair<parity,state> prox : proxies[p]){
				if(prox.first!=par1[p]){
					//copy edges of 'original'
					out1[prox.second] = out1[p];
				}
			}
		}
	}

	//IO
	if(NAMES){
		statenames->clear();
		statenames->reserve(N0+N1);
	}

	//transform into game
	ParityGame res(N0,N1);
	for(state p = 0; p < N0; p++){
		res.par[p] = par0[p];
		for(state q : out0[p]){
			res.addEdge(p,q+N0);
		}

		if(NAMES){
			statenames->push_back(names0[p]);
		}
	}
	for(state p = 0; p < N1; p++){
		res.par[p+N0] = par1[p];
		for(state q : out1[p]){
			res.addEdge(p+N0,q);
		}

		if(NAMES){
			statenames->push_back(names1[p]);
		}
	}

	res.addSinks();	//OINK requires that every state has a successor

	//statistics
	auto end = std::chrono::steady_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();

	unsigned int viscount = 0;

	for(size_t i = 0; i < visiteddet.size(); i++){
		if(visiteddet[i]) viscount++;
	}
	if(DEBUG){
		std::cout << "Visited "<<viscount<<"/"<<det.N<<" states of the determinized automaton." << std::endl;	//DEBUG
		std::cout << "Total gamestates: " << res.N0 << "+" << res.N1 << std::endl;		//DEBUG
	}

	parity max = res.maxParity();	//(= det.maxParity())
	if(LOG){
		opt->writeToLog(opt->make_prefix() + " makeGame: took: "+took(time)+", a="+(opt->useAlt()?"T":"F")+", "
		"vis="+std::to_string(viscount)+"/"+std::to_string(det.N)+", #3="+std::to_string(wc3violations)+", "
		"#pr="+std::to_string(proxiescreated)+", #out="+std::to_string(outtranschecked)+"\n");
		opt->writeToLog("\t\tresult: N="+std::to_string(res.N)+"/"+std::to_string(NMaxRes)+", P="+std::to_string(max)+"\n");		
	}
	if(opt->stats()){
		opt->add_data(GAME_N,res.N);
		opt->add_data(GAME_NMAX, NMaxRes);
		opt->add_data(GAME_P,max);
		opt->add_data(GAME_VIS,viscount);
		opt->add_data(GAME_VISMAX,det.N);
		opt->add_data(GAME_T, time);
		opt->add_data(GAME_CALLS,1L);
	}

	return res;
}

ParityAut makeNonDetPartTEST(const MultiCounterAut& costAut, Options* opt){
	Alphabet alph(costAut,opt->g==8,opt->compressHoa());
	std::vector<state>* oldindices = NULL;
	if(opt->output()&&opt->canWrite()){
		oldindices = new std::vector<state>();
	}
	ParityAut nondet = chsndet(costAut,alph,opt,oldindices);
	if(opt->output()&&opt->canWrite()){
		std::string path = opt->make_path_prefix()+"_nondet.dot";
		try{
			if(opt->g==0||opt->g==1) writeToFile(path,nondetpart_to_dot(nondet,alph,opt->useAlt(),true, oldindices));
			else if(!opt->compressHoa()||opt->g!=8) writeToFile(path,nondet.to_dot(false));
			else{
				letter oldA = nondet.A;
				nondet.A = 1;
				std::string dotstr = nondet.to_dot(false);
				nondet.A = oldA;
				writeToFile(path,dotstr);
			}
		}
		catch(const std::exception& e){
			if(opt->verbose()||opt->debug()){
				std::string msg = e.what();
				std::cout << ">Failed to write dot file of non det. part of the winning condition: "+msg;
			}
		}
	}
	delete oldindices; oldindices = NULL;
	return nondet;
}

void compressDet(DetParityAut& det, const Alphabet& alph){
	std::vector<std::vector<state>> newtrans(det.N,std::vector<state>(alph.TS));
	for(state p = 0; p < det.N; p++){
		for(letter a = 0; a < alph.A; a++){
			tset off = alph.letterOffset[a]-alph.letterSetIndices[a];
			for(tset ts = alph.letterSetIndices[a]; ts < alph.nextLetterSet(a); ts++){
				newtrans[p][ts] = det.trans[p][off+ts];
			}
		}
	}
	det.trans = newtrans;
	det.A = alph.TS;
}

ParityGame makeParityGame(const MultiCounterAut& costAut, Options* opt, const Alphabet& alph){
	//IO
	if(opt->verbose()||opt->debug()) alph.printShort();
	auto start = std::chrono::steady_clock::now();
	std::vector<state>* oldindices = NULL;
	if(opt->output()&&opt->canWrite()) oldindices = new std::vector<state>();


	ParityAut nondet = chsndet(costAut,alph,opt,oldindices);

	//IO
	if(opt->verbose()){
		std::cout << ">Non deterministic part of the winning condition: " << nondet.N << " states, "<<nondet.A<<" letters." << std::endl;
		std::cout << ">Determinizing.." << std::endl;
	}
	if(opt->output()&&opt->canWrite()){
		std::string path = opt->make_path_prefix()+"_nondet.dot";
		try{
			if(opt->g==0||opt->g==1) writeToFile(path,nondetpart_to_dot(nondet,alph,opt->useAlt(),true, oldindices));
			else if(!opt->compressHoa()||opt->g!=8) writeToFile(path,nondet.to_dot(false));
			else{
				letter oldA = nondet.A;
				nondet.A = 1;
				std::string dotstr = nondet.to_dot(false);
				nondet.A = oldA;
				writeToFile(path,dotstr);
			}
		}
		catch(const std::exception& e){
			if(opt->verbose()||opt->debug()){
				std::string msg = e.what();
				std::cout << ">Failed to write dot file of non det. part of the winning condition: "+msg;
			}
		}
	}
	delete oldindices; oldindices = NULL;


	DetParityAut det = determinizeParityAut(&nondet,opt,false,(opt->g==8 ? &alph : NULL));
	
	if(false){		//there should be no rejecting sinks in -det, and rejecting sink in det should already be merged
		if(opt->verbose()){
			std::cout << ">Merging/identifying rejecting sinks.." << std::endl;
		}
		det.identifyAndMergeRejectingSinks();
		det.complement();
		det.identifyAndMergeRejectingSinks();
		if(opt->verbose()){
			std::cout << ">Determinized automaton with merged rejecting sinks: "<<det.N<<" states." << std::endl;
		}
	}
	else{
		det.complement();
	}

	//IO
	std::string makedet = took(start);
	if(opt->verbose()){
		std::cout << ">Determinized automaton: "<<det.N<<" states, maximal parity of "<<det.maxParity()<<"." << std::endl;
	}
	if(opt->verbose()){
		std::cout << ">Making parity game.." << std::endl;
	}
	std::vector<std::string>* statenames = NULL;
	if(opt->output()&&opt->canWrite()) statenames = new std::vector<std::string>();


	ParityGame game = makeParityGame(costAut,alph,det,statenames,opt);

	//IO
	if(opt->verbose()){
		std::cout << ">Parity game: " << game.N0 << "+" << game.N1 << " states, maximal parity of "<<game.maxParity()<<"." << std::endl;
	}
	if(opt->output()&&opt->canWrite()){
		while(statenames->size()<game.N){
			statenames->push_back("Sink");
		}
		std::string path = opt->make_path_prefix()+"_game.dot";
		try{
			writeToFile(path,game.to_dot(*statenames));
		}
		catch(const std::exception& e){
			if(opt->verbose()||opt->debug()){
				std::string msg = e.what();
				std::cout << ">Failed to write dot file of limitedness game: "+msg;
			}
		}
	}
	delete statenames; statenames = NULL;


	return game;
}

bool isLimited(MultiCounterAut& costAut, Options* opt){
	if(opt->verbose()){
		std::cout << ">Deciding limitedness.." << std::endl;
	}
	if(opt->debug()){
		costAut.print();
	}
	if(opt->verbose()){
		std::cout << ">Reducing to parity game.." << std::endl;
	}
	if(opt->logdata()) opt->writeToLog("\n");
	auto start = std::chrono::steady_clock::now();

	Alphabet alph(costAut,opt->g==8,opt->compressHoa());		//alphabet: contains numbering of transitions(/transition sets)

	bool res;
	if(alph.T>0&&alph.C>0&&alph.A>0){
		ParityGame game = makeParityGame(costAut, opt, alph);	//construct the limitedness game
		
		if(opt->verbose()){
			std::cout << ">Solving parity game.." << std::endl;
		}
		res = solveParityGame(game,opt)[game.N0]==P0;		//solve parity game;
														//limited iff even player wins from first state of odd player
	}
	else res = true;

	auto end = std::chrono::steady_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
	std::string time = took(us);
	if(opt->logdata()) opt->writeToLog(opt->make_prefix()+" lim: took: "+time+", result: "+(res ? "T" : "F")+"\n");

	if(opt->stats()){
		opt->add_data(LIM_T,us);
		opt->add_data(LIM_CALLS,1L);
		if(res) opt->add_data(LIM_LIM,1L);
	}
	return res;
}

unsigned int starheight(const std::string regex, Options* opt, bool useSTAMINA){
	throw logic_error("Star height from regex: Functionality not implemented!");
}

unsigned int starheight(ClassicAut& aut, Options* opt){
	ResetOPT r(opt);
	if(opt->verbose()){
		std::cout << ">Computing star height.." << std::endl;
	}
	if(opt->debug()){
		aut.print();
	}
	unsigned int MAX;		//upper bound for the star height
	if(opt->useLoopComplexity()){
		if(opt->verbose()){
			std::cout << ">Computing maximal star height using STAMINA.." << std::endl;
		}
		MAX = maxStarHeight(aut);
	}
	else{
		MAX = -1;
	}
	if(opt->verbose()&&opt->useLoopComplexity()){
		std::cout << ">Maximal star height is " << MAX <<"."<< std::endl;
	}
	MultiCounterAut* red;		//reduction to limitedness
	for(unsigned int k = 1; k < MAX; k++){		//assumes sh >0
		opt->setline(k);
		if(opt->verbose()){
			std::cout << ">Testing for star height of " << k << ".."<< std::endl;
			std::cout << ">Reducing to limitedness using STAMINA.." << std::endl;
		}
		red = reductionSTAMINA(aut,k,opt);		//reduce
		if(opt->output()&&opt->canWrite()){
			std::string path = opt->make_path_prefix(true)+(k==0?"_0":"")+"_red";
			try{
				writeToFile(path,to_stamina(*red));
			}
			catch(const std::exception& e){
				if(opt->verbose()||opt->debug()){
					std::cout << ">Failed to write cost automaton of star height reduction to file: "<<e.what()<<std::endl;
				}
			}
		}
		if(opt->verbose()){
			std::cout << ">Computing limitedness.." << std::endl;
		}
		bool lim = isLimited(*red, opt);		//check limitedness
		if(opt->verbose()){
			std::cout << ">Cost automaton is" << (lim ? "" : " not") << " limited." << std::endl;
		}
		delete red; red = NULL;
		if(lim) return k;	//resulting star height
	}
	if(opt->verbose()){
		std::cout << ">Star height must have the calculated maximal value "<<MAX << "."<<std::endl;
	}
	return MAX;
}

bool finitePower(const std::string regex, Options* opt, bool useSTAMINA){
	throw logic_error("Finite power from regex: Functionality not implemented!");
}

bool finitePower(const ClassicAut& aut, Options* opt, bool useSTAMINA){
	return finitePower(classicToClassicEps(aut),opt);
}

bool finitePower(const ClassicEpsAut& aut, Options* opt, bool useSTAMINA){
	if(opt->verbose()){
		cout << ">Computing finite power.." << endl;
	}
	if(opt->debug()){
		printNFA(aut);
	}
	if(opt->verbose()){
		cout << ">Reducing to limitedness.." << endl;
	}
	MultiCounterAut* red = NULL;		//reduction to limitedness
	try{
		red = finitePowerCaut(aut);		//reduce
	}
	catch(const invalid_argument& e){
		string msg = e.what();
		throw invalid_argument("Error during reduction to limitedness: "+msg);
	}
	if(opt->output()&&opt->canWrite()){
		std::string path = opt->make_path_prefix()+"_red.dot";
		try{
			writeToFile(path,to_stamina(*red));
		}
		catch(const std::exception& e){
			if(opt->verbose()||opt->debug()){
				std::string msg = e.what();
				std::cout << ">Failed to write cost automaton of finite power reduction to file: "+msg;
			}
		}
	}
	if(opt->verbose()){
		if(useSTAMINA) cout << ">Running STAMINA.." << endl;
	}
	bool lim;
	try
	{	//check limitedness
		if(useSTAMINA) lim = isLimitedSTAMINA(*red,opt);
		else lim = isLimited(*red,opt);
	}
	catch(const exception& e){
		string msg = e.what();
		delete red; red = NULL;
		throw runtime_error("Error during computation of limitedness: "+msg);
	}
	delete red; red = NULL;
	return lim;		//has finite power iff red is limited
}