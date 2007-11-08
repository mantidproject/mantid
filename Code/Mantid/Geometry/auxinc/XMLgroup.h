#ifndef XMLgroup_h
#define XMLgroup_h

namespace XML
{


/*!
  \class XMLgroup
  \brief XML container object
  \author S. Ansell
  \date February 2006
  \version 1.0
  
  Contains a set of XMLobjects

*/

class XMLgroup : public XMLobject 
{
 public:

  typedef std::multimap<std::string,int> holdType;
    
 private:

  std::vector<XMLobject*> Grp;          ///< Orderd list of Objects
  holdType Index;                       ///< Map for searching for an object

  int countKey(const std::string&) const;

 public:

  XMLgroup(XMLobject*);
  XMLgroup(XMLobject*,const std::string&);
  XMLgroup(XMLobject*,const std::string&,const int);
  XMLgroup(const XMLgroup&);
  virtual XMLgroup* clone() const;
  XMLgroup& operator=(const XMLgroup&);
  virtual ~XMLgroup();

  const holdType& getMap() const { return Index; }

  XMLobject* getItem(const int) const;
  XMLobject* getObj(const std::string&,const int =0) const;
  XMLgroup* getGrp(const std::string&,const int =0) const;
  // Find:: (deep-search) 
  XMLobject* findObj(const std::string&,const int =0) const;
  XMLobject* getLastObj() const;
  template<typename T>
  T* getType(const int =0) const;
  template<typename T>
  T* getLastType(const int =0) const;

  template<typename T> int addComp(const std::string&,const T&);
  int addComp(const std::string&,const XMLobject*);
  int addManagedObj(XMLobject*);
  XMLgroup* addGrp(const std::string&);
  void deleteGrp();
  int deleteObj(XMLobject*);

  const XMLobject* findParent(const XMLobject*) const;
  

  std::vector<XMLobject*>::iterator begin() { return Grp.begin(); }
  std::vector<XMLobject*>::iterator end() { return Grp.end(); }

  virtual int readObject(std::istream&);
  virtual void writeXML(std::ostream&) const;
  
};

};   /// NAMESPACE

#endif

