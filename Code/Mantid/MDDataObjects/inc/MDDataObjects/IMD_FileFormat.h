#ifndef H_FILE_FORMAT
#define H_FILE_FORMAT

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <hdf5.h>
#include <vector>
#include "MantidKernel/Logger.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"

/** interface to various MDData file formats (and may be other parts of MD-workspace file in a future);

    @author Alex Buts, RAL ISIS
    @date 28/09/2010

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
// forward declaration of classes, contributing into MD workspace which should be read from file;
class MDImage;
class MDPointDescription;
class MDDataPoints;
//
class Mantid::Kernel::Logger;

/** class describes the interface to file operations, which are supported by generic MD dataset
*/
class IMD_FileFormat
{
public:
    IMD_FileFormat(void){};
	/// is the file with underlying MDWorkspace is open;
    virtual bool is_open(void)const{return false;}

	//****> MD baisis object (will be expanded?)
	/// read the part of MD dataset containing the basis; Current implementations allocate basis from what they are reading
	///TODO: should be probably MDGeometryBasisDescription, which later used to build the geometry basis from the description;
	virtual void read_basis(Mantid::Geometry::MDGeometryBasis &)=0;

	//****> MD image object
	/// reads the MD geometry description, which allows to build MD geometry from the description and the basis;
	virtual void read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &)=0;
    /// reads the data part of the MD Image 
    virtual void read_mdd(MDImage &)=0; 

	//****> datapoints object
	/// read the description of the data points format and (possibly) service information to calculate the pixel location;
	/// TODO: identify the service information for pixels and if we should read it here; Currently it returns things related to point only
	virtual Mantid::MDDataObjects::MDPointDescription read_pointDescriptions(void)const=0;
    /// tries to read MDDataPoint (pixels) part of the dataset into memory. Usually impossible for TOF instruments but may be the best method for 3-pl axis
    virtual bool read_pix(MDDataPoints &)=0; 
    /// read part of the dataset, specified by the vector of MDImage cell numbers. 
    virtual size_t read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)=0; 
    /// obtain the number of data points (pixels) stored in the dataset;
    virtual hsize_t getNPix(void)=0;

//
    virtual void write_mdd(const MDImage &)=0;
//  virtual void write_pix(const MDDataPoints &)=0;
//  virtual void write_pix_subset(const std::vector<size_t> &cell_indexes, const str::vector<char> &pix_buf)=0;
    virtual ~IMD_FileFormat(void){};
protected: 

     /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& f_log;

};

} // MDDataObjects
} // Mantid;
#endif