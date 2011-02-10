#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/MDImage.h"

namespace Mantid{
namespace MDDataObjects{
using namespace Mantid::Kernel;


// logger for MD workspaces  
Kernel::Logger& MDDataPoints::g_log =Kernel::Logger::get("MDWorkspaces");

//------------------------------------------------------------------------------------------------
void 
MDDataPoints::store_pixels(const std::vector<char> &all_new_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels)
{
	if(this->memBased){
		if(this->pMemoryMGR.get()){
			API::MemoryInfo memInf=API::MemoryManager::Instance().getMemoryInfo();
			size_t  free_memory = memInf.availMemory*1024;
			this->memBased = pMemoryMGR->store_pixels(all_new_pixels,pixels_selected,cell_indexes,n_selected_pixels,free_memory,DataBuffer);
			//TODO: this is stub to deal with very frequent possibility of target pixels not fitting memory;
			if(!this->memBased){
				g_log.error()<<" can not store rebinned pixels in memory and storing them on HDD is not implemented yet\n";
				DataBuffer.clear();
				return;
			}
		}else{
			g_log.error()<<" MDDataPoints have not been initiated properly to use store pixels\n";
			throw(std::logic_error("Incorrect MDDataPoints class initialisation"));
		}
	}else{
		//TODO: StorePixelsOnHDD, in temporary or permanent swap file;
	}

}

/** Constructor
 *
 * @param spImage: ???
 * */
MDDataPoints::MDDataPoints(const MDDataPointsDescription &descr):
  pixDescription(descr),
  memBased(true),
  n_data_points(0),
  pMemoryMGR(NULL)
{

  unsigned int nDims = pixDescription.PixInfo().NumDimensions;

  this->box_min.assign(nDims,FLT_MAX);
  this->box_max.assign(nDims,-FLT_MAX);
}
//
bool 
MDDataPoints::is_initialized(void)const
{
    if(spMDImage.get()!=NULL){
		if(spMDImage->getNMDDPoints()==this->n_data_points){
			return true;
		}else{
			return false;
		}
    }else{
        return false;
    }
}
//

void 
MDDataPoints::set_file_based()
{
    //TODO: should verify and if there are fresh data in buffer, dump it on HDD (or algorithm should take care about it)
    this->memBased = false;
	//throw(Kernel::Exception::NotImplementedError("This functionality is not implemented yet"));
	// save pixels from memory
    //this->data_buffer.clear();
}
//
void 
MDDataPoints::initialize(boost::shared_ptr<const MDImage> spImage,boost::shared_ptr<IMD_FileFormat> in_spFile)
{
    this->spMDImage   = spImage;
    this->spFileReader= in_spFile;

    std::vector<std::string> dim_tags = spMDImage->getGeometry()->getBasisTags();
    std::vector<std::string> data_tags = this->pixDescription.getColumnNames();
    // check if the dimensions id are consistant with the data columns i.e. the MDImage and the MDpoints parts of the dataset are consistent
    for(size_t i=0;i<dim_tags.size();i++){
        if(std::find(data_tags.begin(),data_tags.end(),dim_tags[i])==data_tags.end()){
            g_log.error()<<" basis dimension with ID: "<<dim_tags[i]<<" can not be found among the data tags:\n";
            std::stringstream buf;
            for(size_t j=0;j<data_tags.size();j++){
                 buf<<data_tags[j]<<" ";
            }
            g_log.error()<<buf.str()<<std::endl;
            throw(std::invalid_argument("MDDataPoints and MDBasis ID-s are inconsistent"));
        }
    }

   this->n_data_points          = this->spFileReader->getNPix();
   // check if the image is synchroneous with the MDDataPoints dataset;
   if(spMDImage->getNMDDPoints() != this->n_data_points){
	// Data point initialization done this way can be suppported by an empty image only; will throw otherwise
	   if(spMDImage->getNMDDPoints()!=0){
		   g_log.error()<<" Number of points contributed into MD image = "<<spMDImage->getNMDDPoints()<<" is not consistent with number of points in MD Dataset ="<<this->n_data_points<<std::endl;
           throw(std::logic_error("MD data image and MDDataPoints part of the workspace are non-synchronous "));
	   } // if the image is empty, workspace would be not initialized;
   }

   this->memBased                = false;
   
   unsigned int nDims= this->spMDImage->getGeometry()->getNumDims();
   this->box_min.assign(nDims,FLT_MAX);
   this->box_max.assign(nDims,-FLT_MAX);

   // initialize memory managment and control class
 
   pMemoryMGR = std::auto_ptr<MDDPoints_MemManager>(new MDDPoints_MemManager(spMDImage->get_MDImgData(),spMDImage->getDataSize(),this->pixDescription.sizeofMDDPoint()));
   size_t buf_size = ((this->n_data_points<PIX_BUFFER_PREFERRED_SIZE)?(size_t)this->n_data_points:PIX_BUFFER_PREFERRED_SIZE);
   pMemoryMGR->alloc_pix_array(DataBuffer,buf_size);

}
size_t 
MDDataPoints::get_pix_bufSize(void)const
{
	if(pMemoryMGR.get()){
		return pMemoryMGR->getDataBufferSize(DataBuffer);
	}else{
		g_log.error()<<"MDDataPoints::get_pix_bufSize MDDataPoints class has not been initiated properly\n";
		throw(std::invalid_argument("Call to non-initated MDDataPoints class"));
	}

}
// 
void
MDDataPoints::initialize(boost::shared_ptr<const MDImage> pImage)
{
	this->spMDImage  = pImage;

// Data point initialization done this way can be suppported by an empty image only throw otherwise; it will initiate empty target workspace for use with algorithms. 
	if(pImage->getNMDDPoints()!=0){
     	 g_log.error()<<" this kind of initialisation for MDDataPoints can be performed by an empty image only\n";
	     throw(std::logic_error("this kind of initialisation for MDDataPoints can be performed by an empty image only"));
 	}

  
    this->n_data_points           = 0;
	this->memBased                = true;

   unsigned int nDims= this->spMDImage->getGeometry()->getNumDims();

   this->box_min.assign(nDims,FLT_MAX);
   this->box_max.assign(nDims,-FLT_MAX);
  // initialize memory managment and control class
 
   pMemoryMGR = std::auto_ptr<MDDPoints_MemManager>(new MDDPoints_MemManager(spMDImage->get_MDImgData(),spMDImage->getDataSize(),this->pixDescription.sizeofMDDPoint()));
  

    
}
  /// load part of the dataset, specified by the vector of MDImage cell numbers into memory. 
size_t 
MDDataPoints::get_pix_subset(const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
{
    size_t ind_cell_read;
	if(this->memBased){
		ind_cell_read  = this->pMemoryMGR->get_pix_from_memory(pix_buf,selected_cells,starting_cell,pix_buf,n_pix_in_buffer);
	}else{
		ind_cell_read = this->spFileReader->read_pix_subset(*spMDImage,selected_cells,starting_cell,pix_buf,n_pix_in_buffer);
	}
    return ind_cell_read; 
}
std::vector<char> *
MDDataPoints::get_pBuffer(size_t buf_size)
{
	if(this->pMemoryMGR.get()){
		// this will analyse the state of the data and do nothing if size is sufficient or try reallocating sufficient size and keep the data if the size is not sufficient;
		pMemoryMGR->alloc_pix_array(DataBuffer,buf_size);
		return &DataBuffer;
	}else{
		g_log.error()<<" MDDataPoints have not been initialized properly\n";
		throw(Kernel::Exception::NullPointerException("MDDataPoints::get_pBuffer","The object has not been initiated properly"));
	}
}

//***************************************************************************************
MDDataPoints::~MDDataPoints()
{

}

}
}
