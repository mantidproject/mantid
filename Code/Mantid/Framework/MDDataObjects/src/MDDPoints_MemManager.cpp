#include "MDDataObjects/MDDPoints_MemManager.h"


namespace Mantid{
namespace MDDataObjects{
using namespace Mantid::Kernel;

// logger for MD workspaces  
Kernel::Logger& MDDPoints_MemManager::g_log =Kernel::Logger::get("MDWorkspaces");

MDDPoints_MemManager::MDDPoints_MemManager(MD_img_data const & inImgArray,size_t nImageCells,unsigned int pix_size):
    n_data_points_in_memory(0),
    pixel_size(pix_size),ImgArray (inImgArray),
    pix_locations_calculated(false),
    n_pix_read_earlier(0)
{
  UNUSED_ARG(nImageCells);

}



MDDPoints_MemManager::~MDDPoints_MemManager()
{
}
//

//
void 
MDDPoints_MemManager::add_pixels_in_memory(std::vector<char> &data_buffer,const std::vector<char> &all_pixels,const std::vector<bool> &pixel_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels)
{
// memory block should be sufficient for place all pixels
	if(this->n_data_points_in_memory>0){
		// image and npixels are not consistent
			if(this->n_data_points_in_memory+n_selected_pixels !=this->ImgArray.npixSum){
				g_log.error()<<"add_pixels_in_memory: number of pixels contributed to image: "<<ImgArray.npixSum<<" is not equal to number of pixels in memory: "
					         <<n_data_points_in_memory+n_selected_pixels<<std::endl;
				throw(std::invalid_argument("MD image is not consistent with MDDataPoints"));
			}
			// expand location of existing data in memory to fit new pixels; will throw if buffer is not big enough to fit everything in memory;
			this->expand_existing_data_in_place(data_buffer,n_selected_pixels);
		}else{
			// image and npixels are not consistent;
			if(n_selected_pixels != this->ImgArray.npixSum){
				g_log.error()<<"add_pixels_in_memory: number of pixels contributed to image: "<<ImgArray.npixSum<<" is not equal to number of pixels in memory: "
				         <<n_selected_pixels<<std::endl;
				throw(std::invalid_argument("MD image is not consistent with MDDataPoints"));
			}

    	   // if this is the first operation, identify the location of pixels in memory
			this->init_pix_locations_in_memory();
	}
	
	this->add_new_pixels(all_pixels,pixel_selected,cell_indexes,n_selected_pixels,data_buffer);
	//
    this->init_pix_locations_in_memory();

}
//

void 
MDDPoints_MemManager::add_new_pixels(const std::vector<char> &all_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels,
                             std::vector<char> &target_buffer)
{
// function adds new pixels to properly allocated and shaped places of memory, providing continuous memory distribution after the operation;
   const char *source_data   = &all_pixels[0];
   char       *target_data   = &target_buffer[0];
   unsigned int data_stride  = this->pixel_size;

	size_t cell_index;
	size_t ic_retauned_pixels(0);
	size_t n_all_pixels = pixels_selected.size();
	for(size_t i=0;i<n_all_pixels;i++){
		if(pixels_selected[i]){
			// what cell this pixel belongs to;
			cell_index = cell_indexes[ic_retauned_pixels];
			// if pixels are in memory, their location can be described by size_t;
			size_t location = pix_location[cell_index];
			pix_location[cell_index]++;

			memcpy(target_data+data_stride*location,source_data+data_stride*i,data_stride);
			// increase the counter of the retained pixels;
			ic_retauned_pixels++;
		}
	}
    this->n_data_points_in_memory+= n_selected_pixels;
	

}

size_t
MDDPoints_MemManager::init_pix_locations_in_memory()
{
   size_t nCells = ImgArray.data_size;
   this->pix_location.assign(nCells+1,0);
   size_t max_npix(0);
   max_npix = ~max_npix;
 

	MD_image_point *pImgData = ImgArray.data;

    pix_location[0] = 0;
    // counter for the number of retatined pixels;
    uint64_t nPix = pImgData[0].npix;
	uint64_t nPix_location(0);
	if(nPix>max_npix){
		g_log.error()<<"init_pix_locations_in_memory: number of the pixels "<<nPix<<" contributed into cell 0 exceeds maximal size of object in memory"<<max_npix<<std::endl;
		throw(std::invalid_argument("number of pixels in memory exceeds the max integer for this computer"));
	}
    for(size_t i=1;i<nCells;i++){
// the next cell starts from the the boundary of the previous one plus the number of pixels in the previous cell
		nPix_location  = pix_location[i-1]+ nPix;
		nPix           = pImgData[i].npix;
		if(nPix>max_npix||nPix_location>max_npix){
			g_log.error()<<"init_pix_locations_in_memory: number of the pixels "<<nPix<<" contributed into cell N "<<i<<" exceeds maximal size of object in memory for current architecture"<<max_npix<<std::endl;
			throw(std::invalid_argument("number of pixels in memory exceeds the max integer for this computer"));
		}
        pix_location[i]=(size_t)(nPix_location);
    }
	// finalize
	nPix_location  = pix_location[nCells-1]+ nPix;
	if(nPix_location>max_npix){
			g_log.error()<<"init_pix_locations_in_memory: total number of the pixels "<<nPix_location<<" exceeds maximal size of object in memory for current architecture"<<max_npix<<std::endl;
			throw(std::invalid_argument("number of pixels in memory exceeds the max integer for this computer"));
	}
    pix_location[nCells]=(size_t)(nPix_location);


	size_t nTotalPix = pix_location[nCells];

	this->pix_locations_calculated=true;
	return nTotalPix;
}
//
void
MDDPoints_MemManager::expand_existing_data_in_place(std::vector<char> &data_buffer,size_t n_additional_pixels)
{
// the function is called after a consequent rebinning step; At this stage, MD_image_point structure have information about the number of pixels in each cell and
// pix_location identifies the final positions of the pixels added at previous rebinning step.
// strictly non-parallel; Should try to use if memory is restricted only; 

 // have to fit memory, so size_t
 	size_t n_pixels_total = this->n_data_points_in_memory+n_additional_pixels;
	size_t data_buffer_size = this->getDataBufferSize(data_buffer);

	if(n_pixels_total>data_buffer_size){ //  Buffer size verified to be sufficient to place all pixels;
		this->alloc_pix_array(data_buffer,n_pixels_total);

	}
	size_t nCells = ImgArray.data_size;

  unsigned int data_stride           = this->pixel_size;
  if(nCells<2)return;
  
  MD_image_point *pImgData = ImgArray.data;

  char      *data   = &data_buffer[0];
  size_t cells_end     = n_pixels_total;
  size_t old_block_end = pix_location[nCells];
  for(size_t i=1;i<nCells;i++){
	// take the last non-processed cell;
	size_t cell_num = nCells-i;
	// identify new block start, sufficient to place all new npix;
	size_t block_start = cells_end - (size_t)pImgData[cell_num].npix;
	//
	size_t old_location=pix_location[cell_num];
	// the size of data in previous block
	size_t block_size  = old_block_end-old_location;
	//
	memmove(data+block_start*data_stride,data+old_location*data_stride,block_size*data_stride);
	// moving to the next block which will start from the end of current
	cells_end = block_start;
	// pix location will be used as a counter for a free space left after adding the following pixels a.g. as new_pix_location;
	old_block_end         = old_location;
	pix_location[cell_num]= block_start+block_size;
  }

   this->pix_locations_calculated=false; 
}
//
void
MDDPoints_MemManager::expand_existing_data_in_new_place(std::vector<char> &old_data_buffer,std::vector<char> &new_data_buffer,size_t n_additional_pixels)
{
 // pix_location provides the locations of free space from previous add operation
// easy parallelizeble after having second copy of pix_location

 // have to fit memory, so size_t
 	size_t n_pixels_total = this->n_data_points_in_memory+n_additional_pixels;

	// the function is called after a rebinning step; At this stage, MD_image_point structure have information about the number of pixels in each cell and
	// pix_location identifies the final positions of the pixels added at previous rebinning step. Buffer size is verified to be sufficient to place all pixels;
	if(n_pixels_total*this->pixel_size>new_data_buffer.size()){
		g_log.error()<<" The size of allocated data buffer = "<<new_data_buffer.size()<<" is insufficient to add "<<n_additional_pixels<< " pixels in memory, as is already occupied by"
			<<n_data_points_in_memory<<" pixels\n";
		throw(std::invalid_argument("can not add new pixels to allocated memory"));
	}

	MD_image_point *pImgData = ImgArray.data;
    size_t nCells            = ImgArray.data_size;
	unsigned int data_stride           = this->pixel_size;


   char      *source_data   = &old_data_buffer[0];
   char      *target_data   = &new_data_buffer[0];
   size_t new_location = 0;
   size_t old_location = 0;
   for(size_t i=0;i<nCells;i++){
	// the size of data in previous block
	 size_t block_size  = pix_location[i+1]-old_location;
	//
 	memcpy(target_data+new_location*data_stride,source_data+old_location*data_stride,block_size*data_stride);
	// moving to the next block which will start from the end of current
	 old_location = pix_location[i+1];
  
	 // indicates new free space to add new pixels to
	 pix_location[i]=new_location+block_size;
     new_location +=(size_t)pImgData[i].npix;
  }
  this->pix_locations_calculated=false;
}
//
size_t 
MDDPoints_MemManager::get_pix_from_memory(const std::vector<char> &data_buffer,const std::vector<size_t> & selected_cells,size_t starting_cell,std::vector<char> &target_pix_buf,size_t &n_pix_in_buffer)
{
	// total number of pixels read during current operation
	size_t n_pix_read_total(0);
	// number of pixels in current cell;
	uint64_t cell_npix;

	if(starting_cell==0){
		n_pix_read_earlier   =0;
		n_last_processed_cell=0;
	}
	if(starting_cell!=n_last_processed_cell){
		n_pix_read_earlier   =0;
	}
	// this will verify if pixels location corresponds to the image and if not -- calculates the pixels location;
	if(!this->pix_locations_calculated){
		this->init_pix_locations_in_memory();
	}

	unsigned int data_stride = this->pixel_size;
	char *target_data       = &target_pix_buf[0];
	const char *source_data = &data_buffer[0];

	size_t buf_capacity_npix = target_pix_buf.size()/data_stride;
	MD_image_point *pData    = ImgArray.data;

	size_t pix_start_location,n_pix_2read;


	size_t cell_num,cell_ind;
	for(cell_num=starting_cell;cell_num<selected_cells.size();cell_num++){
		cell_ind  = selected_cells[cell_num];
		cell_npix = pData[cell_ind].npix;
		if(cell_npix==0)continue;

		if(buf_capacity_npix>=n_pix_read_total+cell_npix-n_pix_read_earlier){
			pix_start_location = pix_location[cell_ind]+n_pix_read_earlier;
			n_pix_2read        = (size_t)(cell_npix-n_pix_read_earlier);
			memcpy(target_data+data_stride*n_pix_read_total,source_data+data_stride*pix_start_location,n_pix_2read*data_stride);
			n_pix_read_earlier   = 0;
		    n_pix_read_total    +=  n_pix_2read;
		}else{  // next cell contents can not be fit into the buffer;
			if(buf_capacity_npix>n_pix_read_total){ // there is still place to read something but not the whole cell
				pix_start_location = pix_location[cell_ind]+n_pix_read_earlier;
				n_pix_read_earlier =  buf_capacity_npix-n_pix_read_total;
                memcpy(target_data+data_stride*n_pix_read_total,source_data+data_stride*pix_start_location,n_pix_read_earlier*data_stride);
				n_pix_read_total+=n_pix_read_earlier;
				break;
			}else{                                  // no place to read anything;
				break;
			}

		}


	}

	n_last_processed_cell=cell_num;
	n_pix_in_buffer      =(size_t)n_pix_read_total;
	return cell_num;
}
//***************************************************************************************
void 
MDDPoints_MemManager::alloc_pix_array(std::vector<char> &data_buffer,size_t buf_size_in_pix)
{  
	// data_bufer_size in pixels
    size_t data_buffer_size = this->getDataBufferSize(data_buffer);
    if(data_buffer.size()>0){
       if(buf_size_in_pix<=data_buffer_size){
           return;
       }
   } 
   

   API::MemoryInfo memInf=API::MemoryManager::Instance().getMemoryInfo();

   size_t  free_memory = memInf.availMemory*1024;
   size_t max_pix_num = free_memory/this->pixel_size/2;
   if(buf_size_in_pix>max_pix_num)buf_size_in_pix=max_pix_num;

   data_buffer_size = buf_size_in_pix;
 
  // remove fractional parts of pixel
   size_t dbs = data_buffer_size*this->pixel_size;
   try{
	 data_buffer.resize(dbs);
   }catch(std::bad_alloc &err){
	  data_buffer_size /= 2;
	  dbs              /= 2;
	  if(this->n_data_points_in_memory > data_buffer_size){
		  	g_log.error()<<" can not re-allocate memory to increase data buffer to "<<data_buffer_size<<" as "<<n_data_points_in_memory<<" MD data points are already in the buffer\n";
			throw(err);
	  }
	  try{
			data_buffer.resize(dbs);
	  }catch(std::bad_alloc &){
		  data_buffer_size /= 2;
		  dbs                    /= 2;
		  if(this->n_data_points_in_memory > data_buffer_size){
			  	g_log.error()<<" can not re-allocate memory to increase data buffer to "<<data_buffer_size<<" as "<<n_data_points_in_memory<<" MD data points are already in the buffer\n";
				throw(err);
		  }
		  try{
			  	data_buffer.resize(dbs);
		  }catch(std::bad_alloc &err){
			  g_log.error()<<" can not allocate memory to keep "<<data_buffer_size<<" MD data points\n";
			  throw(err);
		  }
	  }
  }

 
}
//
bool  
MDDPoints_MemManager::store_pixels(const std::vector<char> &all_new_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels,
                                  size_t free_memory,std::vector<char> & target_data_buffer)
{
	bool memBased(true);
 //  API::MemoryInfo memInf=API::MemoryManager::Instance().getMemoryInfo();
//   size_t  free_memory = memInf.availMemory*1024;
	size_t data_buffer_size = this->getDataBufferSize(target_data_buffer);
	size_t max_num_pix(0);
	max_num_pix = ~max_num_pix;
	size_t max_pix_num = free_memory/this->pixel_size;
	size_t max_npix_fit_memory = ((max_pix_num>max_num_pix)?max_num_pix:max_pix_num);
	size_t size_requested     = this->n_data_points_in_memory+n_selected_pixels;

	if(size_requested > max_npix_fit_memory){
		memBased = false;
	}
	if(memBased){

		if(size_requested>data_buffer_size){
		 
 			  // let's identify what memory we want to allocate for this pixels and for a future;
				  size_t size_optimal = (size_requested/PIX_BUFFER_PREFERRED_SIZE);
				  if(size_optimal*PIX_BUFFER_PREFERRED_SIZE!=size_requested)size_optimal++;
				  size_optimal*=PIX_BUFFER_PREFERRED_SIZE;
				  if(size_optimal>max_pix_num)size_optimal = max_pix_num;

				  // try to allocate the memory clamed to be free
				  try{
					  // 1 try to allocate new buffer
					  std::vector<char> new_buffer(size_optimal*this->pixel_size);
					  if(this->n_data_points_in_memory>0){
						  // copy old data to new buffer and swap buffers;
						  this->expand_existing_data_in_new_place(target_data_buffer,new_buffer,n_selected_pixels);
						  new_buffer.swap(target_data_buffer);
						  data_buffer_size = size_optimal;
						  new_buffer.clear();

						  // add new data to the buffer; the locations is already prepared;
						  this->add_new_pixels(all_new_pixels,pixels_selected,cell_indexes,n_selected_pixels,target_data_buffer);
						  this->init_pix_locations_in_memory();
					  }else{
						  new_buffer.swap(target_data_buffer);
						  data_buffer_size = size_optimal;
						  size_t nPixInImage=this->init_pix_locations_in_memory();
						  if(nPixInImage!=n_selected_pixels){
							  g_log.error()<<"store_pixels:: Number of pixels contributed in MDImage: "<<nPixInImage<<" is not equal to number of actual pixels "<<n_selected_pixels<<std::endl;
							  throw(std::invalid_argument("store_pixels:: MD image and MDDataPoints are not consistent"));
						  }
						  this->add_new_pixels(all_new_pixels,pixels_selected,cell_indexes,n_selected_pixels,target_data_buffer);
						  this->init_pix_locations_in_memory();
					  }
				  }catch(std::bad_alloc &){
					  try{
						  // try this as this is slower but trying accomodate pixels in-place which still may be possible
						  this->add_pixels_in_memory(target_data_buffer,all_new_pixels,pixels_selected,cell_indexes,n_selected_pixels);
					  }catch(std::bad_alloc &){
     						memBased = false;
					  }
				  }

		}else{  // existing buffer is sufficient or almost sufficient to place all pixels, so no point in allocating new buffer;
			this->add_pixels_in_memory(target_data_buffer,all_new_pixels,pixels_selected,cell_indexes,n_selected_pixels);
		}
	           
	}else{ // mem_based false
	}
	return memBased;
}


}
}
