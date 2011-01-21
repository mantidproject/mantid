#ifndef H_MD_Pixels
#define H_MD_Pixels

#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MDDataObjects/MDDataPoint.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include <boost/shared_ptr.hpp>

/** Class to support operations on single data pixels, as obtained from the instrument.
 *  Currently it contains information on the location of the pixel in
    the reciprocal space but this can change as this information
    can be computed at the run time.
    
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

namespace Mantid
{

namespace MDDataObjects
{

//* MDPixels
/// the size of the buffer to read pixels (in bytes) while reading parts of datasets --should be optimized for performance and deleted
#define PIX_BUFFER_SIZE 10000000*36
/// the size of the data page (in bytes), providing optimal speed of data exchange with HDD -- should be calculated;
#define PAGE_SIZE  4096


  class MDImage;
  class MDDataPointsDescription: public MDPointDescription
  {
  public:
	  MDDataPointsDescription(const MDPointDescription &descr):
	  /// this one describes structure of single pixel as it is defined and written on HDD
      MDPointDescription(descr)
	  { }
  private:
	  // this has not been defined at the moment, but will provide the description for possible lookup tables
	  std::vector<double> lookup_tables;
  };
  
  //**********************************************************************************************************************
  class DLLExport MDDataPoints
  {
  public:
    MDDataPoints(const MDDataPointsDescription &description);
    virtual ~MDDataPoints();
/******************************************************************************************************
    /** initialises MDDataPoints, allocates all necessary arrays and provides it with  valid data reader; 
      * if the input dataReader is absent, the function initializes its own dataReader and sets output 
      * temporary scrach file to accept pixels data 
     */
	virtual void initialize(boost::shared_ptr<const MDImage> spImage,boost::shared_ptr<IMD_FileFormat> in_spFile);
    //
	virtual boost::shared_ptr<IMD_FileFormat> initialize(boost::shared_ptr<const MDImage> pImageData);
    /// return file current file reader (should we let the factory to remember them and return on request?
    virtual boost::shared_ptr<IMD_FileFormat> getFileReader(void)const{return this->spFileReader;}
    /// check if the MDDataPoints class is initialized;
    bool is_initialized(void)const;

    /// check if the pixels are all in memory;
    bool isMemoryBased(void)const{return memBased;}
    /// function returns numnber of pixels (dataPoints) contributiong into the MD-dataset  
    uint64_t getNumPixels(void)const{return n_data_points;}
    /// get the size of the allocated data buffer (may or may not have valid data in it); Identify main memory footprint;
    size_t getMemorySize()const{return data_buffer_size*pixel_size;}
     /// function returns minimal value for dimension i
    double &rPixMin(unsigned int i){return *(&box_min[0]+i);}
    /// function returns maximal value for dimension i
    double &rPixMax(unsigned int i){return *(&box_max[0]+i);}
    /// get part of the dataset, specified by the vector of MDImage cell numbers. 
    virtual size_t get_pix_subset(const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer);
    /** return the size of the buffer allocated for pixels (the number of pixels possible to fit the buffer; 
      * The actual size in bytes will depend on pixel size  -- see get memory size*/
    size_t get_pix_bufSize(void)const{return data_buffer_size;} 
    /// the function provides memory footprint of the class in the form commont to other MD classes
    size_t sizeofPixelBuffer(void)const{ return getMemorySize();}

    /// get the pixel MDDataPoint size (in bytes)
    unsigned int sizeofMDDataPoint(void)const{return pixel_size;}
   /** function returns the pointer to the buffer to keep data points; If the buffer has not been allocated it allocates it;
       if it was, it reallocates buffer if the size requested is bigger than existing. The buffer size is specified in pixels; */
    std::vector<char> &getBuffer(size_t buf_size=PIX_BUFFER_SIZE);
    /** function returns the part of the colum-names which corresponds to the dimensions information;
     * the order of the ID corresponds to the order of the data in the datatables */
    std::vector<std::string> getDimensionsID(void)const{return pixDescription.getDimensionsID();}
   //
    MDDataPointsDescription const & getMDPointDescription(void)const{return pixDescription;}
   /** function adds pixels,from the array of input pixels, selected by indexes in array of pix_selected to internal structure of data indexes 
     which can be actually on HDD or in memory */
    void store_pixels(const std::vector<char> &all_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels);
  /// calculate the locations of the data points blocks with relation to the image cells
    virtual void init_pix_locations();
  protected:

  private:
    /** the parameter identify if the class data are file or memory based
     * usually it is HD based and memory used for small datasets, debugging
     * or in a future when PC-s are big     */
    bool memBased;
    std::vector<MDPointsLocations> pix_location;
	 /// The class which describes the structure of sinle data point (pixel)
	 MDDataPointsDescription pixDescription;

    /// the data, describing the detector pixels(events, MDPoints etc.)
    uint64_t  n_data_points;  //< number of data points contributing to dataset
    /// the size of the pixel (DataPoint, event)(single point of data in reciprocal space) in bytes
    unsigned int pixel_size; 
     /// minimal values of ranges the data pixels are in; size is nDimensions
    std::vector<double> box_min;
    /// maximal values of ranges the data pixels are in; size is nDimensions
    std::vector<double> box_max;
    //
    size_t  data_buffer_size; //< size the data buffer in pixels (data_points) rather then in char;
    std::vector<char> data_buffer;

    // private for the time being but may be needed in a future
    MDDataPoints(const MDDataPoints& p);
    MDDataPoints & operator = (const MDDataPoints & other);
    //Shared pointer Allows access to current geometry owned by MDImage
    boost::shared_ptr<const MDImage> spMDImage; 
    /** Shared pointer to the file reader responsible for data exchange with the data file. For input forkspace it has to be
     *  initated by the data reader from the MDWorkspace and for the output workspace it may be initated by the factory if
     * the memory is not sufficient to keep incoming pixels; It also has to be initated when (if) save workpace algorithm is called;
     */
    boost::shared_ptr<IMD_FileFormat> spFileReader;
    /// Common logger for logging all MD Workspace operations;
    static Kernel::Logger& g_log;


  // initiates memory for part of the pixels, which should be located in memory;
    void alloc_pix_array(size_t data_buffer_size);

  };
}
}
#endif
