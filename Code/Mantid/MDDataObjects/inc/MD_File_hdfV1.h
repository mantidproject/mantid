#ifndef H_FILE_HDF
#define H_FILE_HDF
#include "MD_FileFormat.h"
/** stub for a future binary (hdf) file format reader/writer which will be used for datasets.

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
//*****************************************************************************
class MD_File_hdfV1 :    public MD_FileFormat
{
public:
    MD_File_hdfV1(const char *file_name){throw("this format is not written yet");}
    virtual ~MD_File_hdfV1(void){};
    virtual bool is_open(void)const{return false;}
    virtual void read_dnd(MDData &){};
    virtual void write_dnd(const MDData &){};
    virtual bool read_pix(MDPixels &){return false; }
    virtual size_t read_pix_subset(const MDPixels &,const std::vector<long> &selected_cells,long starting_cell,sqw_pixel *& pix_buf, long &nPixels){
        return selected_cells.size();}
    virtual hsize_t getNPix(void){return -1;}

};
    }
}
#endif
