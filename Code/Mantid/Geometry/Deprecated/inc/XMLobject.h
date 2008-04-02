#ifndef XMLobject_h
#define XMLobject_h

#include "XMLattribute.h"

namespace Mantid
{

namespace XML
{

/*!
  \class nullObj
  \brief Null object act as a template place holder
  \author S. Ansell
  \date February 2006
  \version 1.0
*/
class nullObj
{};


/*!
  \class XMLobject
  \brief Abstract XMLitem class
  \author S. Ansell
  \date February 2006
  \version 1.0
  
  Top level object which holds its depth and
  its keyname.

*/

class XMLobject
{
 protected:

  int depth;                ///< Indent level of the XML 
  int loaded;               ///< Flag to indicate read in
  int repNumber;            ///< Repeat number [0 none : 1 -N ]
  std::string Key;          ///< XML key to this object
  XMLattribute Attr;        ///< Attribute list
  XMLobject* Base;          ///< Base group

 public:
  
  XMLobject(XMLobject*);
  XMLobject(XMLobject*,const std::string&);
  XMLobject(XMLobject*,const std::string&,const int);
  XMLobject(const XMLobject&);
  virtual XMLobject* clone() const;       ///< virtual constructor
  XMLobject& operator=(const XMLobject&);     
  virtual ~XMLobject() { }

  //  void addAttribute(const std::string&,const std::string&);
  void addAttribute(const std::vector<std::string>&);
  void addAttribute(const std::string&,const char*);
  template<typename T> void addAttribute(const std::string&,const T&);

  int setAttribute(const std::string&,const std::string&);
  int hasAttribute(const std::string&) const;
  std::string getAttribute(const std::string&) const;

  /// sets the depth of indentation
  void setDepth(const int A) { depth=A; }
  void writeDepth(std::ostream&) const;
  /// write out object (invalid for base object)

  const std::string& getKey() const { return Key; }
  std::string getFullKey() const;
  std::string getKeyBase() const;
  int getKeyNum() const;
  int getRepNum() const { return repNumber; }
  void setRepNum(const int RN) { repNumber=RN; }

  XMLobject* getParent() const { return Base; }
  void setParent(XMLobject* B) { Base=B; }

  virtual void setEmpty() { } 
  virtual int readObject(std::istream&);

  std::string getCurrentFile(const int =1) const;
  virtual void writeXML(std::ostream&) const 
    {}      

  template<typename T> 
  const T& getValue(const T&) const;
};

std::ostream& operator<<(std::ostream&,const XMLobject&);

}   /// NAMESPACE XML

}   /// NAMESPACE Mantid

#endif
