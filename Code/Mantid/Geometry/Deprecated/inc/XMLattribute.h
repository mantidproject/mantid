#ifndef XMLattribute_h
#define XMLattribute_h

namespace XML
{

/*!
  \class XMLattribute
  \brief atribute for XMLobjects
  \author S. Ansell
  \date December 2006
  \version 1.0
  
  Has a simple string base (currently)
*/

class XMLattribute
{
 private:
  
  int empty;                        ///< flag on string
  std::vector<std::string> AObj;    ///< Key lies
  std::vector<std::string> Val;     ///< Values 

 public:

  XMLattribute();
  XMLattribute(const std::string&,const std::string&);
  XMLattribute(const XMLattribute&);
  XMLattribute& operator=(const XMLattribute&);
  ~XMLattribute();

  void addAttribute(const std::string&,const std::string&);
  void addAttribute(const std::vector<std::string>&);
  int setAttribute(const std::string&,const std::string&);
  int hasAttribute(const std::string&) const;
  std::string getAttribute(const std::string&) const;
  void writeXML(std::ostream&) const;    
  
};

std::ostream& operator<<(std::ostream&,const XMLattribute&);

}  // NAMESPACE

#endif
