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
/** Constructor
 *
 * @param spImage: ???
 * */
MDDataPoints::MDDataPoints(boost::shared_ptr<const MDImage> spImage,const MDDataPointsDescription &descr):
  description(descr),
  memBased(false),
  n_data_points(0),

  data_buffer_size(0),
  data_buffer(NULL),
  m_spMDImage(spImage)
{
  if(!m_spMDImage||!m_spMDImage->is_initialized())return;

  std::vector<std::string> dim_tags= m_spMDImage->getGeometry()->getBasisTags();
 
  int nDims = m_spMDImage->getGeometry()->getNumDims();
  this->box_min.assign(nDims,FLT_MAX);
  this->box_max.assign(nDims,-FLT_MAX);

  this->pixel_size = descr.sizeofMDPoint();
}


//------------------------------------------------------------------------------------------------
/** */
size_t MDDataPoints::getNumPixels(boost::shared_ptr<IMD_FileFormat> spFile)
{
  if(this->n_data_points>0){   return n_data_points;
  }

  if(spFile.get()){
    this->n_data_points = spFile->getNPix();
  }else{
    this->g_log.information("MDPixels::getNumPixels: Attemting to get number of pixels from undefined dataset");
    n_data_points = 0;
    throw(Kernel::Exception::NullPointerException("getNumPixels","File reader has not beed defined yet"));
  }
  return n_data_points;
}

//***************************************************************************************
void MDDataPoints::alloc_pix_array(boost::shared_ptr<IMD_FileFormat> spFile)
{
  if(this->data_buffer){
    // if it is already allocated and big enough, do nothing
    size_t nPix= this->getNumPixels(spFile);

    size_t buf_size = (nPix<PIX_BUFFER_SIZE)?nPix:PIX_BUFFER_SIZE;
    if(buf_size!=this->data_buffer_size){
      delete [] this->data_buffer;
      this->data_buffer=NULL;
    }else{
      return;
    }
  }
  unsigned int nDims = this->m_spMDImage->getGeometry()->getNumDims();

  this->box_min.assign(nDims,FLT_MAX);
  this->box_max.assign(nDims,-FLT_MAX);

  // identify maximal number of pixels, possible to fit into buffer for current architecture;
  size_t nPix(0);
  size_t max_size = ~(nPix);
  nPix            = this->getNumPixels(spFile);
  size_t max_pix_num = max_size/this->pixel_size+1;
   
 
  this->data_buffer_size = (nPix<PIX_BUFFER_SIZE)?nPix:PIX_BUFFER_SIZE;
  if(data_buffer_size>max_pix_num){
      data_buffer_size = max_pix_num;
  }

  data_buffer = new char[data_buffer_size*this->pixel_size];


}
//***************************************************************************************
MDDataPoints::~MDDataPoints()
{
  if(data_buffer){
    delete [] data_buffer;
    data_buffer = NULL;
  }
  n_data_points=0;


}

}
}
