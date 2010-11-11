#ifndef H_FILE_FORMAT
#define H_FILE_FORMAT

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <hdf5.h>

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
class MDData;
class MDPixels;
struct sqw_pixel; // TO DO: this will probably change
//
class IMD_FileFormat
{
public:
    IMD_FileFormat(void){};
    virtual bool is_open(void)const{return false;}
    virtual void read_mdd(MDData &)=0;
    virtual bool read_pix(MDPixels &)=0; 
    virtual size_t read_pix_subset(const MDPixels &sqw,const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<sqw_pixel> &pix_buf, size_t &buf_size,size_t &n_pix_in_buffer)=0; 
    virtual hsize_t getNPix(void)=0;
    virtual void write_mdd(const MDData &)=0;
    virtual ~IMD_FileFormat(void){};
};
//
    }
}
#endif