#ifndef XMLgrid_h
#define XMLgrid_h

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

template<template<typename T,typename A> class V,typename T,typename A> 
class XMLgrid : public XMLobject
{
 private:

  int size;                   ///< Current size
  int empty;                  ///< Null value
  std::vector< V<T,A> > Grid;   ///< X values
  int contLine;

 public:

  XMLgrid(XMLobject*);
  XMLgrid(XMLobject*,const std::string&);
  XMLgrid(const XMLgrid<V,T,A>&);
  XMLgrid<V,T,A>* clone() const;
  XMLgrid<V,T,A>& operator=(const XMLgrid<V,T,A>&);
  ~XMLgrid();

  void setEmpty() { empty=0; } 
  int isEmpty() const { return empty; }
  int getSize() const { return (!empty) ? size : 0; }   
  void setContLine(const int IFlag) { contLine=IFlag; }

  void setComp(const int,const V<T,A>&);    ///< Assumes copy setting
  virtual void writeXML(std::ostream&) const;    
  virtual int readObject(std::istream&);

  V<T,A>& getGVec(const int);
  const V<T,A>& getGVec(const int) const;
  
};

}  // NAMESPACE XML


} // NAMESPACE Mantid

#endif
