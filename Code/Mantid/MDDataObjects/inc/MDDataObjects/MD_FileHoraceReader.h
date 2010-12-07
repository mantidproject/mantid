#ifndef FILE_HDF_HORACE_READER_H
#define FILE_HDF_HORACE_READER_H
//

#include "MDDataObjects/IMD_FileFormat.h"


/**  Class supports Horace(MATLAB)-written binary mdd data format and will be used at the initial stage of the development;
*    to read the data initially provided by MATLAB, Horace

    @author Alex Buts, RAL ISIS
    @date 01/10/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>

*/

namespace Mantid{
namespace MDDataObjects{

//
class DLLExport MD_FileHoraceReader :    public IMD_FileFormat
{  
public:
	MD_FileHoraceReader(const char *file_name);

    virtual bool is_open(void)const;

	//****> MD baisis object (will be expanded?)
	/// read the part of MD dataset containing the basis; Current implementations allocate basis from what they are reading
	// no basis exists in Horace dataset so this funtion will return defaults
	virtual void read_basis(Mantid::Geometry::MDGeometryBasis &);

	//****> MD image object
	/// reads the MD geometry description, which allows to build MD geometry from the description and the basis;
	virtual void read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &);
   // read DND object data;
    virtual void read_mdd(MDImage & mdd);
   

	//****> datapoints object
	/// read the description of the data points format and (possibly) service information to calculate the pixel location;
	/// TODO: identify the service information for pixels and if we should read it here; Currently it returns things related to point only
	virtual Mantid::MDDataObjects::MDPointDescription read_pointDescriptions(void)const;
    /// read whole pixels information in memory; usually impossible, then returns false;
    virtual bool read_pix(MDDataPoints & sqw);
    /// read the information from the data pixels, specified by the numbers of selected cells, returns the number of cells actually processed 
    /// by this read operation and number of pixels found in these cells;
    virtual size_t read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer);
    /// get number of data pixels(points) contributing into the dataset;
    virtual size_t getNPix(void);
    /// not implemented and probably will not be as we will develop our own mdd_hdf format
    virtual void write_mdd(const MDImage & dnd){throw(Kernel::Exception::NotImplementedError("write_mdd-H?orace format function is not supported and should not be used"));}
    
	virtual ~MD_FileHoraceReader(void){};
protected:
    /// name of a file which keeps mdd dataset;
    std::string File_name;


};

} // end namespaces
}
#endif
