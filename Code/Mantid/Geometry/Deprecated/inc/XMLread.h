#ifndef XML_XMLread_h
#define XML_XMLread_h

namespace Mantid
{

namespace XML
{

/*!
  \class XMLread
  \brief Hold an XML input before specilization
  \author S. Ansell
  \date May 2007
  \version 1.0
  
  This holds the input in a vector of strings

*/

class XMLread : public XMLobject
{
 public:

  typedef std::list<std::string> CStore;

 private:

  
  int empty;
  CStore Comp;         ///< list of read components

 public:

  
  XMLread(XMLobject*,const std::string&);
  XMLread(XMLobject*,const std::string&,
	  const std::vector<std::string>&);
  XMLread(const XMLread&);
  virtual XMLread* clone() const;       ///< virtual constructor
  XMLread& operator=(const XMLread&);     
  virtual ~XMLread();

  void setEmpty() { empty=0; }           ///< Accessor for empty
  int isEmpty() const { return empty; }  ///< Accessor for empty

  void addLine(const std::string&);          
  void setObject(const std::vector<std::string>&);

  const std::string& getFront() const;
  std::string& getFront();
  std::string getFullString() const;

  int pop();
  CStore::const_iterator begin() const { return Comp.begin(); }
  CStore::const_iterator end() const { return Comp.end(); }
  

  // Conversions (using StrFunc)
  template<template<typename T,typename A> class V,typename T,typename A> 
  int convertToContainer(V<T,A>&) const;

  // Containers with forgotten junk!
  template<template<typename T,typename A> class V,typename T,typename A> 
  int convertToContainer(const int ,V<T,A>&,V<T,A>&) const;

  template<typename T> 
  int convertToObject(T&) const;

  int readObj(std::istream&,std::string&);
  void writeXML(std::ostream&) const;

};

std::ostream& operator<<(std::ostream&,const XMLread&);

}   // NAMESPACE XML

}   // NAMESPACE Mantid

#endif
