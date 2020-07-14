#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>


typedef std::map<std::string, std::vector<std::string> > myMap;


class Grammar{
    friend class RandomSentenceGenerator;
private:
    myMap grammarMap;
    
public:
    Grammar(); //the constructor
    
    void clear();
    
    void addProduction(const std::string & nonTerm, const std::string & rhs);
    std::string getRandomRHS(const std::string & nonTerm);
    bool containsNonTerminal(const std::string & nonTerm);
    
    friend std::ostream &operator<<(std::ostream & , const Grammar &);  
    
};
