#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/MDImage.h"

namespace Mantid{
namespace MDDataObjects{
using namespace Mantid::Kernel;
/// this class is idle at the moment as all renbinning functionality is wired up through the file reader;

// logger for MD workspaces  
Kernel::Logger& MDDataPoints::g_log =Kernel::Logger::get("MDWorkspaces");

//------------------------------------------------------------------------------------------------
void 
MDDataPoints::store_pixels(const std::vector<char> &all_pixels,const std::vector<bool> &pixels_selected,const std::vector<size_t> &cell_indexes,size_t n_selected_pixels)
{

}
void 
MDDataPoints::init_pix_locations()
{
    // and calculate cells location for pixels;
    MD_image_point const * const pData = this->spMDImage->get_const_pData();
    size_t nCells = this->spMDImage->getDataSize();
    if(this->pix_location.size()!=nCells){
        this->pix_location.resize(nCells);
    }

    pix_location[0].points_location = 0;
    // counter for the number of retatined pixels;
    size_t nPix = pData[0].npix;
    for(size_t i=1;i<nCells;i++){
// the next cell starts from the the boundary of the previous one plus the number of pixels in the previous cell
        pix_location[i].points_location=pix_location[i-1].points_location+pData[i-1].npix;
    }

}
/** Constructor
 *
 * @param spImage: ???
 * */
MDDataPoints::MDDataPoints(const MDDataPointsDescription &descr):
  pixDescription(descr),
  memBased(false),
  n_data_points(0),
  data_buffer_size(0),
  data_buffer(NULL)
{
 
  this->pixel_size = descr.sizeofMDPoint();
}
//
bool 
MDDataPoints::is_initialized(void)const
{
    if(spMDImage.get()!=NULL){
        return true;
    }else{
        return false;
    }
}
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
    //this->alloc_pix_array();
   this->n_data_points = this->spFileReader->getNPix();
   
   unsigned int nDims= this->spMDImage->getGeometry()->getNumDims();
   this->box_min.assign(nDims,FLT_MAX);
   this->box_max.assign(nDims,-FLT_MAX);
}
// 
boost::shared_ptr<IMD_FileFormat> 
MDDataPoints::initialize(boost::shared_ptr<const MDImage> pImageData)
{
    if(this->spFileReader.get()==NULL){
        this->spFileReader = getFileReader();
    }
    this->n_data_points =0;
    //this->alloc_pix_array();
   unsigned int nDims= this->spMDImage->getGeometry()->getNumDims();

   this->box_min.assign(nDims,FLT_MAX);
   this->box_max.assign(nDims,-FLT_MAX);

    return this->spFileReader;
}
  /// get part of the dataset, specified by the vector of MDImage cell numbers. 
size_t 
MDDataPoints::get_pix_subset(const std::vector<size_t> &selected_cells,size_t starting_cell,std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
{
    return this->spFileReader->read_pix_subset(*spMDImage,selected_cells,starting_cell,pix_buf,n_pix_in_buffer);
}
std::vector<char> &
MDDataPoints::getBuffer(size_t buf_size)
{
    if(this->data_buffer.size()<buf_size){
        this->alloc_pix_array(buf_size);
    }

     return this->data_buffer;
}
//***************************************************************************************
void 
MDDataPoints::alloc_pix_array(size_t buf_size)
{
    uint64_t nPix = this->n_data_points;
 
    if(this->data_buffer.size()>0){
  
       uint64_t pix_buf_size = (nPix<buf_size)?nPix:buf_size;
       if(pix_buf_size<=this->data_buffer_size){
           return;
       }else{
           this->data_buffer.clear();
           this->data_buffer_size=0;
       }
   }
   unsigned int nDims = this->spMDImage->getGeometry()->getNumDims();


  // identify maximal number of pixels, possible to fit into buffer for current architecture;
  size_t nMemPix(0);
  size_t max_size = ~(nMemPix);
  size_t max_pix_num = max_size/this->pixel_size;
 
  this->data_buffer_size = buf_size;
 
  if(data_buffer_size>max_pix_num){
      this->data_buffer_size = max_pix_num;
  }
  // remove fractional parts of pixel
  size_t dbs = data_buffer_size/this->pixel_size;
  dbs        *=this->pixel_size;
  data_buffer.resize(dbs);


}
//***************************************************************************************
MDDataPoints::~MDDataPoints()
{
 
    data_buffer.clear();
    data_buffer_size=0;
}

}
}
