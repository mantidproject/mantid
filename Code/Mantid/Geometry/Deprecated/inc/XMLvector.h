#ifndef XMLvector_h
#define XMLvector_h

namespace Mantid
{

namespace XML
{
  /*!
    \class XMLvector
    \version 1.0
    \author S. Ansell
    \date August 2007
    \brief Holds a vector pair
  */

template<template<typename T,typename A> class V,typename T,typename A> 
class XMLvector : public XMLobject
{
 private:
  
  int empty;          ///< Null value
  V<T,A> X;            ///< X values
  V<T,A> Y;            ///< Y values

 public:

  XMLvector(XMLobject*);
  XMLvector(XMLobject*,const std::string&);
  XMLvector(XMLobject*,const std::string&,const V<T,A>&,const V<T,A>&);
  XMLvector(const XMLvector<V,T,A >&);
  XMLvector<V,T,A>* clone() const;
  XMLvector<V,T,A>& operator=(const XMLvector<V,T,A>&);
  ~XMLvector();

  void setEmpty() { empty=0; } 
  int isEmpty() const { return empty; }

  void setComp(const V<T,A>&,const V<T,A>&);    ///< Assumes copy setting
  virtual void writeXML(std::ostream&) const;    
  virtual int readObject(std::istream&);
  V<T,A>& getX() { return X; }
  V<T,A>& getY() { return Y; }
  const V<T,A>& getX() const { return X; }
  const V<T,A>& getY() const { return Y; }
  
};

}        // NAMESPACE XML

}        // NAMESPACE Mantid
#endif
