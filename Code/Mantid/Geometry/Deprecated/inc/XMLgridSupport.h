#ifndef XMLGridSupport_h
#define XMLGridSupport_h

namespace Mantid
{

namespace XML
{
 
void combineGrid(XML::XMLcollect&,const std::string&,const int =10);
void combineDeepGrid(XML::XMLcollect&,const std::string&,const int =10);
 
}

}
#endif
