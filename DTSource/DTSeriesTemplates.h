// Part of DTSource. Copyright 2006. David Adalsteinsson.
// see http://www.visualdatatools.com/DTSource/license.html for more information.

#include <map>
#include <algorithm>

template <class T,class TIterator>
void DTRegisteredSeries(map<string,T> &S,string name,string type,const T *series)
{
    TIterator searchResult = S.find(name);
    if (searchResult==S.end()) {
        S[name] = *series;
    }
    else {
        DTErrorMessage(type+"::Register","Already a series with this name.");
    }
}

template <class T,class TIterator>
void DTPrintRegisteredSeries(map<string,T> &S)
{
    TIterator iterator = S.begin();
    while (iterator!=S.end()) {
        std::cerr << iterator->first << " : ";
        iterator->second.pinfo();
        iterator++;
    }
}

template <class T,class TIterator>
bool DTHasRegisteredSeries(map<string,T> &S,string name)
{
    TIterator searchResult = S.find(name);
    return (searchResult!=S.end());
}

template <class T,class TIterator>
T DTFindRegisteredSeries(map<string,T> &S,string name,string type)
{
    TIterator searchResult = S.find(name);
    
    T toReturn;
    if (searchResult==S.end()) {
        DTErrorMessage(type+"::ByName","No series with this name.");
    }
    else {
        toReturn = searchResult->second;
    }
    
    return toReturn;
}
