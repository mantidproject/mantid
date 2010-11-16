#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid{
  namespace MDDataObjects{

    // Register the workspace into the WorkspaceFactory
    DECLARE_WORKSPACE(MDWorkspace)

// 
void 
MDWorkspace::read_mdd(const char *file_name)
{
    // select file reader and read image part of the data
    this->MDImageData::read_mdd(file_name);
    // alocate memory for pixels;
    this->alloc_pix_array();
}
//
void
MDWorkspace::read_pix(void)
{
        if(this->theFile){
            if(!this->pData){
                g_log.error()<<" can not read pixels data before the MD image is defined\n";
                throw(Exception::NullPointerException("MDPixels::read_pix","MDPixels->data"));
            }
            this->alloc_pix_array();
            if(!this->theFile->read_pix(*this)){
                this->memBased=false;
                g_log.information()<<"MDWorkspace::read_pix: can not read pixels in memory, file operations has to be performed\n";
                throw(std::bad_alloc("can not place all data pixels in the memory, file operations needs to be performed"));
            }else{
                this->memBased=true;
            }
        }else{
            g_log.error()<<"MDWorkspace::read_pix: file reader has not been defined\n";
            throw(Exception::NullPointerException("MDPixels::read_pix","MDPixels->theFile"));
        }
}
size_t 
MDWorkspace::read_pix_selection(const std::vector<size_t> &cells_nums,size_t &start_cell,std::vector<char> &pix_buf,size_t &n_pix_in_buffer)
{
    if(!this->theFile){
        throw(std::bad_alloc("MDPixels::read_selected_pix: file reader has not been defined"));
    }
    return this->theFile->read_pix_subset(*this,cells_nums,start_cell,pix_buf,n_pix_in_buffer);
} 


}
}
//*********************************************************************************************************************************************************************************
namespace Mantid
{
  namespace Kernel
  {
    template<> DLLExport
    Mantid::MDDataObjects::MDWorkspace_sptr IPropertyManager::getValue<Mantid::MDDataObjects::MDWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::MDDataObjects::MDWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::MDDataObjects::MDWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::MDDataObjects::MDWorkspace_const_sptr IPropertyManager::getValue<Mantid::MDDataObjects::MDWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::MDDataObjects::MDWorkspace_const_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::MDDataObjects::MDWorkspace_const_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type";
        throw std::runtime_error(message);
      }
    }
  } // namespace Kernel
} // name