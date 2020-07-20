#include "Grammar.h"

#include <cstdlib>
#include <time.h>

using namespace std;

/**
 * The constructor. Creates an empty grammar map.
 **/
Grammar::Grammar(){
    grammarMap.clear();
    srand((unsigned int)time(NULL));
}

void Grammar::clear()
{
    grammarMap.clear();
}


/**
 *  This method takes the non-terminal lefthandside 
 *  and one​ of the right hand sides of the production, and adds it to the
 *  data structure.
 **/
void Grammar::addProduction(const string & nonTerm,  const string & rhs)
{
    if (!containsNonTerminal(nonTerm)){
        vector<string> sentences(1, rhs);
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
string Grammar::getRandomRHS(const string & nonTerm)
{
    int sz = (int) grammarMap[nonTerm].size();
    if (sz == 0){
        return "";
    }
    return grammarMap[nonTerm].at(rand()%sz);
}

/**
 * Returns whether or not the grammar
 * has a rule with the given non-terminal as its left hand side.
 **/
bool Grammar::containsNonTerminal(const string & nonTerm)
{
    if (grammarMap.find(nonTerm) == grammarMap.end()){
        return false;
    }
    return true;
}
       
/**
 * Overloaded ostream operator
 **/
ostream &operator<<(ostream &out, const Grammar &g)
{
    myMap::const_iterator it;
    
    //iterate through the map
    for (it = g.grammarMap.begin(); it != g.grammarMap.end(); it++) {
        out << "key:  '" << it->first << "'" << endl;
        vector<string>::const_iterator rhs = (it->second).begin();
        while (rhs != (it->second).end())
        {
            out << *rhs << " ;" << endl;
            rhs++;
        } //end inner loop
    }//end outer loop
        
    return out;
}
   
