#ifndef H_MD_Pixels
#define H_MD_Pixels

#include "MDDataObjects/MDImageData.h"
#include "MDDataObjects/MDDataPoint.h"
/** Class to support operations on single data pixels, as obtained from the instrument. Currently it contains information on the location of the pixel in 
    the reciprocal space but this can chane as this information can be computed in the run time
    
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
  
//**********************************************************************************************************************


class DLLExport MDDataPoints :  public MDImageData
{
public:
     MDDataPoints(unsigned int nDims);
    ~MDDataPoints();
 
     /// check if the pixels are all in memory;
    bool isMemoryBased(void)const{return memBased;}
    /// function returns numnber of pixels (dataPoints) contributiong into the MD-data
    size_t getNumPixels(void);


    // Accessors & Mutators used mainly for IO Operations on the dataset
    /// function returns minimal value for dimension i
    double &rPixMin(unsigned int i){return *(&box_min[0]+i);}
    /// function returns maximal value for dimension i
    double &rPixMax(unsigned int i){return *(&box_max[0]+i);}
    /// in-out -- returns or allow to set-up the number of data fields in the dataPoint
    unsigned int &numFields(void){return n_fields;}
     /// returns the pointer to the location of the data buffer; the data has to be processed through MDDataPoint then
    void * get_pBuffer(void){return data_buffer;}
    /// return the size of the buffer allocated for pixels
    size_t get_pix_bufSize(void)const{return data_buffer_size;}
    void set_field_length(const std::vector<unsigned int> &in_fields);
    std::vector<unsigned int> get_field_length(void)const{return field_length;}
    std::vector<unsigned int> get_field_start(void)const {return field_start;}
protected:
/// initiates memory for part of the pixels, which should be located in memory;  
    void alloc_pix_array();
  // the parameter identify if the class data are file or memory based
   // usually it is le based and memory used for small datasets, debugging or in a future when PC are big
    bool      memBased;
private:

/// the data, describing the detector pixels
   size_t  n_data_points;  //< number of data points contributing in dataset
   //
   unsigned int n_fields;  //< number of fields contributed into each data point;
   std::vector<double> box_min, //< minimal and 
                       box_max; //< maximal values of ranges the data pixels are in; size is nDimensions
   std::vector<unsigned int> field_length;  //< length each data point in bytes
   std::vector<unsigned int> field_start;   //< location of data points field from the start of the field (roughly sum of field_length-es, but padding has to be considered for efficiency)
   std::vector<std::string>  field_tag;     //< name of each data field in the pixels array;
   //
   size_t  data_buffer_size;              // in pixels (data_points) rather then in char;
   void *data_buffer;

   // boolean values identifying the way to treat NaN-s and Inf-s in the pixel data
   bool   ignore_inf,ignore_nan;     
// private for the time being but may be needed in a future
   MDDataPoints(const MDDataPoints& p);
   MDDataPoints & operator = (const MDDataPoints & other);
// rebin pixels in the pix_aray and add them to the current dataset ;
//    long rebin_dataset4D(const SlicingProperty &transf, const sqw_pixel *pix_array, long nPix_cell);

//    void extract_pixels_from_memCells(const std::vector<long> &selected_cells,long nPix,sqw_pixel *pix_extracted);
};
    }
}
#endif