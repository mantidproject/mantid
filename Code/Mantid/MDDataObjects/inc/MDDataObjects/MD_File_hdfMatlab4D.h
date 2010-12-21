#ifndef FILE_HDF_MATLAB_4D_H
#define FILE_HDF_MATLAB_4D_H
//
#include "MDDataObjects/MD_File_hdfMatlab.h"


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
class DLLExport MD_File_hdfMatlab4D :    public MD_File_hdfMatlab
{
public:
	MD_File_hdfMatlab4D(const char *file_name):MD_File_hdfMatlab(file_name){};

     
    /// read whole pixels information in memory; usually impossible, then returns false;
    virtual bool read_pix(MDDataPoints & sqw);
    /// read the information from the data pixels, specified by the numbers of selected cells, returns the number of cells actually processed 
    /// by this read operation and number of pixels found in these cells;
    virtual size_t read_pix_subset(const MDImage &dnd,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf,size_t &n_pix_in_buffer);
    /// get number of data pixels contributing into the dataset;
    virtual ~MD_File_hdfMatlab4D(void);
private:
/// private copy constructor and assighnment
   MD_File_hdfMatlab4D(const MD_File_hdfMatlab4D& p);
   MD_File_hdfMatlab4D & operator = (const MD_File_hdfMatlab4D & other);
 
};
//
    }
}
#endif
