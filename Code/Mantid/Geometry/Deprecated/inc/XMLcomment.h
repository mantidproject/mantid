#ifndef XMLcomment_h
#define XMLcomment_h

namespace Mantid
{

namespace XML
{

/*!
  \class XMLcomment
  \brief Hold an XML comment
  \author S. Ansell
  \date June 2007
  \version 1.0
  
  This holds the outgoing comment object

*/

class XMLcomment : public XMLobject
{
 public:

  typedef std::list<std::string> CStore;

 private:

  int empty;
  CStore Comp;         ///< list of read components

 public:

  
  XMLcomment(XMLobject*,const std::string&);
  XMLcomment(XMLobject*,const std::vector<std::string>&);
  XMLcomment(XMLobject*,const std::string&,const std::string&);
  XMLcomment(XMLobject*,const std::string&,const std::vector<std::string>&);
  XMLcomment(const XMLcomment&);
  virtual XMLcomment* clone() const;       ///< virtual constructor
  XMLcomment& operator=(const XMLcomment&);     
  virtual ~XMLcomment();

  void setEmpty() { empty=0; }           ///< Accessor for empty
  int isEmpty() const { return empty; }  ///< Accessor for empty

  void addLine(const std::string&);          
  void setObject(const std::vector<std::string>&);

  const std::string& getFront() const;
  std::string& getFront();
  int pop();
  CStore::const_iterator begin() const { return Comp.begin(); }
  CStore::const_iterator end() const { return Comp.end(); }


  void writeXML(std::ostream&) const;

};

std::ostream& operator<<(std::ostream&,const XMLcomment&);

} // NAMESPACE XML

} // NAMESPACE Mantid

#endif
