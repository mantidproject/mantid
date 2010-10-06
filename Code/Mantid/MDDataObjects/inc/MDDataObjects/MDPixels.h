#ifndef H_MD_Pixels
#define H_MD_Pixels

#include "MDData.h"
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

// structure describing the Horace pixels
// will probably change in a future as tailored for our purposes;
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

struct pix_location
{
    size_t chunk_file_location0;
    std::auto_ptr<std::vector<sqw_pixel>> cell_memPixels;
    std::auto_ptr<std::vector<size_t>>  data_chunks;
};


//**********************************************************************************************************************

class DLLExport MDPixels :  public MDData
{
public:
     MDPixels(unsigned int nDims);
    ~MDPixels();
     void set_shape(const SlicingData &trf){
         this->alloc_mdd_arrays(trf);
         if(pix_array){
             delete [] pix_array;
             pix_array=NULL;
         }
         alloc_pix_array();
    }
    /// function reads image part of sqw file and prepares class to work with pixels information
    void read_mdd(const char *file_name);
    /// check if the pixels are all in memory;
    bool isMemoryBased(void)const{return memBased;}
    /// function returns numnber of pixels contributiong into the MD-data
    long getNumPixels(void);

    /// read the whole pixels dataset in the memory
    void read_pix(void);

   /// read the the pixels corresponding to cells in the vector cell_num
    size_t read_pix_selection(const std::vector<size_t> &cells_nums,size_t &start_cell,sqw_pixel *& pix_buf,size_t &pix_buf_size,size_t &n_pix_in_buffer);
 
    // function applies transformation matrix to the current dataset and returns new dataset rebinned accordingly to the 
    // requests of the transformation
    //void rebin_dataset4D(const SlicingData &transf,MDPixels &newsqw);
private:
    // the parameter identify if the class data are file or memory based
   // usually it is file based and memory used for small datasets, debugging or in a future when PC are big
    bool      memBased;
   
/// number of real pixels contributed in the dataset (rather sqw parameter) (should be moved there?)
    long nPixels;      
/// the array of structures, describing the detector pixels
    pix_location *pix_array;
   // boolean values identifying the way to treat NaN-s and Inf-s in the pixel data
    bool   ignore_inf,ignore_nan;     

    void alloc_pix_array();
//
    friend class MD_File_hdfMatlab;
// private for the time being but will be needed in a future
   MDPixels(const MDPixels& p);
   MDPixels & operator = (const MDPixels & other);
// rebin pixels in the pix_aray and add them to the current dataset ;
//    long rebin_dataset4D(const SlicingData &transf, const sqw_pixel *pix_array, long nPix_cell);
//    void complete_rebinning(void);
//    void extract_pixels_from_memCells(const std::vector<long> &selected_cells,long nPix,sqw_pixel *pix_extracted);
};
    }
}
#endif