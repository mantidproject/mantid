#ifndef MDD_POINTS_MEM_MANAGER_H
#define MDD_POINTS_MEM_MANAGER_H

#include "MDDataObjects/MDDataPoint.h"
#include "MDDataObjects/MDImageDatatypes.h"
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "MantidAPI/MemoryManager.h"
#include <boost/shared_ptr.hpp>

/** Class to support memory managment operations performed over arrays of MDDataPoints, which are represented as an arrays of bytes here
    The array of MDDataPoints is arranged in memory according to MDImage, 

	Currently:
	Every image cell has correspondent block of pixels; The pixels for cell N are located after all pixels contributed to cells with i<N and the size of the block
	equal to the value necessary to place MD_image_point[N].npix pixels which contribute to the cell N;

	This all can change in a future, so access to this structure should be organized through MDDPoints interface;
 

    @author Alex Buts, RAL ISIS
    @date 28/01/2011
	Inspired by Horace. 

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

//*
/// the size of the buffer to read pixels (in pixels) while reading parts of datasets --should be optimized for performance and calculated on the basis of performance but how?
//TODO: move it into configuration
#define PIX_BUFFER_PREFERRED_SIZE 10000000
//******************************************
class DLLExport MDDPoints_MemManager
{
public:
	
//************************************************************************************************************
	/// function returns actual number of MDDataPoints (pixels) placed in memory;
	size_t getNPixInMemory(void)const{return n_data_points_in_memory;}
	/// function returns number of cells in MD image used to key the MDDataPoints location
	size_t getNControlCells(void)const{return ImgArray.data_size;}
	/// helper fucntion to get data buffer size in pixels;
	size_t getDataBufferSize(const std::vector<char> &data_buffer)const{return data_buffer.size()/this->pixel_size;}

	/// this function used to check if user completed reading a cell in case when all pixels, contributed to cell do not fit to the read buffer;
	bool is_readCell_completed(void)const{return (n_pix_read_earlier==0)?true:false;}

	/** function adds selected pixels into the collection of the pixels, belonging to this class; Function  returns false if the pixels do not fit memory (free memory); 
	    if the file reader/writer is not initiated, it initiates it. free_memory is expressed in bytes
		To force the pixels dataset to be file-based, set the free_memory to 0 */
    bool  store_pixels(const std::vector<char> &all_new_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels,
	                   size_t free_memory,std::vector<char> & target_data_buffer);

	/** function fills supplied buffer with the data describing selected pixels (pixels contributing into the cells -- number supplied) when all initial pixels data are placed in memory
	 *  returns the index of the last processed cell  */
    size_t get_pix_from_memory(const std::vector<char> &source_data_buffer,const std::vector<size_t> & selected_cells,size_t starting_cell,std::vector<char> &target_pix_buf,size_t &npix_placed_in_buffer);
  /** function analyses the request for the memory, compares it with free memory and modifies actual DataBuffer accordingly, allocating necessary memory if able to do so; 
     * If the buffer has not been allocated it allocates it;
       if it was, it reallocates buffer if the size requested is bigger than existing. The buffer size is specified in pixels, not bytes; 
	   Typical 4D pixel with folat signal and error values and non-compressed indexes occupies 36bytes  (9*4-byte values)  
	   Throws if can not do the job */
    void alloc_pix_array(std::vector<char> &DataBuffer,size_t buf_size=PIX_BUFFER_PREFERRED_SIZE);

	/// constructor, which initiate references to MDImagData, which describes data keys and pixel size 
	MDDPoints_MemManager(MD_img_data const & ImgArray,size_t nImageCells,unsigned int pix_size);
	virtual ~MDDPoints_MemManager();
    
	// internal functions; protected for tests;
protected:
  // internal functions and variables connected with functions above had wired through them.
 /// function adds set of selected pixels(datapoints) to the preallocated data buffer;
	void add_pixels_in_memory(std::vector<char> &data_buffer,const std::vector<char> &all_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels);
private:
	/// function adds new pixels in memory into the free positions, prepared for them before (memory is prepared properly); It relies on previous state of pix_location
	void add_new_pixels(const std::vector<char> &all_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels,
                             std::vector<char> &target_buffer);
	/// function moves existing data within the MD points data buffer to free space for new chunk of data points;
     void expand_existing_data_in_place(std::vector<char> &old_buffer,size_t n_additional_pixels);
     void expand_existing_data_in_new_place(std::vector<char> &old_buffer,std::vector<char> &new_buffer,size_t n_additional_pixels);
 /// calculate the locations of the data points blocks with relation to the image cells, assuming all this can fit to memory;
    virtual size_t init_pix_locations_in_memory();
//***********************************************************************************************************************************

	static Kernel::Logger& g_log;
   // should not copy construct
	MDDPoints_MemManager(const MDDPoints_MemManager &);
	// should not assign
    MDDPoints_MemManager & operator = (const MDDPoints_MemManager & other);

//**************************************************************************************************
	/// number of data points loaded to memory;
	size_t    n_data_points_in_memory;
    /// the size of the pixel (DataPoint, event)(single point of data in reciprocal space) in bytes
    unsigned int pixel_size; 
	/// pointer to the MD image data, which is the source of the information about the location of data points within Image cells
	MD_img_data const & ImgArray;
	/// the array of size nCells (Cells are the MD image cells) which describes location of each blocks of pixels coresponding a the cell in memory;
    std::vector<size_t> pix_location;

//********* Internal variables controlling read from memory operations;
	/// this variable describes the state of the pix_location array; The variable is true if real pixel location which corresponds to the MD image has been calculated
	bool pix_locations_calculated;
	/// this  variabless are necessary to process the case when only part of the pixels contributed into the particular cell fits the data buffer;
	size_t n_pix_read_earlier,    //< this desctibes how meny pixels were read from the current cell during previous incomplete read operation;
	       n_last_processed_cell;


};

}
}
#endif