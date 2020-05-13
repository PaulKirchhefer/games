#include "Automata.hpp"
#include "StarHeight.hpp"
#include "games.h"
#include "parity.h"
#include "wrapper.h"
#include "parser.h"
#include "options.h"
#include <iostream>
#include <stdexcept>
#include "iostuff.h"

/*
SOURCES:
	OINK		https://github.com/trolando/oink
	SPOT:		https://spot.lrde.epita.fr/
	STAMINA:	https://stamina.labri.fr/
*/

/*
REQUIRES installation of SPOT/OINK,
	ie the program must be able to call autfilt/oink by command, using std::system

ONLY TESTED ON UBUNTU
*/

void test(){
	
}

int main(int argc, char *argv[]){
	//test();

	bool finished = false;
	std::vector<std::string> results;
	Options OPT(&results);
	for(int i = 1; i < argc; i++){
		std::string arg = trim(argv[i]);
		if(arg=="-i"){
			finished = true;
			i++;
			if(i < argc){
				std::string arg2 = trim(argv[i]);
				try{
					handleFilePath(arg2,&OPT);
					if(results.size()>0){
						cout << ">>Results:" << endl;
						for(size_t i = 0; i < results.size(); i++){
							cout << ">>" << results[i] << endl;
						}
					}
					results.clear();
				}
				catch(const invalid_argument& e){
					cerr << "Syntax error, stopped parsing file: " << e.what() << endl;
				}
				catch(const runtime_error& e){
					cerr << "Error reading file: " << e.what() << endl;
				}
			}
			else{
				cerr << "Syntax error, ignored \"-i\" without path argument." << endl;
			}
		}
		else if(startsWith(arg, "-")){
			try{
				OPT.update(arg);
			}
			catch(const invalid_argument& e){
				cerr << "Syntax error, ignored \""+arg+"\": " << e.what() << endl;
			}
		}
		else{
			cerr << "Syntax error, ignored \""+arg+"\"." << endl;
		}
	}

	if(finished) return 0;

	//****DEBUG****//
	//OPT.filelocation= "/home/paul/Desktop/TEST/";
	//OPT.outputdirectory="/home/paul/Desktop/TEST/output/";
	//*************//

	std::cout << "Current output directory: " << (OPT.outputdirectory.empty() ? "none" : "'"+OPT.outputdirectory+"'") << endl;
	std::cout << "Command directory: " << (OPT.filelocation.empty() ? "none" : "'"+OPT.filelocation+"'") << endl;
	std::cout << "Type 'h' for help and 'q' to quit."<<endl;

	unsigned int cmd = 1;

	while(true){
		std::string input,inputL;
		getline(cin,input);
		input = trim(input);
		inputL = aZToLowerCase(input);
		if(input=="") continue;
		if(inputL=="h"||inputL=="help"){
			printHelp();
			continue;
		}
		else if(inputL=="q"){
			break;
		}
		else if(startsWith(input,">")){
			continue;
		}
		else if(startsWith(input,"-")){
			if(input=="-"){
				OPT.print();
				continue;
			}
			try{
				OPT.update(input);
			}
			catch(const invalid_argument& e){
				cout << ">Syntax error, could not parse option: "<< e.what() << endl;
			}
			continue;
		}
		auto args = splitWords(input);
		if(args.empty()) continue;
		args[0] = aZToLowerCase(args[0]);
		if(args[0] == "cd"){
			vector<string> args = splitWords(input);
			if(args.size()>=2){
				if(args[1][args[1].size()-1]!='/') args[1] += "/";
				OPT.filelocation = getPath(args[1],OPT.filelocation);
				cout << ">Changed directory to "<<OPT.filelocation<<"." << endl;
			}
			else{
				cout << ">Syntax error, missing mandatory argument." << endl;
			}
		}
		else if(startsWith(args[0],"#")){
			if(args[0]=="#"){
				if(args.size()>=2) args[0] += args[1];
				if(args.size()>=3) args[1] = args[2];
				if(args.size()>=2) args.pop_back();
			}
			if(args.size()>=2){
				try{
					int macro = getMacroID(args[0]);
					string content = readFileFromPath(getPath(args[1],OPT.filelocation));
					OPT.mm[macro] = content;
				}
				catch(const exception& e){
					cout << ">Failed to add macro: "<<e.what() << endl;
				}
			}
			else{
				cout << ">Syntax error, missing mandatory argument." << endl;
			}
		}
		else{
			try{
				OPT.setline(cmd);
				handleCommand(input,&OPT);
				cmd++;
				if(results.size()>0){
					cout << ">>Results:" << endl;
					for(size_t i = 0; i < results.size(); i++){
						cout << ">>" << results[i] << endl;
					}
				}
				if(OPT.has_data()){
					auto lines = OPT.make_data_results();
					if(!lines.empty()) cout << ">>Statistics:" << endl;
					for(int l = 0; l < lines.size(); l++){
						cout << ">>" << lines[l] << endl;
					}
				}
				OPT.reset_data();
				results.clear();
			}
			catch(const exception& e){
				string msg = e.what();
				std::cout << ">Error running command: "<<msg<<std::endl;
				if(OPT.has_data()){
					auto lines = OPT.make_data_results();
					if(!lines.empty()) cout << ">>Statistics:" << endl;
					for(int l = 0; l < lines.size(); l++){
						cout << ">>" << lines[l] << endl;
					}
				}
				OPT.reset_data();
				results.clear();
				}
		}
	}

	return 0;
}