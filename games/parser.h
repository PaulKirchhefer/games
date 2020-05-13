#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <list>
#include <chrono>

//contains IO/DEBUG code

enum status{
    VALID,INVALID,ENDOFLINE,ENDOFFILE
};

class StringParser
{
    public:
    const std::string input;
    const size_t length;
    status lastOp;
    size_t next;
    size_t last;

    size_t line;

    StringParser(const std::string input);

    size_t remaining() const;

    std::string advanceLine();

    std::string advanceToChar(const char c, const bool advance=false);

    std::string advanceBlock(const char open, const char close, const bool advance=false);

    bool goBack();

    private:
    size_t lastline;
};

std::string trim(const std::string in);

std::vector<std::string> split(const std::string in, const char c, const bool trimsubstrings=true);

std::vector<std::string> splitWords(const std::string in, const bool trimsubstrings=true);

std::string extract(const std::string in);

bool startsWith(const std::string in, const std::string pattern);

unsigned int count(const std::string in, const char c);

std::string readFile(std::ifstream& file);

std::string readFileFromPath(const std::string absolutepath);

void writeToFile(std::string absolutepath, std::string content, bool append = false);

//makes the absolute path given by a (possibly) relative path and the current filelocation
//if filelocation is a folder it is terminated by '/' otherwise it is considered a file
//resolves leading '.' and '..' in the relative path
std::string getPath(const std::string arg, const std::string filelocation);

std::string took(std::chrono::time_point<std::chrono::steady_clock> start);

std::string took(const long us);

std::string aZToLowerCase(std::string input);
