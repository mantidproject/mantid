#ifndef XMLnamespace_h
#define XMLnamespace_h

/*!
  \namespace XML
  \version 1.0
  \author S. Ansell
  \date December 2006
  \brief Holds the XML namespace objects
*/

namespace XML
{

int cutString(std::string&,std::string&);
std::vector<std::string> getParts(const std::string& KeyList);
int getFilePlace(std::istream&,const std::string&);
int getNumberIndex(const std::multimap<std::string,int>&,
		   const std::string&);
std::pair<int,std::string>  procKey(const std::string&);

int procGroupString(const std::string&,std::string&,std::vector<std::string>&); 

int getNextGroup(std::istream&,std::string&,std::vector<std::string>&); 
int collectBuffer(std::istream&,std::vector<std::string>&); 
int splitGroup(std::istream&,std::string&,
	       std::vector<std::string>&,std::string&);
 
int splitComp(std::istream&,std::string&,std::string&);
int splitLine(std::istream&,std::string&,std::string&);
int getNextGroup(std::istream&,std::string&,std::vector<std::string>&); 
int getGroupContent(std::istream&,std::string&,std::vector<std::string>&,std::vector<std::string>&);
int splitAttribute(std::string&,std::string&,std::string&);
std::string procString(const std::string&);

 int matchPath(const std::string&,const std::string&);

};

#endif
