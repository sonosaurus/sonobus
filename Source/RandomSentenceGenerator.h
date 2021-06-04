// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "JuceHeader.h"

typedef std::map<std::string, std::vector<std::string> > GrammarMap;

class Grammar {
    friend class RandomSentenceGenerator;
private:
    GrammarMap grammarMap;
    
public:
    Grammar()  {
        grammarMap.clear();

        rng.setSeedRandomly();
    }
    virtual ~Grammar() {}
    
    void clear() {
        grammarMap.clear();   
    }
    
    /**
    *  This method takes the non-terminal lefthandside 
    *  and one​ of the right hand sides of the production, and adds it to the
    *  data structure.
    **/
    void addProduction(const std::string & nonTerm, const std::string & rhs) {
        if (!containsNonTerminal(nonTerm)){
            std::vector<std::string> sentences(1, rhs);
            grammarMap[nonTerm] = sentences;  
        }else{
            grammarMap[nonTerm].push_back(rhs);
        }
    }
    
    /**
    * This method gets a random sentence and returns it.
    * If there is no right handside it returns an empty string "",
    * this is to assist with recursion.
    **/
    std::string getRandomRHS(const std::string & nonTerm){
        int sz = (int) grammarMap[nonTerm].size();
        if (sz == 0){
            return "";
        }
        return grammarMap[nonTerm].at(abs(rng.nextInt())%sz);
    }
    
    /**
    * Returns whether or not the grammar
    * has a rule with the given non-terminal as its left hand side.
    **/
    bool containsNonTerminal(const std::string & nonTerm) {
        if (grammarMap.find(nonTerm) == grammarMap.end()){
            return false;
        }
        return true;
    }
    
    friend std::ostream &operator<<(std::ostream & , const Grammar &);  
    
    Random rng;

};


class RandomSentenceGenerator{
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

private:
    std::unique_ptr<Grammar> grammar;

};
