#include "MantidMDAlgorithms/CenterpieceRebinning.h"
#include <sstream>
// Availble rebinning methods: Eventually will go to factory
#include "MantidMDAlgorithms/CpRebinningNx3.h"

namespace Mantid{
    namespace MDAlgorithms{
        using namespace Mantid;
        using namespace MDDataObjects;
        using namespace Kernel;
        using namespace API;
        using namespace Geometry;


// Register the class into the algorithm factory
DECLARE_ALGORITHM(CenterpieceRebinning)

Kernel::Logger& CenterpieceRebinning::bin_log=Kernel::Logger::get("MD rebinning Operations");

CenterpieceRebinning::CenterpieceRebinning(void): API::Algorithm() 
{}

/** Destructor     */
CenterpieceRebinning::~CenterpieceRebinning()
{
 
}
void 
CenterpieceRebinning::setTargetGeomDescrEqSource()
{
    // input workspace has to exist and be loaded;
	MDWorkspace_sptr inputWS;
  // Get the input workspace
    if(existsProperty("Input")){
         inputWS = getProperty("Input");
         if(!inputWS){
             bin_log.error()<<"Can not identify initial workspace to do rebinning from\n";
             throw(std::runtime_error("input workspace has to exist"));
         }

    }else{
       bin_log.error()<<"Input workspace has not been defined in properties\n";
       throw(std::runtime_error("input workspace has to be availible through properties"));
    }


    // set up slicing property to the shape of current workspace;
    MDGeometryDescription *pSlicing = dynamic_cast< MDGeometryDescription *>((Property *)(this->getProperty("SlicingData")));
    if(!pSlicing){
          bin_log.error()<<"Rebinning request can not be retrieved from properties\n";
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
  
      declareProperty(new Kernel::PropertyWithValue<bool>("KeepPixels",false,Direction::Input),
        " This property specifies if user wants to keep"
        " all pixels(events) contributing in the target MD workspace during rebinning operation; This is to accelerate work if the user sure that he wants"
        " to save the workspace after rebinning. If he does not specify this option, a rebinning which keeps contributing pixels will be performed"
        " when user decides to save the final multidimensional workspace" );	
   
}

unsigned int CenterpieceRebinning::reportOccurance(const unsigned int nSteps)
{
  return nSteps; //Determine any other report occurance desired.
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
            bin_log.error()<<"Can not identify initial workspace to do rebinning from\n";
            throw(std::runtime_error("input workspace has to exist"));
        }
   }else{
      bin_log.error()<<"Input workspace has not been defined in properties\n";
       throw(std::runtime_error("input workspace has to be availible through properties"));
   }


   MDPropertyGeometry  *pSlicing; 

 // get slicing data from property manager. At this stage the data has to be shaped to the form desribing the final resulting cut
   pSlicing = dynamic_cast< MDPropertyGeometry *>((Property *)(this->getProperty("SlicingData")));
   if(!pSlicing){
         bin_log.error()<<"Rebinning request can not be retrieved from properties manager\n";
         throw(std::runtime_error("can not obtain slicing property from the property manager"));
   }
 
  // Now create the output workspace or get the one which is ready for this purpose;
 
     outputWS = getProperty("Result");
     std::string ws_name = this->getPropertyValue("Result");
     if(!outputWS){
         bin_log.information()<<" new target MD Worokspace "<<ws_name<<" will be created\n";
         outputWS      = MDWorkspace_sptr(new MDWorkspace());
        // this adds workspace to dataservice
        setProperty("Result", outputWS);
     }else{
         bin_log.information()<<" Target MD Wororkspace "<<ws_name<<" will be owerwritten\n";
         Workspace_sptr result = AnalysisDataService::Instance().retrieve(ws_name);
         outputWS = boost::dynamic_pointer_cast<MDWorkspace>(result);
         if(outputWS.get()==NULL){
             bin_log.error()<<" Can not retrieve workspace "<<ws_name<<" from Analysis data service or it is not a multidimensional workspace\n";
             throw(std::runtime_error(" Can not get any or proper kind of workspace from Abalysis data service"));
         }
    }
    if(inputWS==outputWS){
      bin_log.error()<<" input and output workspace have to be different do to rebinnning\n";
      throw(std::runtime_error("input and output workspaces have to be different"));
    }
    bool keep_pixels(false);
    keep_pixels = getProperty("KeepPixels");

   // here we should have the call to factory, providing best rebinning method for the job
   std::auto_ptr<IDynamicRebinning> pRebin = std::auto_ptr<IDynamicRebinning>(new CpRebinningNx3(inputWS,pSlicing,outputWS,keep_pixels));
  //  std::auto_ptr<IDynamicRebinning> pRebin = std::auto_ptr<IDynamicRebinning>(new CpRebinning4x3StructHR(inputWS,pSlicing,outputWS));
  
    bool selection_valid(false);
    // indicate cells, which may contribute into cut
    size_t  n_precelected_cells = pRebin->preselect_cells();
    if(n_precelected_cells==0)return;

    // find out how many steps is needed to take to make the cut
    unsigned int nSteps = pRebin->getNumDataChunks();

   /// The progress reporting object
    int occurance = reportOccurance(nSteps);
    std::auto_ptr<API::Progress> pProgress;
    if(nSteps>1){
      pProgress = std::auto_ptr<API::Progress>(new Progress(this,0,1,occurance));
    }

    selection_valid = true;

    // counter for the rebinning reporting the progress
    unsigned int ic(0);
    std::stringstream message_buf;
    while(selection_valid){
        if(keep_pixels){
            selection_valid=pRebin->rebin_data_chunk_keep_pixels();
        }else{
            selection_valid=pRebin->rebin_data_chunk();
        }
        if(pProgress.get()){
            message_buf<<"Making cut; step "<<ic<<" out of: "<<nSteps<<std::endl;
            pProgress->report(ic,message_buf.str());
            message_buf.seekp(std::ios::beg);
        }
        ic++;
    }
    // calculate necessary statistical properties of the cut
    pRebin->finalize_rebinning();


}
//



} //namespace MDAlgorithms
} //namespace Mantid
