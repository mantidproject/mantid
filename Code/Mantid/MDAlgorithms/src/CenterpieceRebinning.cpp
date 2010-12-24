#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include <sstream>
// Availble rebinning methods: Eventually will go to factory
#include "MantidMDAlgorithms/CpRebinningNx3.h"
#include "MantidMDAlgorithms/CpRebinning4x3StructHR.h"

namespace Mantid{
    namespace MDAlgorithms{
        using namespace Mantid;
        using namespace MDDataObjects;
        using namespace Kernel;
        using namespace API;
        using namespace Geometry;


// Register the class into the algorithm factory
DECLARE_ALGORITHM(CenterpieceRebinning)

CenterpieceRebinning::CenterpieceRebinning(void): API::Algorithm() 
{}

/** Destructor     */
CenterpieceRebinning::~CenterpieceRebinning()
{
 
}
void 
CenterpieceRebinning::init_slicing_property()
{
    // input workspace has to exist and be loaded;
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


    // set up slicing property to the shape of current workspace;
    MDGeometryDescription *pSlicing = dynamic_cast< MDGeometryDescription *>((Property *)(this->getProperty("SlicingData")));
    if(!pSlicing){
            throw(std::runtime_error("can not obtain slicing property from the property manager"));
     }

    pSlicing->build_from_geometry(*(inputWS->getGeometry()));
    //pSlicing=NULL; // should remain in Property
 
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

  // prospective property does nothing at the moment TODO: enable
    declareProperty(new Kernel::PropertyWithValue<bool>("KeepPixels",false,Direction::Input),
        " This property specifies if user wants to keep"
        " all pixels(events) contributing in the target MD workspace during rebinning operation; This is to accelerate work if the user sure that he wants"
        " to save the workspace after rebinning. If he does not specify this option, a rebinning which keeps contributing pixels will be performed"
        " when user decides to save the final multidimensional workspace. DISABLED AT THE MOMENT" );	
   
}

//
void 
CenterpieceRebinning::exec()
{
 MDWorkspace_sptr inputWS;
 MDWorkspace_sptr outputWS;

   if(existsProperty("Input")){
        inputWS = this->getProperty("Input");
        if(!inputWS){
              throw(std::runtime_error("input workspace has to exist"));
        }
   }else{
       throw(std::runtime_error("input workspace has to be availible through properties"));
   }


   MDPropertyGeometry  *pSlicing; 
   if(existsProperty("SlicingData")){ 
 // get slicing data from property manager. At this stage the data has to be shaped to the form desribing the final resulting cut
    pSlicing = dynamic_cast< MDPropertyGeometry *>((Property *)(this->getProperty("SlicingData")));
    if(!pSlicing){
                throw(std::runtime_error("can not obtain slicing property from the property manager"));
    }
  }else{
        throw(std::runtime_error("slising property has to exist and has to be defined "));
  }

  // Now create the output workspace or get the one which is ready for this purpose;
  if(existsProperty("Result")){
 
     outputWS = getProperty("Result");
     if(!outputWS){
        outputWS      = MDWorkspace_sptr(new MDWorkspace());
        // this adds workspace to dataservice
        setProperty("Result", outputWS);
     }else{

     }
  }else{
        throw(std::runtime_error("output workspace has to be created "));
  }
  if(inputWS==outputWS){
      throw(std::runtime_error("input and output workspaces have to be different"));
  }

   // here we should have the call to factory, providing best rebinning method for the job
   std::auto_ptr<IDynamicRebinning> pRebin = std::auto_ptr<IDynamicRebinning>(new CpRebinningNx3(inputWS,pSlicing,outputWS));
  //  std::auto_ptr<IDynamicRebinning> pRebin = std::auto_ptr<IDynamicRebinning>(new CpRebinning4x3StructHR(inputWS,pSlicing,outputWS));
  
    bool selection_valid(false);
    // indicate cells, which may contribute into cut
    size_t  n_precelected_cells = pRebin->preselect_cells();
    if(n_precelected_cells==0)return;

    // find out how many steps is needed to take to make the cut
    unsigned int nSteps = pRebin->getNumDataChunks();

   /// The progress reporting object
    std::auto_ptr<API::Progress> pProgress;
    if(nSteps>1){
      pProgress = std::auto_ptr<API::Progress>(new Progress(this,0,1,nSteps));
    }

    selection_valid = true;

    // to the cut reporting the progress
    unsigned int ic(0);
    std::stringstream message_buf;
    while(selection_valid){
        selection_valid=pRebin->rebin_data_chunk();

        if(pProgress.get()){
            message_buf<<"Making cut; step "<<ic<<" out of: "<<nSteps<<std::endl;
            pProgress->report(ic,message_buf.str());
            // check if canceled
            //if(this->c
        }
        ic++;
    }
    // calculate necessary statistical properties of the cut
    pRebin->finalize_rebinning();





}
//



} //namespace MDAlgorithms
} //namespace Mantid