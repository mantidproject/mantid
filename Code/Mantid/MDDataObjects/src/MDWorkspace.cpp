#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid{
  namespace MDDataObjects{

     //Seam method.
     boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints> getDataPoints(boost::shared_ptr<Mantid::Geometry::MDGeometry> spGeometry, boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> spFile)
     {
       return  boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints>(new MDDataPoints(spGeometry)); //TODO replace with some other factory call.
     }

     //Seam method.
     boost::shared_ptr<Mantid::MDDataObjects::MDImageData> getImageData(boost::shared_ptr<Mantid::Geometry::MDGeometry> spGeometry, boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> spFile)
     {
       return boost::shared_ptr<Mantid::MDDataObjects::MDImageData>(new MDImageData(spGeometry));
     }

     void MDWorkspace::init(boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> spFile, boost::shared_ptr<Mantid::Geometry::MDGeometry> spGeometry) //TODO: this provides a 'seam' for simplier move to DataHandling in future.
     {
       this->m_spFile = spFile;
       this->m_spGeometry = spGeometry;
       this->m_spDataPoints = getDataPoints(m_spGeometry, m_spFile);
       this->m_spImageData = getImageData(m_spGeometry, m_spFile);
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
        throw(std::bad_alloc("read_pix: file reader has not been defined"));
      }


    }

    size_t 
      MDWorkspace::read_pix_selection(const std::vector<size_t> &cells_nums,size_t &start_cell,std::vector<char> &pix_buf,size_t &n_pix_in_buffer)
    {
      if(!this->m_spFile.get()){
        throw(std::bad_alloc("MDPixels::read_selected_pix: file reader has not been defined"));
      }
      return this->m_spFile->read_pix_subset(*m_spImageData,cells_nums,start_cell,pix_buf,n_pix_in_buffer);
    } 

    boost::shared_ptr<Mantid::Geometry::MDGeometry> 
      MDWorkspace::getGeometry() const
    {
      return boost::shared_ptr<Mantid::Geometry::MDGeometry>(new MDGeometry()); //Hack : Should be returning the member geometry.
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
          throw(std::bad_alloc("MDPixels::read_selected_pix: file reader has not been defined"));
      }
    }


    int MDWorkspace::getNPoints() const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::IMDDimension* MDWorkspace::getDimension(std::string id) const
    {
      return this->m_spGeometry->getDimension(id,true);
    }

    Mantid::Geometry::MDPoint * MDWorkspace::getPoint(int index) const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCell(int dim1Increment) const 
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCell(int dim1Increment, int dim2Increment) const 
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCell(int dim1Increment, int dim2Increment, int dim3Increment)  const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCell(int dim1Increment, int dim2Increment, int dim3Increment, int dim4Increment)  const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::MDCell * MDWorkspace::getCell(...)  const
    {
      throw std::runtime_error("Not implemented"); //TODO: implement
    }

    Mantid::Geometry::IMDDimension* MDWorkspace::getXDimension() const
    {
      return & this->m_spGeometry->getXDimension();
    }

    Mantid::Geometry::IMDDimension* MDWorkspace::getYDimension() const
    {
     return & this->m_spGeometry->getYDimension();
    }

    Mantid::Geometry::IMDDimension* MDWorkspace::getZDimension() const
    {
      return & this->m_spGeometry->getZDimension();
    }

     Mantid::Geometry::IMDDimension* MDWorkspace::gettDimension() const
    {
     return & this->m_spGeometry->getTDimension();
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