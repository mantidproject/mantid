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

MD_File_hdfV1::MD_File_hdfV1(const char  *file_name):
IMD_FileFormat(file_name)
{
	
}


} // end namespaces;
}