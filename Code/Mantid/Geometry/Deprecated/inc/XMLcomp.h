#ifndef XMLcomp_h
#define XMLcomp_h

namespace Mantid
{

namespace XML
{


/*!
  \class XMLcomp
  \brief XML base object
  \author S. Ansell
  \date February 2006
  \version 1.0
  
  Contains any type of XML value.
  Based on the template type T
*/

template<typename T>
class XMLcomp : public XMLobject
{
 private:
  
  int empty;       ///< Null value
  T Value;         ///< An actual value!!  

 public:

  XMLcomp(XMLobject*);
  XMLcomp(XMLobject*,const std::string&);
  XMLcomp(XMLobject*,const std::string&,const T&);
  XMLcomp(const XMLcomp<T>&);
  XMLcomp<T>* clone() const;
  XMLcomp<T>& operator=(const XMLcomp<T>&);
  ~XMLcomp();

  void setEmpty() { empty=0; } 
  int isEmpty() const { return empty; }

  void setComp(const T&);                      ///< Assumes copy setting
  virtual void writeXML(std::ostream&) const;    
  virtual int readObject(std::istream&);
  T& getValue() { return Value; }
  const T& getValue() const { return Value; }
};


std::ostream& operator<<(std::ostream&,const XMLattribute&);

} // NAMESPACE XML

} // NAMESPACE Mantid

#endif
