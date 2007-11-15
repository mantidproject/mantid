#ifndef Mantid_XML_XMLgrid_h
#define Mantid_XML_XMLgrid_h

namespace Mantid
{

namespace XML
{

  /*!
    \class XMLgrid
    \version 1.0
    \author S. Ansell
    \date August 2007
    \brief The object is to hold a set of containers
   */

template<template<typename T> class V,typename T> 
class XMLgrid : public XMLobject
{
 private:

  int size;                   ///< Current size
  int empty;                  ///< Null value
  std::vector< V<T> > Grid;   ///< X values
  int contLine;

 public:

  XMLgrid(XMLobject*);
  XMLgrid(XMLobject*,const std::string&);
  XMLgrid(const XMLgrid<V,T>&);
  XMLgrid<V,T>* clone() const;
  XMLgrid<V,T>& operator=(const XMLgrid<V,T>&);
  ~XMLgrid();

  void setEmpty() { empty=0; } 
  int isEmpty() const { return empty; }
  int getSize() const { return (!empty) ? size : 0; }   
  void setContLine(const int A) { contLine=A; }

  void setComp(const int,const V<T>&);    ///< Assumes copy setting
  virtual void writeXML(std::ostream&) const;    
  virtual int readObject(std::istream&);

  V<T>& getGVec(const int);
  const V<T>& getGVec(const int) const;
  
};

}  // NAMESPACE XML

}  // NAMESPACE Mantid

#endif
