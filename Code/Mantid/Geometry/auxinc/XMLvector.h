#ifndef XMLvector_h
#define XMLvector_h

namespace XML
{
  /*!
    \class XMLvector
    \version 1.0
    \author S. Ansell
    \date August 2007
    \brief Holds a vector pair
  */

template<template<typename T> class V,typename T> 
class XMLvector : public XMLobject
{
 private:
  
  int empty;          ///< Null value
  V<T> X;            ///< X values
  V<T> Y;            ///< Y values

 public:

  XMLvector(XMLobject*);
  XMLvector(XMLobject*,const std::string&);
  XMLvector(XMLobject*,const std::string&,const V<T>&,const V<T>&);
  XMLvector(const XMLvector<V,T>&);
  XMLvector<V,T>* clone() const;
  XMLvector<V,T>& operator=(const XMLvector<V,T>&);
  ~XMLvector();

  void setEmpty() { empty=0; } 
  int isEmpty() const { return empty; }

  void setComp(const V<T>&,const V<T>&);    ///< Assumes copy setting
  virtual void writeXML(std::ostream&) const;    
  virtual int readObject(std::istream&);
  V<T>& getX() { return X; }
  V<T>& getY() { return Y; }
  const V<T>& getX() const { return X; }
  const V<T>& getY() const { return Y; }
  
};

}        // NAMESPACE XML

#endif
