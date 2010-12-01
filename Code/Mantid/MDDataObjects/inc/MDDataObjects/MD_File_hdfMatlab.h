#ifndef FILE_HDF_MATLAB_ND_H
#define FILE_HDF_MATLAB_ND_H
//
//#include <../../../Third_Party/include/hdf5/hdf5.h>
#include <hdf5.h>
#include "MDDataObjects/IMD_FileFormat.h"
#include "MDDataObjects/MDDataPoints.h"


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
        using namespace Mantid::Kernel;
//
class MD_File_hdfMatlab :    public IMD_FileFormat
{
public:
    MD_File_hdfMatlab(const char *file_name);

    virtual bool is_open(void)const{return (this->file_handler>0)?true:false;}

    virtual void read_mdd(MDImage & mdd);
   
    /// read whole pixels information in memory; usually impossible, then returns false;
    virtual bool read_pix(MDDataPoints & sqw);
    /// read the information from the data pixels, specified by the numbers of selected cells, returns the number of cells actually processed 
    /// by this read operation and number of pixels found in these cells;
    virtual size_t read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer);
    /// get number of data pixels contributing into the dataset;
    virtual hsize_t getNPix(void);
    /// not implemented and probably will not be as we will develop our own mdd_hdf format
    virtual void write_mdd(const MDImage & dnd){throw(Exception::NotImplementedError("write_mdd-Matlab format function is not supported and should not be used"));}
    
    virtual ~MD_File_hdfMatlab(void);
private:
    /// name of a file which keeps mdd dataset;
    std::string File_name;
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
//
}
}
#endif
