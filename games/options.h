#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "parser.h"
#include <map>
#include <list>
#include <vector>
#include <stdexcept>

typedef std::map<int,std::string> macromap;

enum Flag{
    DEFAULT, FALSE, TRUE, SETFALSE, SETTRUE
};


enum Data{
    NDET_N, NDET_NMAX, A, NDET_CALLS,
    SPOT_W, SPOT_D, SPOT_R, DET_N, DET_P, SPOT_CALLS,
    GAME_T, GAME_VIS, GAME_VISMAX, GAME_N, GAME_NMAX, GAME_P, GAME_CALLS,
    OINK_W, OINK_S, OINK_R, OINK_CALLS,
    LIM_T, LIM_LIM, LIM_CALLS,
    DATA_SIZE
};

constexpr int MODES = 9;
std::string mode_to_string(int mode);

constexpr bool checkfrom = true;
//constexpr bool checksubset = true;

//contains options that may change how commands are executed
class Options{
    public:
    Flag v,c,o,d,max,log,s;       //verbose output; compare stamina/games output; output aux files; output debug information; use upper sh bound; write data to log; stats
    Flag bighoa;
    int g;

    //nondet: N, NMax, A
    //determ: w, d, r
    //detres: N, P
    //mkgame: t, vis
    //gmeres: N, NMax, P
    //solgme: w, s, r
    std::vector<std::vector<long>> data;
    std::vector<bool> dset;
    long stamtime = 0L;
    long stamcalls = 0L;
    long stamlim = 0L;

    std::vector<bool> testmodes;

    macromap mm;
    std::string filelocation;       //relative paths will be relative to this
    std::string outputdirectory;    //for -o and temp files
    
    unsigned int tests;             //tests (that could be parsed)
    unsigned int passed;            //passed tests

    unsigned int timeout;           //timeout used to kill autfilt/oink

    std::vector<unsigned int> lines;        //lines of commands, used to trace results back to file/command
    std::vector<std::string>* results;      //stores results that will be printed after the last file is done

    Options(std::vector<std::string>* results = NULL);

    bool verbose() const;
    bool compare() const;
    bool output() const;
    bool debug() const;
    bool useAlt() const;
    bool canWrite() const;
    bool useLoopComplexity() const;
    bool logdata() const;
    bool stats() const;
    bool compressHoa() const;

    void writeToLog(std::string data) const;

    void update(const std::string input);
    void setline(const unsigned int line);

    void print() const;

    void reset_data();
    void add_data(Data d, long val);
    bool has_data() const;

    void add_result(std::string result);
    void add_test_results();
    void add_data_results();

    std::string make_prefix() const;
    //file path used to write files
    std::string make_path_prefix(const bool out = false) const;
    std::string make_test_results() const;
    std::vector<std::string> make_data_results() const;
    std::vector<std::string> make_data_results(const int mode) const;

    void set();
};

//used to restore options after return, as the destructor is called by leaving scope
class ResetOPT{

    public:
    Options o;
    Options* r;
    ResetOPT(Options* opt);
    ~ResetOPT();

};