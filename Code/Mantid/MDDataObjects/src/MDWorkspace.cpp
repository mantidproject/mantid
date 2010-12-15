#include "MDDataObjects/stdafx.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MantidKernel/IPropertyManager.h"


namespace Mantid{
  namespace MDDataObjects{
	  using namespace Kernel;
	  using namespace Geometry;
	  using namespace API;


    // Register the workspace into the WorkspaceFactory
    DECLARE_WORKSPACE(MDWorkspace)


    // logger for MD workspaces  
    Kernel::Logger& MDWorkspace::g_log =Kernel::Logger::get("MDWorkspaces");


    //----------------------------------------------------------------------------------------------
     //Seam method.
     boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints> getDataPoints(boost::shared_ptr<MDImage> imageData)
     {
	   MDPointDescription descr;
       return  boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints>(new MDDataPoints(imageData,descr)); //TODO replace with some other factory call.
     }

     //Seam method.
     boost::shared_ptr<Mantid::MDDataObjects::MDImage> getImageData(Mantid::Geometry::MDGeometry* geometry)
     {
       return boost::shared_ptr<Mantid::MDDataObjects::MDImage>(new MDImage(geometry));
	 }
//
void 
MDWorkspace::load_workspace(boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> spFile)
{
	this->m_spFile = spFile;
	this->m_spMDBasis = boost::shared_ptr<Geometry::MDGeometryBasis>(new Geometry::MDGeometryBasis());
	this->m_spFile->read_basis(*m_spMDBasis);

	// read the geometry description
	MDGeometryDescription description(m_spMDBasis->getNumDims(),m_spMDBasis->getNumReciprocalDims());
	this->m_spFile->read_MDGeomDescription(description);

	//build image from basis and description
	this->m_spMDImage  = boost::shared_ptr<MDImage>(new MDImage(description,*m_spMDBasis));

	// obtain the MDPoint description now (and MDPointsDescription in a future)
	MDPointDescription pd = this->m_spFile->read_pointDescriptions();

	// temporary constructor --> have not been finished and ugly
	this->m_spDataPoints = boost::shared_ptr<MDDataPoints>(new MDDataPoints(m_spMDImage,pd));

    //  read image part of the data
     this->m_spFile->read_MDImg_data(*this->m_spMDImage);
	 // need to be moved to to dataPoints;
	 this->m_spMDImage->identify_SP_points_locations();
   
}
//
     void MDWorkspace::init(boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> spFile, Mantid::Geometry::MDGeometry* geometry) //TODO: this provides a 'seam' for simplier move to DataHandling in future.
     {
       this->m_spFile = spFile;
       this->m_spMDImage = getImageData(geometry);
       this->m_spDataPoints = getDataPoints(m_spMDImage); //Takes a pointer to the image data in order to be able to extract an up-to-date geometry.
     }

//
void
MDWorkspace::init(boost::shared_ptr<const MDWorkspace> SourceWorkspace,const Mantid::Geometry::MDGeometryDescription *const transf)
{
	this->m_spMDBasis = boost::shared_ptr<MDGeometryBasis>(new MDGeometryBasis(SourceWorkspace->get_const_MDBaisis()));

	// build old or new geometry
	std::auto_ptr<MDGeometry> pGeometry;
	// no changes to new workspace is defined and we are initiating the new workspace as a copy of an old workspace;
	if(!transf){
		std::auto_ptr<MDGeometryDescription> oldShape = std::auto_ptr<MDGeometryDescription>(new MDGeometryDescription(*SourceWorkspace->getGeometry()));
		// we have basis and description, now can build geometry
		pGeometry = std::auto_ptr<MDGeometry>(new MDGeometry(*m_spMDBasis,*oldShape));
	}else{
		pGeometry = std::auto_ptr<MDGeometry>(new MDGeometry(*m_spMDBasis,*transf));
	}
	//
   this->m_spMDImage = boost::shared_ptr<MDImage>(new MDImage(pGeometry.get()));
   // free the pGeometry as it is now resides with MDImage and should not be deleted by auto_ptr;
   pGeometry.release();

   //TODO: 
   // MDDataPoints have to be constructed here and intiated later after the image is build and points need to be saved
   // fileManager has to be initated for writing workspace but this will happens only when saveWorkspace algorithm is 
   // called and new file name is known. Temporary file manager has to be created if and when datapoint writing is necessary;

}

    /** Default constructor - does nothing */
    MDWorkspace::MDWorkspace(unsigned int nDimensions, unsigned int nRecDims)
    {
    }

    //----------------------------------------------------------------------------------------------
    void MDWorkspace::read_MDImg()
    {
      Geometry::MDGeometryDescription Description;
	  this->m_spFile->read_MDGeomDescription(Description);
	  //
	  this->m_spMDImage->initialize(Description);
      //  read image part of the data
      this->m_spFile->read_MDImg_data(*this->m_spMDImage);
      // alocate memory for pixels;
      m_spDataPoints->alloc_pix_array(m_spFile);
      m_spMDImage->identify_SP_points_locations();
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
      return this->m_spFile->read_pix_subset(*m_spMDImage,cells_nums,start_cell,pix_buf,n_pix_in_buffer);
    } 

    Mantid::Geometry::MDGeometry const * const
      MDWorkspace::getGeometry() const
    {
      return this->m_spMDImage->getGeometry();
    }

    long MDWorkspace::getMemorySize(void) const
    {
      return m_spMDImage->getMemorySize() + m_spDataPoints->getMemorySize() ;
    } 

    void  MDWorkspace::write_mdd(void)
    {
      if(this->m_spFile.get()){
         this->m_spFile->write_mdd(*m_spMDImage);
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
      return m_spMDImage->getGeometry()->getXDimension(); 
    }

    boost::shared_ptr< const Mantid::Geometry::IMDDimension> MDWorkspace::getYDimension() const
    { 
      return m_spMDImage->getGeometry()->getYDimension();
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::getZDimension() const
    { 
      return m_spMDImage->getGeometry()->getZDimension();
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::gettDimension() const
    { 
      return m_spMDImage->getGeometry()->getTDimension();
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDWorkspace::getDimension(std::string id) const
    { 
		return m_spMDImage->getGeometry()->get_constDimension(id,true); 
    }

    const Mantid::Geometry::SignalAggregate & MDWorkspace::getPoint(int index) const
    {
      throw std::runtime_error("Not implemented"); //TODO
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(int dim1Increment) const
    {
      throw std::runtime_error("Not implemented"); //TODO
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(int dim1Increment, int dim2Increment) const
    {
      throw std::runtime_error("Not implemented"); //TODO
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(int dim1Increment, int dim2Increment, int dim3Increment) const
    {
      throw std::runtime_error("Not implemented"); //TODO
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(int dim1Increment, int dim2Increment, int dim3Increment, int dim4Increment) const
    {
      throw std::runtime_error("Not implemented"); //TODO
    }

    const Mantid::Geometry::SignalAggregate& MDWorkspace::getCell(...) const
    {
      throw std::runtime_error("Not implemented"); //TODO
    }

    std::string MDWorkspace::getWSLocation() const 
    {
      //Forward request to file format.
      return this->m_spFile->getFileName();
    }

    std::string MDWorkspace::getGeometryXML() const
    {
      //Forward request via image and geometry.
      return this->m_spMDImage->getGeometry()->toXMLString();
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
