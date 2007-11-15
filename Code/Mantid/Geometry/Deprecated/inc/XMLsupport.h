#ifndef XMLsupport_h
#define XMLsupport_h

namespace Command
{
  class CommandObj;
}

namespace XML
{
  
  /*!
    \class mapXML
    \version 1.0
    \brief Support for writing out stuff
    \author S. Ansel
  */

class mapXML
{
 private:
  
  XMLcollect& AX;

 public:
 
  mapXML(XMLcollect& AX);
 
  void operator()(const std::pair<std::string,
		  Command::CommandObj*>&);

};


}   // NAMESPACE XML

#endif
