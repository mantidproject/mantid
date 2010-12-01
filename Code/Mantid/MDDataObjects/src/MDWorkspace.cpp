#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid{
  namespace MDDataObjects{

     //Seam method.
     boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints> getDataPoints(boost::shared_ptr<MDImage> imageData)
     {
       return  boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints>(new MDDataPoints(imageData)); //TODO replace with some other factory call.
     }

     //Seam method.
     boost::shared_ptr<Mantid::MDDataObjects::MDImage> getImageData(Mantid::Geometry::MDGeometry* geometry)
     {
       return boost::shared_ptr<Mantid::MDDataObjects::MDImage>(new MDImage(geometry));
     }

     void MDWorkspace::init(boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> spFile, Mantid::Geometry::MDGeometry* geometry) //TODO: this provides a 'seam' for simplier move to DataHandling in future.
     {
       this->m_spFile = spFile;
       this->m_spImageData = getImageData(geometry);
       this->m_spDataPoints = getDataPoints(m_spImageData); //Takes a pointer to the image data in order to be able to extract an up-to-date geometry.
       
     }

    // Register the workspace into the WorkspaceFactory
    DECLARE_WORKSPACE(MDWorkspace)


    // logger for MD workspaces  
    Kernel::Logger& MDWorkspace::g_log =Kernel::Logger::get("MDWorkspaces");

    // 
    void 
    MDWorkspace::read_mdd()
    {
      //  read image part of the data
      this->m_spFile->read_mdd(*this->m_spImageData);
      // alocate memory for pixels;
      m_spDataPoints->alloc_pix_array(m_spFile);
      m_spImageData->identify_SP_points_locations();
    }
    //
    void
    MDWorkspace::read_pix(void)
    {
      if(this->m_spFile.get()){
        m_spFile->read_pix(*m_spDataPoints);
      }else{
		  throw(std::runtime_error("read_pix: file reader has not been defined"));
      }


    }

    size_t 
      MDWorkspace::read_pix_selection(const std::vector<size_t> &cells_nums,size_t &start_cell,std::vector<char> &pix_buf,size_t &n_pix_in_buffer)
    {
      if(!this->m_spFile.get()){
		  throw(std::runtime_error("MDPixels::read_selected_pix: file reader has not been defined"));
      }
      return this->m_spFile->read_pix_subset(*m_spImageData,cells_nums,start_cell,pix_buf,n_pix_in_buffer);
    } 

    Mantid::Geometry::MDGeometry const * const
      MDWorkspace::getGeometry() const
    {
      return this->m_spImageData->getGeometry();
    }

    long MDWorkspace::getMemorySize(void) const
    {
      return m_spImageData->getMemorySize() + m_spDataPoints->getMemorySize() ;
    } 

    void  MDWorkspace::write_mdd(void)
    {
      if(this->m_spFile.get()){
         this->m_spFile->write_mdd(*m_spImageData);
      }else{
		  throw(std::runtime_error("MDPixels::read_selected_pix: file reader has not been defined"));
      }
    }


    int MDWorkspace::getNPoints() const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::IMDDimension* MDWorkspace::getDimensionImp(std::string id) const
    {
      MDDimension* dimension =m_spImageData->getGeometry()->getDimension(id,true); //TODO: fix MDGeometry to work with vector of shared_ptr rather than vector of raw pointers.
      return new MDDimension(dimension->getDimensionId());
    }

    Mantid::Geometry::MDPoint * MDWorkspace::getPointImp(int index) const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCellImp(int dim1Increment) const 
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCellImp(int dim1Increment, int dim2Increment) const 
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCellImp(int dim1Increment, int dim2Increment, int dim3Increment)  const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCellImp(int dim1Increment, int dim2Increment, int dim3Increment, int dim4Increment)  const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCellImp(...)  const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::IMDDimension* MDWorkspace::getXDimensionImp() const
    {
      MDDimension& xDimension = m_spImageData->getGeometry()->getXDimension(); //TODO: fix MDGeometry to work with vector of shared_ptr rather than vector of raw pointers.
      return new MDDimension(xDimension.getDimensionId());
    }

    Mantid::Geometry::IMDDimension* MDWorkspace::getYDimensionImp() const
    {
     MDDimension& yDimension = m_spImageData->getGeometry()->getYDimension(); //TODO: fix MDGeometry to work with vector of shared_ptr rather than vector of raw pointers.
      return new MDDimension(yDimension.getDimensionId());
    }

    Mantid::Geometry::IMDDimension* MDWorkspace::getZDimensionImp() const
    {
      MDDimension& zDimension = m_spImageData->getGeometry()->getZDimension(); //TODO: fix MDGeometry to work with vector of shared_ptr rather than vector of raw pointers.
      return new MDDimension(zDimension.getDimensionId());
    }

     Mantid::Geometry::IMDDimension* MDWorkspace::gettDimensionImp() const
    {
     MDDimension& tDimension = m_spImageData->getGeometry()->getTDimension(); //TODO: fix MDGeometry to work with vector of shared_ptr rather than vector of raw pointers.
      return new MDDimension(tDimension.getDimensionId());
    }

} // namespace
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