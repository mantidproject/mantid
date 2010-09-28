#ifndef H_SQW
#define H_SQW
#include "MDData.h"

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
    long chunk_file_location0;
    std::vector<sqw_pixel>  cell_memPixels;
    std::vector<long>     * data_chunks;
};
// the size of the buffer to read pixels while reading parts of datasets --should be optimized for performance;
#define PIX_BUFFER_SIZE 1000000


//**********************************************************************************************************************

class MDPixels :  public MDData
{
public:
     MDPixels(void);
    ~MDPixels(void);
     void set_shape(const SlicingData &trf){
         this->alloc_dnd_arrays(trf);
         if(pix_array){
             delete [] pix_array;
             pix_array=NULL;
         }
         alloc_pix_array();
    }


    /// read the whole pixels dataset in the memory
    void read_pix(void);
    // function applies transformation matrix to the current dataset and returns new dataset rebinned accordingly to the 
    // requests of the transformation
    void rebin_dataset4D(const SlicingData &transf,MDPixels &newsqw);
private:
    // the parameter identify if the class data are file or memory based
   // usually it is file based and memory used for small datasets, debugging or in a future when PC are big
    bool      memBased;
   
// number of real pixels contributed in the dataset (rather sqw parameter) (should be moved there?)
    size_t nPixels;      
// the array of structures, describing the detector pixels
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
    long rebin_dataset4D(const SlicingData &transf, const sqw_pixel *pix_array, long nPix_cell);
    void complete_rebinning(void);
    void extract_pixels_from_memCells(const std::vector<long> &selected_cells,long nPix,sqw_pixel *pix_extracted);
};
    }
}
#endif