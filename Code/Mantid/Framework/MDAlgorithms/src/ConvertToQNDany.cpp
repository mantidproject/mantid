/*WIKI* 


*WIKI*/
#include "MantidMDAlgorithms/ConvertToQNDany.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/NumericAxis.h"

#include "MantidKernel/PhysicalConstants.h"

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ProgressText.h"
#include "MantidAPI/Progress.h"

#include "MantidMDEvents/MDEvent.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"

#include <float.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
//using namespace Mantid::MDEvents;

namespace Mantid
{
namespace MDAlgorithms
{
// logger for loading workspaces  
   Kernel::Logger& ConvertToQNDany::convert_log =Kernel::Logger::get("MD-Algorithms");


// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToQNDany)
  
/// Sets documentation strings for this algorithm
void ConvertToQNDany::initDocs()
{
    this->setWikiSummary("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, then events are added to it.");
    this->setOptionalMessage("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, then events are added to it.");
}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
ConvertToQNDany::ConvertToQNDany():
n_activated_dimensions(2),
Q_ID_possible(2)
{
    Q_ID_possible[0]="|Q|";
    Q_ID_possible[1]="QxQyQz";
  
}
    
//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToQNDany::~ConvertToQNDany()
{
}


//
const double rad2deg = 180.0 / M_PI;
/// helper function to create empty MDEventWorkspace with nd dimensions 
template<size_t nd>
boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> > create_emptyNDEventWS(const std::string dimensionNames[nd],const std::string dimensionUnits[nd],
                                                                              const std::vector<double> &dimMin,const std::vector<double> &dimMax)
{

       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> > ws = 
       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> >(new MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd>());
    
      // Give all the dimensions
      for (size_t d=0; d<nd; d++)
      {
        MDHistoDimension * dim = new MDHistoDimension(dimensionNames[d], dimensionNames[d], dimensionUnits[d], dimMin[d], dimMax[d], 10);
        ws->addDimension(MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller
      Mantid::API::BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(5);
//      bc->setSplitThreshold(1500);
      bc->setSplitThreshold(10);
      bc->setMaxDepth(20);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
      return ws;
}


//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void 
ConvertToQNDany::init()
{
      CompositeWorkspaceValidator<> *ws_valid = new CompositeWorkspaceValidator<>;
      ws_valid->add(new HistogramValidator<>);
      ws_valid->add(new InstrumentValidator<>);
      // the validator which checks if the workspace has axis and any units
      ws_valid->add(new WorkspaceUnitValidator<>(""));


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace 2D, processed by Convert to energy (homer) algorithm and its x-axis has to be in the units of energy transfer with energy in mev.");
    BoundedValidator<int> *min_nDim = new BoundedValidator<int>();

     declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace. If the workspace already exists, then the events will be added to it.");
  
    declareProperty("QDimensions",Q_ID_possible[0],new ListValidator(Q_ID_possible),
                              "You can select mod(Q) (1 dimension) or QxQyQz (3 dimensions) in Q space",Direction::InOut);        
    min_nDim->setLower(0);
    declareProperty(new PropertyWithValue<int>("NumAddDim",3,min_nDim,Direction::Input),
        " Number of additional to Q (orthogonal) dimensions in the target workspace");

// -------- Dynamic properties;
    // build properties for possible additional dimensions
      build_default_properties(8);

//    declareProperty(new ArrayProperty<std::string>("dim1"),

    // BoundedValidator<double> *minEn = new BoundedValidator<double>();
    // minEn->setLower(0);
    // declareProperty("EnergyInput",0.000000001,minEn,"The value for the incident energy of the neutrons leaving the source (meV)",Direction::InOut);

    // // this property is mainly for subalgorithms to set-up as they have to identify 
    //declareProperty(new PropertyWithValue<bool>("UsePreprocessedDetectors", true, Direction::Input), 
    //    "Store the part of the detectors transfromation into reciprocal space to save/reuse it later;");
    //// // this property is mainly for subalgorithms to set-up as they have to identify 
    ////declareProperty(new ArrayProperty<double>("u"), "first base vecotor");
    ////declareProperty(new ArrayProperty<double>("v"), "second base vecotors");


    // declareProperty(new ArrayProperty<double>("MinQdE_values"),
    //     "An array containing minimal values for Q[A^-1] and energy transfer[meV] in a form qx_min,qy_min,qz_min, dE min\n"
    //     "(momentum and energy transfer values lower that this one will be ignored if this is set.\n"
    //     " If a minimal output workspace range is higer then specified, the workspace range will be used intstead)" );

    // declareProperty(new ArrayProperty<double>("MaxQdE_values"),
    //     "An array containing maximal values for Q[A^-1] and energy transfer[meV] in a form qx_max,qy_max,qz_max, dE_max\n"
    //     "(momentum and energy transfer values higher that this one will be ignored if this is set.\n"
    //     " If a maximal output workspace ranges is lower, then one of specified, the workspace range will be used instead)" );


}
//
void
ConvertToQNDany::setPropertyValue(const std::string &name, const std::string &value){
        if(name.compare("Num_dimensions")){

        }
        API::Algorithm::PropertyManagerOwner::setProperty(name,value);
};

std::vector<std::string > 
ConvertToQNDany::get_dimension_names(const std::vector<std::string> &default_prop,MatrixWorkspace_const_sptr inMatrixWS)const{
    // number of properties we always want to have, it is Q3D, |Q| and some property form workspace;
    // if workspace unit is the energy transfer, it is a special property;
    size_t n_common_properties = default_prop.size();

    const std::vector< Kernel::Property * > run_properties =  inMatrixWS->run().getProperties();  

    std::vector<std::string> prop_names(default_prop.begin(),default_prop.end());
    prop_names.resize(n_common_properties+run_properties.size());
    
 // inelastic workspaces need special treatment, their own subalgorithms and have additional dimension
   // let's identify if it is inelastic ws
    Kernel::Unit_const_sptr unit = inMatrixWS->getAxis(0)->unit();
    if (unit){
        std::string ws_property_name = unit->unitID();
        std::vector<std::string >::iterator it;
        it = find (prop_names.begin(), prop_names.end(), "DeltaE"); // can not fail, DeltaE is in the list of the properties;
        *it          = ws_property_name;
    }else{
        convert_log.error()<<" input workspace has to have units\n";
        throw(std::invalid_argument(" input worspace has to have units"));
    }

    // extract all properties, which can be treated as dimension names;
    for(size_t i=0;i<run_properties.size();i++){
        prop_names[n_common_properties+i]=run_properties[i]->name();
    }
    return prop_names;
}

bool
ConvertToQNDany::   build_default_properties(size_t max_n_dims)
{ 
    // number of additional known dimensions:
    size_t n_add_dims =boost::lexical_cast<size_t>(getPropertyValue("NumAddDim"));
    // number of total dimensions is also defined by the value of the Q -dimensions
    std::string dim_id =   getPropertyValue("QDimensions");
    // check if the number of dimensions have not changed and new input is needed
    size_t n_dims;
    if    (dim_id.compare(Q_ID_possible[0])){ // |Q|    -- 1 dimension
        n_dims = 1;
    }else{                                 // QxQyQz -- 3 dimensions     
        n_dims = 3;
    }
//    size_t n_dim_visible  = n_dims+n_add_dims;
//    size_t n_dim_invisible= max_n_dims-n_dim_visible;

    std::vector<std::string> dim_ID(max_n_dims);
    dim_ID[0]=std::string("DeltaE");
    for(size_t i=1;i<max_n_dims;i++){
        std::string num     = boost::lexical_cast<std::string>(i+1);
        std::string dim_num = "dim_"+num; 
        dim_ID[i] = dim_num;
    }
    for(size_t i=0;i<max_n_dims;i++){
          if(this->existsProperty(dim_ID[i])) this->removeProperty(dim_ID[i]);         
          this->declareProperty(dim_ID[i],dim_ID[i],new ListValidator(dim_ID),"",Direction::InOut);        
          this->setPropertySettings(dim_ID[i], new EnabledWhenProperty(this, "NumAddDim", IS_MORE_OR_EQ, boost::lexical_cast<std::string>(i)));
    }   
   return true; 
}
//
void 
ConvertToQNDany::build_ND_property_selector(size_t n_dims,const std::vector<std::string> & dim_ID)
{

    if(dim_ID.size()<n_dims){
        convert_log.error()<<" number of dimensions requested="<<n_dims<<" and this has to be less or equal to the number of possible workspace variables"<<dim_ID.size()<<std::endl;
        throw(std::invalid_argument(" nuber of dimensions exceed the possible dimension number"));
    }

   
}



void 
ConvertToQNDany::check_max_morethen_min(const std::vector<double> &min,const std::vector<double> &max){
    for(size_t i=0; i<min.size();i++){
        if(max[i]<=min[i]){
            convert_log.error()<<" min value "<<min[i]<<" not less then max value"<<max[i]<<" in direction: "<<i<<std::endl;
            throw(std::invalid_argument("min limit not smaller then max limit"));
        }
    }
}
 

 
  //----------------------------------------------------------------------------------------------
  /* Execute the algorithm.   */
void ConvertToQNDany::exec(){

        return;
   
}


} // namespace Mantid
} // namespace MDAlgorithms

