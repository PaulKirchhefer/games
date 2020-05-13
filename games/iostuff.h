#pragma once
#include "options.h"
#include <string>
#include <games.h>
#include "Automata.hpp"
#include "Parser.hpp"
#include "tests.h"
#include <chrono>

//contains exclusively IO code, such as parsing commands or files


//transforms a macro of the form '#<index>' to the int <index>
//throws invalid_argument if input doesnt start with '#' or index can not be parsed or is not an int
int getMacroID(const std::string ID);

//updates the given macromap
//line contains the macro definition header, '#'<index>:'
//test is a StringParser that has just read the header and will be used to read the definition content of the form '<content>#'
//a valid macro definition is thus of the form '#<index>:/n<content>#/n'
void updateMacroMap(const std::string line, StringParser& text, macromap& mm);

//prints available commands and usage
void printHelp();

//builds the automaton either by macro or path
//caller is responsible for deleting the result
ExplicitAutomaton* getAut(const std::string arg, const std::string filelocation, const macromap& mm);

//looks up the macro in the macromap and decodes the resulting string into an automaton
//throws invalid_argument if macro is not valid or macro content is not a valid automaton
//caller is responsible for deleting the result
ExplicitAutomaton* macroToAut(const std::string macro, const macromap& mm);

//reads the file at the given absolute path and builds the automaton given by the file
//throws runtime_error if file could not be read and invalid_argument if file content is not a valid automaton
//caller is responsible for deleting the result
ExplicitAutomaton* fileToAut(const std::string path);

//parses the state list given by arg
//throws invalid_argument if arg is not a valid state list
std::vector<state> getStateList(const std::string arg, const macromap& mm);

//Commands:
//do <file>
//starheight <regex>
//starheight <dfa file>
//finitepower <regex>
//finitepower <nfa file>
//shortcut:
//sh = starheight
//fp = finitepower
//throws invalid_argument if command or arguments could not be parsed
//throws runtime_error if file could not be opened
//outputs result messages if verbose is true
//otherwise outputs '>' followed by the result
void handleCommand(std::string cmd, Options* opt);

//handles content of file at given relative path
//throws runtime_error if file could not be opened
void handleFilePath(std::string path, Options* opt);

//ignores faulty commands
//outputs error messages for faulty commands if verbose is true
//throws invalid_argument if a {}-block is not closed
//expects OPT.filelocation to be set correctly
void handleFile(std::string fileContent, Options* opt);