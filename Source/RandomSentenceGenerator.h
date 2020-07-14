#pragma once
#include <iostream>
#include <string>

#include "Grammar.h"


class RandomSentenceGenerator{
private:
    Grammar grammar;
public:
    RandomSentenceGenerator(const std::string & filename);
    RandomSentenceGenerator(std::istream & inputstream); //the constructor
    
    bool readGrammar(std::istream & inputstream);
    
    std::string randomSentence(); //returns a random sentence generated from the grammar
    std::string randomSentence( std::string );
    void replaceRule(std::string *s, std::string lhs, std::string sub);
    std::string getRule(std::string);
    std::string toLwer(std::string);
    void capFirst(std::string &s);
    void printGrammar(); 
    
    bool capEveryWord;
};
