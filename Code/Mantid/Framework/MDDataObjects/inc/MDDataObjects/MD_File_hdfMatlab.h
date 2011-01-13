#ifndef FILE_HDF_MATLAB_ND_H
#define FILE_HDF_MATLAB_ND_H
//
//#include <../../../Third_Party/include/hdf5/hdf5.h>
#include <hdf5.h>
#include "MDDataObjects/IMD_FileFormat.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MDDataObjects/MDDataPoints.h"

// TEMPORARY COMPARTIBILITY FUNCIONS with HDF1.6 -- should be enabled if HDF1.6 is used
// #define HDF_1_6

/**    Class supports MATLAB-written hdf5 mdd data format and will be used at the initial stage of the development;
*      to read the data initially provided by MATLAB, Horace

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
//******************************************************************************************************************
// couple of structures and MATPAB compartibility functions defined here
// MATLAB datatypes which may be written into hdf HORACE file
enum matlab_attrib_kind
{
    double_scalar,
    double_array,
    char_array, // usually string e.g. 1D array of characters
    empty,
    char_cellarray,
    double_cellarray
};

// functions used to understand Horace-written HDF5 file format;
/// structure describing the Horace pixels. It is here for tests and compartibility stuff;
struct sqw_pixel{
    double qx;  //| 
    double qy;  //| 3 Coordinates of each pixel in Q space
    double qz;  //|
    double En;  //| and the pixel enerty 
    double s;   // pixels signal;
    double err; // pixels error (variance i.e. error bar squared)


// this the info about compressed direct picture, which describes the pixel in direct space; It is not used at the moment by Horace
    int    irun; //    Run index in the header block from which pixel came          | these two parameters (or the last one) 
    int    idet; //    Detector group number in the detector listing for the pixel  | convoluted with detectors.par and cristal orientation
                 //    describe 3D picture in Q space and can be the source of additional dimensions if a parameter (e.g. T or pressure)
                 //    changes from run to run
    int    ien ; //    Energy bin number for the pixel in the array in the (irun)th header |-> 4th coordinate dimension
};      
//
class DLLExport MD_File_hdfMatlab :    public IMD_FileFormat
{
public:
    MD_File_hdfMatlab(const char *file_name);

    virtual bool is_open(void)const{return (this->file_handler>0)?true:false;}

	//****> MD baisis object (will be expanded?)
	/// read the part of MD dataset containing the basis; Current implementations allocate basis from what they are reading
	// no basis exists in Horace dataset so this funtion will return defaults
	virtual void read_basis(Mantid::Geometry::MDGeometryBasis &);

	//****> MD image object
	/// reads the MD geometry description, which allows to build MD geometry from the description and the basis;
	virtual void read_MDGeomDescription(Mantid::Geometry::MDGeometryDescription &);
 
    virtual void read_MDImg_data(MDImage & mdd);
   

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
    virtual uint64_t getNPix(void);
    /// not implemented and probably will not be as we will develop our own mdd_hdf format
    virtual void write_mdd(const MDImage & dnd){throw(Kernel::Exception::NotImplementedError("write_mdd-Matlab format function is not supported and should not be used"));}
    

    virtual ~MD_File_hdfMatlab(void);
protected:
     /// the variable which provides access to the open hdf file
    hid_t file_handler;
   /// the variable to access open pixels dataset (necessary for partial read operations)
    hid_t pixel_dataset_h;
   // the variable to deal with pixel dataspace; Especially usefull when dealing with number of partial reading operations;
    hid_t pixel_dataspace_h;
   /// the variable describes file access mode, which is complicated if parallel access is used 
    hid_t file_access_mode;

    /// the vector of DND field names used by the reader/writer
    std::vector<std::string> mdd_field_names;
    ///  the vector of mdd_hdf attributes used by the reader/writer
    std::vector<std::string> mdd_attrib_names;

///  number of fields in HORACE sqw dataset;
    static const int  DATA_PIX_WIDTH=9;

// not used at the moment
//   static std::stringstream ErrBuf;
// private copy constructor and assighnment
   MD_File_hdfMatlab(const MD_File_hdfMatlab& p);
   MD_File_hdfMatlab & operator = (const MD_File_hdfMatlab & other);

   // function checks if pixel dataset is opened and if not opens it. true if it was already opened, false if did nothing
   bool check_or_open_pix_dataset(void);
 
};

// function used to understand Horace written Matlab dataset.
bool read_matlab_field_attr(hid_t group_ID,const std::string &field_name,void *&data, std::vector<int> &dims,int &rank,matlab_attrib_kind &kind,const std::string &file_name);
void ** transform_array2cells(void *data, std::vector<int> dims,int rank,matlab_attrib_kind kind,void *pFiller);
//********************************************************************************************************************************************************************


}
}
#endif
