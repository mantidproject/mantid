#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include <boost/ptr_container/ptr_vector.hpp>

namespace Mantid{
    namespace MDAlgorithms{
        using namespace Mantid;
        using namespace MDDataObjects;
        using namespace Kernel;
        using namespace API;
        using namespace Geometry;


// Register the class into the algorithm factory
DECLARE_ALGORITHM(CenterpieceRebinning)

CenterpieceRebinning::CenterpieceRebinning(void): API::Algorithm(), m_progress(NULL) 
{}

/** Destructor     */
CenterpieceRebinning::~CenterpieceRebinning()
{
    if( m_progress ){
            delete m_progress;
            m_progress=NULL;
    }
}
void 
CenterpieceRebinning::init_source(MDWorkspace_sptr inputWSX)
{
MDWorkspace_sptr inputWS;
  // Get the input workspace
    if(existsProperty("Input")){
         inputWS = getProperty("Input");
         if(!inputWS){
              throw(std::runtime_error("input workspace has to exist"));
         }

    }else{
       throw(std::runtime_error("input workspace has to be availible through properties"));
    }

   std::string filename;
  // filename = "../../../../Test/VATES/fe_demo.sqw";

   if(existsProperty("Filename")){
      filename= getProperty("Filename");
   }else{
      throw(std::runtime_error("filename property can not be found"));
   }

    inputWS->read_mdd(filename.c_str());

    // set up slicing property to the shape of current workspace;
    MDGeometryDescription *pSlicing = dynamic_cast< MDGeometryDescription *>((Property *)(this->getProperty("SlicingData")));
    if(!pSlicing){
            throw(std::runtime_error("can not obtain slicing property from the property manager"));
     }

     pSlicing->build_from_geometry(*inputWS);
     pSlicing=NULL; // should remain in Property
}
/*
void
CenterpieceRebinning::set_from_VISIT(const std::string &slicing_description,const std::string &XML_definition)
{
    this->slicingProperty.fromXMLstring(slicing_description);
}
*/
void
CenterpieceRebinning::init()
{
      declareProperty(new WorkspaceProperty<MDWorkspace>("Input","",Direction::Input),"initial MD workspace");
      declareProperty(new WorkspaceProperty<MDWorkspace>("Result","",Direction::Output),"final MD workspace");

      declareProperty(new MDPropertyGeometry("SlicingData","",Direction::Input));
      declareProperty(new API::FileProperty("Filename","", API::FileProperty::Load), "The file containing input MD dataset");


      m_progress = new Progress(this,0,1,10);

   
}
//
void 
CenterpieceRebinning::exec()
{
 MDWorkspace_sptr inputWS;
 MDWorkspace_sptr outputWS;

  
  // Get the input workspace
  if(existsProperty("Input")){
        inputWS = getProperty("Input");
        if(!inputWS){
            throw(std::runtime_error("input workspace has to exist"));
        }
  }else{
      throw(std::runtime_error("input workspace has to be accessible through properties"));
  }


  // Now create the output workspace
  if(existsProperty("Result")){
 
     outputWS = getProperty("Result");
     if(!outputWS){
        outputWS      = MDWorkspace_sptr(new MDWorkspace(4));
        setProperty("Result", outputWS);
     }
  }else{
        throw(std::runtime_error("output workspace has to be created "));
  }
  if(inputWS==outputWS){
      throw(std::runtime_error("input and output workspaces have to be different"));
  }

 
  MDPropertyGeometry  *pSlicing; 
  if(existsProperty("SlicingData")){ 
 // get slicing data from property manager. These data has to bebeen shaped to proper form . 
    pSlicing = dynamic_cast< MDPropertyGeometry *>((Property *)(this->getProperty("SlicingData")));
    if(!pSlicing){
                throw(std::runtime_error("can not obtain slicing property from the property manager"));
    }
  }else{
        throw(std::runtime_error("slising property has to exist and has to be defined "));
  }
 
 
  // transform output workspace to the target shape and allocate memory for resulting matrix
  outputWS->alloc_mdd_arrays(*pSlicing);

 

  std::vector<size_t> preselected_cells_indexes;
  size_t  n_precelected_pixels(0);
  // identify MDImageCells which may contribute into cut
  preselect_cells(*inputWS,*pSlicing,preselected_cells_indexes,n_precelected_pixels);
  if(n_precelected_pixels == 0)return;

  unsigned int n_hits = n_precelected_pixels/PIX_BUFFER_SIZE+1;

  size_t    n_pixels_read(0),
            n_pixels_selected(0),
            n_pix_in_buffer(0),pix_buffer_size(PIX_BUFFER_SIZE);
  
  std::vector<char  > pix_buf;
  //TO DO: Give correct pixel size from the workspace method
  pix_buf.resize(PIX_BUFFER_SIZE*sizeof(sqw_pixel));
 

  // get pointer for data to rebin to; 
  MD_image_point*pImage     = outputWS->get_pData();

  // and the number of elements the image has;
  size_t         image_size=  outputWS->getDataSize();
 //
  transf_matrix trf = build_scaled_transformation_matrix(*inputWS,*pSlicing,this->ignore_inf,this->ignore_nan);
// start reading and rebinning;
  size_t n_starting_cell(0);
  for(unsigned int i=0;i<n_hits;i++){
      n_starting_cell  += inputWS->read_pix_selection(preselected_cells_indexes,n_starting_cell,pix_buf,n_pix_in_buffer);
      n_pixels_read    += n_pix_in_buffer;
      
      n_pixels_selected+= rebin_Nx3dataset(trf,&pix_buf[0],n_pix_in_buffer,*outputWS);
  } 
  finalise_rebinning(pImage,image_size);

  pix_buf.clear();


}
//

void 
CenterpieceRebinning::set_from_VISIT(const std::string &slicing_description_in_hxml,const std::string &definition)
{  

//double originX, originY, originZ, normalX, normalY, normalZ;
/*
Mantid::API::ImplicitFunction* ifunc = Mantid::API::Instance().ImplicitFunctionFactory(xmlDefinitions, xmlInstructions);
  PlaneImplicitFunction* plane = dynamic_cast<PlaneImplicitFunction*>(ifunc);

for(int i = 0; i < compFunction->getNFunctions() ; i++)
{

  ImplicitFunction* nestedFunction =   compFunction->getFunction().at(i).get();
  PlaneImplicitFunction* plane = dynamic_cast<PlaneImplicitFunction*>(nestedFunction);
  if(NULL != plane)
  {
    originX = plane->getOriginX();
    originY = plane->getOriginY();
    originZ = plane->getOriginZ();
    normalX = plane->getNOrmalX();
    normalY = plane->getNormalY();
    normalZ = plane->getNOrmalX();
    break;
  }
}
*/
}

} //namespace MDAlgorithms
} //namespace Mantid