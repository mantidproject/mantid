#ifndef XMLcollect_h
#define XMLcollect_h

namespace Mantid
{

namespace XML
{
/*!
  \class XMLcollect 
  \brief XML holder for Spectrum information
  \author S. Ansell
  \version 1.0
  \date December 2006
  
  This class holds an XML schema of any 
  type but normally built against a class.
  It can write out a set of XML information.
  Its key component is Master, which holds the 
  main XML tree. Additionally WorkGrp is used
  as a placement pointer for extra speed

  \todo Need the output buffered
*/

class XMLcollect 
{
 private:

  static Kernel::Logger& PLog;        ///< The official logger  

  std::string depthKey;               ///< Current depth layer
  XMLgroup Master;                    ///< Master group
  XMLgroup* WorkGrp;                  ///< Working group [Never Null]

  std::string makeTimeString(const tm*) const;

 public:

  XMLcollect(); 
  XMLcollect(const XMLcollect&);
  XMLcollect& operator=(const XMLcollect&);
  ~XMLcollect();

  void clear();            

  void addGrp(const std::string&);
  int addNumGrp(const std::string&);
  template<typename T> int addNumComp(const std::string&,const T&);
  int addNumComp(const std::string&,const char*);
  
  // Non-file type One
  template<template<typename T,typename A> class V,typename T,typename A> 
  int addNumComp(const std::string&,const V<T,A>&);
  // Non-file type
  template<template<typename T,typename A> class V,typename T,typename A> 
  int addNumComp(const std::string&,const V<T,A>&,const V<T,A>&);

  template<template<typename T,typename A> class V,typename T,typename A> 
  int addNumComp(const std::string&,
		 const V<T,A>&,const V<T,A>&,const V<T,A>&);
  // file type
  template<template<typename T,typename A> class V,typename T,typename A> 
  int addNumComp(const std::string&,const std::string&,
		 const V<T,A>&,const V<T,A>&);

  template<template<typename T,typename A> class V,typename T,typename A> 
  int addNumComp(const std::string&,const std::string&,
		 const V<T,A>&,const V<T,A>&,const V<T,A>&);

  template<typename T> int addComp(const std::string&,const T&);
  int addComp(const std::string&,const char*);

  // One container
  template<template<typename T,typename A> class V,typename T,typename A> 
  int addComp(const std::string&,const V<T,A>&);

  // Two container
  template<template<typename T,typename A> class V,typename T,typename A> 
  int addComp(const std::string&,const V<T,A>&,const V<T,A>&);

  // Two container + file
  template<template<typename T,typename A> class V,typename T,typename A> 
  int addComp(const std::string&,const std::string&,const V<T,A>&,const V<T,A>&);


  template<template<typename T,typename A> class V,typename T,typename A> 
  int addComp(const std::string&,const V<T,A>&,const V<T,A>&,const V<T,A>&);

  // Three Containers + file
  template<template<typename T,typename A> class V,typename T,typename A> 
  int addComp(const std::string&,const std::string&,
	      const V<T,A>&,const V<T,A>&,const V<T,A>&);

  // Four Containers
  template<template<typename T,typename A> class V,typename T,typename A> 
  int addComp(const std::string&,
	      const V<T,A>&,const V<T,A>&,const V<T,A>&,const V<T,A>&);

  // Attributes
  void addAttribute(const std::string&,const char*);
  template<typename T>
  void addAttribute(const std::string&,const T&);
  void addAttribute(const std::string&,const std::string&,const char*);
  template<typename T>
  void addAttribute(const std::string&,const std::string&,const T&);

  // Add a comment
  void addComment(const std::string&);  
  void addComment(const std::vector<std::string>&);  

  // Get/Find object
  XMLobject* getObj(const std::string&,const int =0) const;
  XMLobject* findObj(const std::string&,const int =0) const;
  XMLgroup* getCurrent() { return WorkGrp; }
  int setToKey(const std::string&,const int =0);

  int getRepeatNumber() const { return WorkGrp->getRepNum(); }

  const XMLobject* findParent(const XMLobject*) const; 
  const XMLgroup* getCurrent() const { return WorkGrp; }

  int deleteObj(XMLobject*);

  void closeGrp();

  void writeXML(std::ostream&) const;

  int readObject(const std::string&);
  int readObject(const std::string&,const std::string&);
  int readObject(std::istream&,const std::string&);

  int loadXML(const std::string&);
  int loadXML(const std::string&,const std::string&);
  int loadXML(std::istream&,const std::string&,
	      const std::vector<std::string>&);


};

}   // NAMESPACE XML

}  // NAMESPACE Mantid

#endif
