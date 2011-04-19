#ifndef FILE_HDF_HORACE_READER_H
#define FILE_HDF_HORACE_READER_H
//

#include <iostream>
#include <fstream>
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
	namespace HoraceReader{

/// horace data locations in bytes from the beginning of the horace binary file
/// some have fixed positions but most not.
struct data_positions{
	std::streamoff  if_sqw_start,
	                n_dims_start,
	                sqw_header_start;
	std::vector<std::streamoff> component_headers_starts;
	std::streamoff detectors_start,
		data_start,
		geom_start,
		npax_start,
		s_start,
		err_start,
		n_cell_pix_start,
		min_max_start,
		pix_start;
	data_positions():if_sqw_start(18),n_dims_start(22),sqw_header_start(26),
		detectors_start(0),data_start(0),geom_start(0),s_start(0), // the following values have to be identified from the file itself
		err_start(0),
		n_cell_pix_start(0),min_max_start(0),pix_start(0){}; // the following values have to be identified from the file itself
};
//
class DLLExport MD_FileHoraceReader :    public IMD_FileFormat
{  
public:
	MD_FileHoraceReader(const char *file_name);

	virtual bool is_open(void)const{return fileStreamHolder.is_open();}

	/// read the part of MD dataset containing the basis; Current implementations allocate basis from what they are reading
	// no basis exists in Horace dataset so this funtion initialises default basis qx qy qz
	virtual void read_basis(Mantid::Geometry::MDGeometryBasis &);

	//****> MD image object
	/// reads the MD geometry description, which allows to build MD geometry from the description and the basis;
	virtual void read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &);
   // read DND object data; Image has to exist and be initiated;
    virtual void read_MDImg_data(MDImage & mdd);
   

	//****> datapoints object
	/// read the description of the data points format and (possibly) service information to calculate the pixel location;
	/// TODO: identify the service information for pixels and if we should read it here; Currently it returns things related to point only
	virtual Mantid::MDDataObjects::MDPointDescription read_pointDescriptions(void)const;

	/// read whole pixels information in memory; usually impossible,  returns false if nothrow or throws bad_alloc if nothrow=true;
	virtual bool read_pix(MDDataPoints &, bool nothrow=false);
	/// read the information from the data pixels, specified by the numbers of selected cells, returns the number of cells actually processed 
    /// by this read operation and number of pixels found in these cells;
    virtual size_t read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer);
    /// get number of data pixels(points) contributing into the dataset;
    virtual uint64_t getNPix(void);
    /// not implemented and probably will not be as we will develop our own mdd_hdf format
    virtual void write_mdd(const MDImage &)
    {
      throw(Kernel::Exception::NotImplementedError("write_mdd-Horace format function is not supported and should not be used"));
    }
    
	virtual ~MD_FileHoraceReader(void);

	// private, but protected for test purposes;
protected:
 	/// the variable which keeps the opened Horace data stream
	std::ifstream fileStreamHolder;
	/// the structure holding positions for all important parts of the file
	data_positions positions;
	/// Various Horace data field which are initiated by constructor but requested by our IO operations
	unsigned int nDims; //< number of dimensions; Horace understands 4 and 3 reciprocal so should be 4
	/// number of bins in every non-integrated dimension
	std::vector<size_t> nBins;
	// size of the multidimensional image in the HDD file (in cells)
	size_t     mdImageSize;
	/// number of data points (pixels) contributing into the MD image and present in the file;
	uint64_t     nDataPoints;

///**** auxilary functions dealing with different parts of Horace file;
	/// skips main sqw header, calculates its size and reads the number of contributing files and sets the location of first contributed
	/// file header
	void parse_sqw_main_header();
	/// reads one component header and returns the locations for the next header or part of the file
	std::streamoff parse_component_header(std::streamoff start_location);
	/// analyse  detectors data (or their length)
	std::streamoff parse_sqw_detpar(std::streamoff detectors_start);
	/// 
	void  parse_data_locations(std::streamoff data_start);
private:
	/// horace data are written on HDD as 9 double words; this function compress them into
	/// four float, two double and 3 uint16_t;
	void compact_hor_data(char *buffer,size_t &buf_size);
	// the size of data block for Horace reader;
    mutable unsigned int hbs;
    // the array specifying the locations of MD points wrt MD cells;
    std::vector<uint64_t> hor_points_locations;

	// Function performing work previously in GOTO statement.
    void inline validateFileStreamHolder(std::ifstream& fileStreamHolder);
 
};



} // end namespaces
} 
}
#endif
