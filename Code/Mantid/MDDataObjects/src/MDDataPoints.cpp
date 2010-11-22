#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDDataPoints.h"

               
namespace Mantid{
    namespace MDDataObjects{
        using namespace Mantid::Kernel;

 // default names for signal and error fields
 const char *DefaultSignalTags[]={"S","Err","iRun","iDet","iEn"};
MDDataPoints::MDDataPoints(unsigned int nDims,unsigned int nRecDims):
MDImageData(nDims,nRecDims),
memBased(false),
n_data_points(0),
n_fields(9),
data_buffer_size(0),
data_buffer(NULL)
{
  
  std::vector<std::string> dim_tags=this->getBasisTags();
  std::vector<std::string> signal_tags(DefaultSignalTags,DefaultSignalTags+4);
  this->field_tag.reserve(n_fields);
  this->field_tag.insert(field_tag.end(),dim_tags.begin(),dim_tags.end());
  this->field_tag.insert(field_tag.end(),signal_tags.begin(),signal_tags.end());

  this->box_min.assign(nDims,FLT_MAX);
  this->box_max.assign(nDims,-FLT_MAX);



}
size_t
MDDataPoints::getNumPixels(void)
{
    if(this->n_data_points>0){   return n_data_points;
    }

    if(this->theFile){
        this->n_data_points=this->theFile->getNPix();
    }else{
        MDImageData::g_log.information("MDPixels::getNumPixels: Attemting to get number of pixels from undefined dataset");
        n_data_points = 0;
        throw(Kernel::Exception::NullPointerException("getNumPixels","File reader has not beed defined yet"));
    }
    return n_data_points;
}
//
void 
MDDataPoints::set_field_length(const std::vector<unsigned int> &in_fields)
{
  this->n_fields=(unsigned int)in_fields.size();
  this->field_length  = in_fields;

  this->field_start.assign(n_fields+1,0);
   for(unsigned int i=1;i<=n_fields;i++){
      field_start[i]=field_start[i-1]+field_length[i-1];
   }


}
//***************************************************************************************
void
MDDataPoints::alloc_pix_array()
{
  if(this->data_buffer){
    // if it is already allocated and bif enough, do nothing
     size_t nPix= this->getNumPixels();

    size_t buf_size = (nPix<PIX_BUFFER_SIZE)?nPix:PIX_BUFFER_SIZE;
    if(buf_size!=this->data_buffer_size){
        delete [] this->data_buffer;
        this->data_buffer=NULL;
    }else{
      return;
    }
  }
   unsigned int nDims = this->getNumDims();

   this->box_min.assign(nDims,FLT_MAX);
   this->box_max.assign(nDims,-FLT_MAX);

   field_length.assign(n_fields,4);
   this->set_field_length(field_length);
   size_t nPix= this->getNumPixels();

   this->data_buffer_size = (nPix<PIX_BUFFER_SIZE)?nPix:PIX_BUFFER_SIZE;
 
   data_buffer = new char[data_buffer_size*field_start[n_fields]];
 

}
//***************************************************************************************
MDDataPoints::~MDDataPoints()
{
  if(data_buffer){
        delete [] data_buffer;
        data_buffer = NULL;
    }
  n_data_points=0;
  n_fields     =0;
  field_start.clear();
  field_length.clear();

}

//***************************************************************************************

/*
//***************************************************************************************
void
SQW::extract_pixels_from_memCells(const std::vector<long> &selected_cells,long nPix,sqw_pixel *pix_extracted)
{
    long i,ind,ic(0);
    size_t j,npix;
    for(i=0;i<selected_cells.size();i++){
        ind=selected_cells[i];
        npix=this->pix_array[ind].cell_memPixels.size();
        for(j=0;j<npix;j++){
            pix_extracted[ic]=this->pix_array[ind].cell_memPixels[j];
            ic++;
#ifdef _DEBUG
            if(ic>nPix){
                throw("extract_pixels_from_memCells::Algorithm error=> there are more real pixels then was estimated during preselection");
            }
#endif
        }
    }
}

*/
}
}