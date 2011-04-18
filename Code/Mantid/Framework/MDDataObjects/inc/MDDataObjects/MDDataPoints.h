#ifndef H_MD_Pixels
#define H_MD_Pixels

#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MDDataObjects/MDDataPoint.h"
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MDDataObjects/MDDPoints_MemManager.h"
#include "MantidAPI/MemoryManager.h"
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
//********************************************************************************************************************************************************************
struct MDPointsLocations
{
    size_t   n_data_points;
    uint64_t points_location;
    MDPointsLocations():n_data_points(0),points_location(0){};
};
//********************************************************************************************************************************************************************
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
      * initialises MDDataPoints, as file-based structure;
	    * allocates all necessary arrays and provides it with  valid data reader;
      * if the input dataReader is absent, the function initializes its own dataReader and sets output 
      * temporary scrach file to accept pixels data 
     */
	virtual void initialize(boost::shared_ptr<const MDImage> spImage,boost::shared_ptr<IMD_FileFormat> in_spFile);
    // initialises MDDataPoints as memory based; it will switch to fileBased later
	virtual void initialize(boost::shared_ptr<const MDImage> pMDImage);
   
	/// return file current file reader (should we let the factory to remember them and return on request?
    virtual boost::shared_ptr<IMD_FileFormat> getFileReader(void)const{return this->spFileReader;}
    /// check if the MDDataPoints class is initialized;
    bool is_initialized(void)const;

	//*********>  MEMORY  <*************************************************************************
    /// check if the pixels are all in memory;
    bool isMemoryBased(void)const{return memBased;}
	///
	std::vector<char> * get_pBuffer(size_t buf_size=PIX_BUFFER_PREFERRED_SIZE);

    /// function returns numnber of pixels (dataPoints) contributiong into the MD-dataset; The pixels may be on HDD or in memory  
    uint64_t getNumPixels(void)const{return n_data_points;}
    /// get the size of the allocated data buffer (may or may not have valid data in it); Identify main memory footprint;
	size_t getMemorySize()const{return DataBuffer.size();}
    /** returns the size of the buffer allocated for pixels (the number of pixels possible to fit the buffer; 
      * The actual size in bytes will depend on pixel size  -- see get memory size*/
     size_t get_pix_bufSize(void)const;
     /// the function provides memory footprint of the class in the form commont to other MD classes
    size_t sizeofPixelBuffer(void)const{ return getMemorySize();}
     /// get the pixel MDDataPoint size (in bytes)
	unsigned int sizeofMDDataPoint(void)const{return pixDescription.sizeofMDDPoint();}
   /// structure of an MDDataPoint
    MDDataPointsDescription const & getMDPointDescription(void)const{return pixDescription;}

  /// sets the datapoints based in file instead of memory; if memory was allocated for the data before, it should be freed and all data should be damped to HDD 
  // TODO: implement it properly
    virtual void set_file_based();
	//**********************************************************************************************

	/// function returns minimal value for dimension i
    double &rPixMin(unsigned int i){return *(&box_min[0]+i);}
    /// function returns maximal value for dimension i
    double &rPixMax(unsigned int i){return *(&box_max[0]+i);}
	//
    /// get part of the dataset, specified by the vector of MDImage cell numbers. 
    virtual size_t get_pix_subset(const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer);
   /** function adds pixels,from the array of input pixels, selected by indexes in array of pix_selected to internal structure of data indexes 
     which can be actually on HDD or in memory */
    void store_pixels(const std::vector<char> &all_new_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels);


    /** function returns the part of the colum-names which corresponds to the dimensions information;
     * the order of the ID corresponds to the order of the data in the datatables */
    std::vector<std::string> getDimensionsID(void)const{return pixDescription.getDimensionsID();}
 
  protected:

  private:
    /** the parameter identify if the class data are file or memory based
     * usually it is HD based and memory used for small datasets, debugging
     * or in a future when PC-s are big     */
    bool memBased;
	/// The class which describes the structure of sinle data point (pixel)
	 MDDataPointsDescription pixDescription;
    /// the data, describing the detector pixels(events, MDPoints etc.)
    uint64_t  n_data_points;  //< number of data points contributing to dataset
    /// size the data buffer in pixels (data_points) rather then in bytes as in DataBuffer.size();
    size_t  data_buffer_size; 
	/** the data buffer which keeps information on MDDataPoints(pixels) loaded to memory or provide space to loat these data
	 all operations with change of this databuffer or modification of its contents should be done through the MDDPoints_MemManager */
	std::vector<char> DataBuffer;

    std::auto_ptr<MDDPoints_MemManager> pMemoryMGR;
     /// minimal values of ranges the data pixels are in; size is nDimensions
    std::vector<double> box_min;
    /// maximal values of ranges the data pixels are in; size is nDimensions
    std::vector<double> box_max;


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


   };
}
}
#endif
