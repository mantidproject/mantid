#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid{
  namespace MDDataObjects{
	  using namespace Kernel;
	  using namespace Geometry;
	  using namespace API;
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


    //----------------------------------------------------------------------------------------------
    /** Default constructor - does nothing */
    MDWorkspace::MDWorkspace(unsigned int nDimensions, unsigned int nRecDims)
    {
    }

    //----------------------------------------------------------------------------------------------
    void MDWorkspace::read_mdd()
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
      MDWorkspace::read_pix_selection(const std::vector<size_t> &cells_nums, size_t &start_cell, std::vector<char> &pix_buf, size_t &n_pix_in_buffer)
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

      boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::getXDimension() const
      { 
        return m_spImageData->getGeometry()->getXDimension(); 
      }

      boost::shared_ptr< const Mantid::Geometry::IMDDimension> MDWorkspace::getYDimension() const
      { 
        return m_spImageData->getGeometry()->getYDimension();
      }

      boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::getZDimension() const
      { 
        return m_spImageData->getGeometry()->getZDimension();
      }

      boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::gettDimension() const
      { 
        return m_spImageData->getGeometry()->getTDimension();
      }

      boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::getDimension(std::string id) const
      { 
        return m_spImageData->getGeometry()->getDimension(id, true); 
      }

      boost::shared_ptr<const Mantid::Geometry::MDPoint> MDWorkspace::getPoint(int index) const
      { 
        throw std::runtime_error("Not implemented"); //TODO: implement;
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> MDWorkspace::getCell(int dim1Increment) const
      { 
        throw std::runtime_error("Not implemented"); //TODO: implement
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> MDWorkspace::getCell(int dim1Increment, int dim2Increment) const
      { 
        throw std::runtime_error("Not implemented"); //TODO: implement
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> MDWorkspace::getCell(int dim1Increment, int dim2Increment, int dim3Increment) const
      { 
        throw std::runtime_error("Not implemented"); //TODO: implement
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> MDWorkspace::getCell(int dim1Increment, int dim2Increment, int dim3Increment, int dim4Increment) const
      { 
        throw std::runtime_error("Not implemented"); //TODO: implement
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> MDWorkspace::getCell(...) const
      { 
        throw std::runtime_error("Not implemented"); //TODO: implement
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
