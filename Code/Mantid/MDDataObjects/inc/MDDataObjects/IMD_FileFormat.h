#ifndef H_FILE_FORMAT
#define H_FILE_FORMAT

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <hdf5.h>
#include <vector>
#include "MantidKernel/Logger.h"

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
//
class MDImageData;
class MDWorkspace;
class Mantid::Kernel::Logger;

/** class describes the interface to file operations, which are supported by generic MD dataset
*/
class IMD_FileFormat
{
public:
    IMD_FileFormat(void){};
    virtual bool is_open(void)const{return false;}
    virtual void read_mdd(MDImageData &)=0;
    virtual bool read_pix(MDWorkspace &)=0; 
    virtual size_t read_pix_subset(const MDWorkspace &sqw,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)=0; 
    virtual hsize_t getNPix(void)=0;
    virtual void write_mdd(const MDWorkspace &)=0;
    virtual ~IMD_FileFormat(void){};
protected: 

     /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& f_log;

};
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
bool read_matlab_field_attr(hid_t group_ID,const std::string &field_name,void *&data, std::vector<int> &dims,int &rank,matlab_attrib_kind &kind,const std::string &file_name);
void ** transform_array2cells(void *data, std::vector<int> dims,int rank,matlab_attrib_kind kind,void *pFiller);
} // MDDataObjects
} // Mantid;
#endif