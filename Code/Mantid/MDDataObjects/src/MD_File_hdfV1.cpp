#include "MDDataObjects/MD_File_hdfV1.h"
#include "MDDataObjects/MDDataPoint.h"

namespace Mantid{
namespace MDDataObjects{

//
MDPointDescription 
MD_File_hdfV1::read_pointDescriptions(void)const
{
	return Mantid::MDDataObjects::MDPointDescription();
}

std::string MD_File_hdfV1::getFileName() const
{
  throw std::runtime_error("This type has not been properly implemented.");
}

} // end namespaces;
}