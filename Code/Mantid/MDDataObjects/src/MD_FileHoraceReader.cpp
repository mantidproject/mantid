#include "MDDataObjects/MD_FileHoraceReader.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDGeometryBasis.h"


namespace Mantid{
namespace MDDataObjects
{
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

MD_FileHoraceReader::MD_FileHoraceReader(const char *file_name):
File_name(file_name)
{
}

bool 
MD_FileHoraceReader::is_open(void)const
{
 return false;
}


//
void 
MD_FileHoraceReader::read_basis(Mantid::Geometry::MDGeometryBasis &)
{
}
//
void 
MD_FileHoraceReader::read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &)
{
}
   // read DND object data;
void 
MD_FileHoraceReader::read_mdd(MDImage & mdd)
{
}
   
MDPointDescription 
MD_FileHoraceReader::read_pointDescriptions(void)const
{
	MDPointDescription defaultDescr;
	return defaultDescr;
}
 //read whole pixels information in memory; usually impossible, then returns false;
bool 
MD_FileHoraceReader::read_pix(MDDataPoints & sqw)
{
	return false;
}
//
size_t 
MD_FileHoraceReader::read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
{
	return 0;
}
 /// get number of data pixels(points) contributing into the dataset;
size_t 
MD_FileHoraceReader::getNPix(void)
{
	return 0;
}

}// end namespaces
}