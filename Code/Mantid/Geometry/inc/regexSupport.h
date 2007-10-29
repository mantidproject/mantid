#ifndef regexSupport_h
#define regexSupport_h

namespace StrFunc
{
/// Find if a pattern matches a string
template<typename T> int StrComp(const char*,const boost::regex&,T&,const int=1);

/// Find if a pattern matches
template<typename T> int StrComp(const std::string&,const boost::regex&,T&,const int=0);

/// Find is a pattern matches
int StrLook(const char*,const boost::regex&);
/// Find is a pattern matches
int StrLook(const std::string&,const boost::regex&);

/// Split  a line into component parts
std::vector<std::string> 
StrParts(std::string,const boost::regex&);

/// Split  a line searched parts
template<typename T> int StrFullSplit(const std::string&,const boost::regex&,std::vector<T>&);
template<typename T> int StrFullSplit(const char*,const boost::regex&,std::vector<T>&);
template<typename T> int StrSingleSplit(const std::string&,const boost::regex&,std::vector<T>&);
template<typename T> int StrSingleSplit(const char*,const boost::regex&,std::vector<T>&);

/// Cut out the searched section and returns component
template<typename T> int StrFullCut(std::string&,const boost::regex&,T&,const int= -1);
template<typename T> int StrFullCut(std::string&,const boost::regex&,std::vector<T>&);

/// Extract a section from a string
int StrRemove(std::string&,std::string&,const boost::regex&);

/// Find a compmonent in a Regex in a file
template<typename T> int findComp(std::istream&,const boost::regex&,T&);

/// Finds a pattern in a file
template<typename T> int findPattern(std::istream&,const boost::regex&,T&,std::string&);

};

#endif
