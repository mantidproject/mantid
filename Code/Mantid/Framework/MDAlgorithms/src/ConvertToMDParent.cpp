
#include "MantidMDAlgorithms/ConvertToMDParent.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
//
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"

#include "MantidMDEvents/MDWSTransform.h"
//

#include "MantidMDEvents/ConvToMDSelector.h"


using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;
using namespace Mantid::MDEvents::CnvrtToMD;

namespace Mantid
{
namespace MDAlgorithms
{


/// Algorithm's category for identification. @see Algorithm::category
const std::string ConvertToMDParent::category() const { return "MDAlgorithms";}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToMDParent::init()
{
      auto ws_valid = boost::make_shared<CompositeValidator>();
      //
      ws_valid->add<InstrumentValidator>();
      // the validator which checks if the workspace has axis and any units
      ws_valid->add<WorkspaceUnitValidator>("");


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace (2DMatrix or Event workspace) ");
   

     std::vector<std::string> Q_modes = MDEvents::MDTransfFactory::Instance().getKeys();
     // something to do with different moments of thime when algorithm or test loads library. To avoid empty factory always do this. 
     if(Q_modes.empty()) Q_modes.assign(1,"ERROR IN LOADING Q-converters");
  
     /// this variable describes default possible ID-s for Q-dimensions   
     declareProperty("QDimensions",Q_modes[0],boost::make_shared<StringListValidator>(Q_modes),
"String, describing available analysis modes, registered with [[MD Transformation factory]].\n "
"There are 3 modes currently available and described in details on [[MD Transformation factory]] page. \n"
"The modes names are '''CopyToMD''', '''mod<nowiki>|Q|</nowiki>''' and '''Q3D'''",
                     Direction::InOut);
     /// temporary, untill dEMode is not properly defined on Workspace
     std::vector<std::string> dE_modes = Kernel::DeltaEMode().availableTypes();
     declareProperty("dEAnalysisMode",dE_modes[Kernel::DeltaEMode::Direct],boost::make_shared<StringListValidator>(dE_modes),
"You can analyse neutron energy transfer in '''Direct''', '''Indirect''' or '''Elastic''' mode. \n"
"The analysis mode has to correspond to experimental set up. Selecting inelastic mode increases \n"
"the number of the target workspace dimensions by one. See [[MD Transformation factory]] for further details.",
                     Direction::InOut);

    MDEvents::MDWSTransform QSclAndFrames;
    std::vector<std::string> TargFrames = QSclAndFrames.getTargetFrames();
    declareProperty("Q3DFrames", TargFrames[CnvrtToMD::AutoSelect],boost::make_shared<StringListValidator>(TargFrames),
      "What will be the Q-dimensions of the output workspace in Q3D case?\n"
      "   AutoSelect: Choose the target coordinate frame as the function of goniometer and UB matrix values set on the input workspace\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Wave-vector change of the lattice in the frame of the sample (taking out goniometer rotation).\n"
      "  HKL: Use the sample's UB matrix to convert to crystal's HKL indices.\n"
      "See [[ MD_Transformation_factory#Q3D |MD Transformation factory]] for more details about this. "
       );


     std::vector<std::string> QScales = QSclAndFrames.getQScalings();
     declareProperty("QConversionScales",QScales[CnvrtToMD::NoScaling], boost::make_shared<StringListValidator>(QScales),
"This property to normalize three momentums obtained in '''Q3D''' mode. See [[MD Transformation factory]] "
"for description and available scaling modes.\n The value can be modified depending on the target coordinate "
"system, defined by the property '''OutputDimensions'''. "
   );


     setPropertySettings("Q3DFrames",new Kernel::VisibleWhenProperty("QDimensions",IS_EQUAL_TO,"Q3D"));
     setPropertySettings("QconversionScales",new Kernel::VisibleWhenProperty("QDimensions",IS_EQUAL_TO,"Q3D"));


     
    declareProperty(new ArrayProperty<std::string>("OtherDimensions",Direction::Input),
"List(comma separated) of additional to '''Q''' and '''DeltaE''' variables which form additional "
"(orthogonal) to '''Q''' dimensions in the target workspace (e.g. Temperature or Magnetic field).\n "
"These variables had to be logged during experiment and the names of these variables have to coincide "
"with the log names for the records of these variables in the source workspace."
                    );

    // this property is mainly for ChildAlgorithms to set-up as they have to identify if they use the same instrument. 
    declareProperty(new PropertyWithValue<std::string>("PreprocDetectorsWS","PreprocessedDetectorsWS",Direction::Input), 
"The name of the table workspace where the part of the detectors transformation into reciprocal space, "
"calculated by [[PreprocessDetectorsToMD]] algorithm stored. If the workspace is not found in analysis "
"data service, [[PreprocessDetectorsToMD]] used to calculate it. If found, the algorithm uses existing "
"workspace. The field is useful if one expects to analyse number of different experiments obtained on "
"the same instrument. <span style=\"color:#FF0000\"> Dangerous if one uses number of workspaces with "
"modified derived instrument one after another. </span> '''In this case this property has to be set to "
"<span style=\"color:#FF0000\">\"-\"</span> sting (without quotes) or empty (possible from script only) "
"to force the workspace recalculation each time the algorithm is invoked'''"
                    );

    declareProperty(new PropertyWithValue<bool>("UpdateMasks", false, Direction::Input),
"if PreprocessDetectorWS is used to build the workspace with preprocessed detectors at first algorithm "
"call and the input workspaces instruments are different by just different masked detectors, setting this "
"option to true forces [[PreprocessDetectorsToMD]] update only the detectors masks for all subsequent "
"calls to this algorithm. <span style=\"color:#FF0000\">This is temporary solution necessary until Mantid "
"masks spectra by 0 rather then by NaN</span> "
                    );

    // if one needs to use Lorentz corrections
    declareProperty(new PropertyWithValue<bool>("LorentzCorrection", false, Direction::Input),
"Correct the weights of events or signals and errors transformed into reciprocal space by multiplying them "
"by the Lorentz multiplier:\n <math>sin(\\theta)^2/\\lambda^4</math>. Currently works in Q3D Elastic case only "
"and is ignored in any other case."
                    );
    declareProperty(new PropertyWithValue<bool>("IgnoreZeroSignals", false, Direction::Input),
 "Enabling this property forces the algorithm to ignore bins with zero signal for an input matrix workspace. Input event workspaces are not affected. "
 "This violates the data normalization but may substantially accelerate calculations in situations when the normalization is not important (e.g. peak finding)."
      );
    
    declareProperty(new ArrayProperty<double>("Uproj"),
//"The functionality of this parameter set to non-default value is still not fully verified (see ticket #5982). "
"Defines the first projection vector of the target Q coordinate system in '''Q3D''' mode - Default (1,0,0)");

    declareProperty(new ArrayProperty<double>("Vproj"),
//"The functionality of this parameter set to non-default value is still not fully verified (see ticket #5982). "
"Defines the second projection vector of the target Q coordinate system in '''Q3D''' mode - Default (0,1,0).");

    declareProperty(new ArrayProperty<double>("Wproj"),
//"The functionality of this parameter set to non-default value is still not fully verified (see ticket #5982). "
"Defines the third projection vector of the target Q coordinate system in '''Q3D''' mode. - Default (0,0,1)");
 
}
/**The method responsible for analyzing input workspace parameters and preprocessing detectors positions into reciprocal space
 *
 * @param InWS2D -- input Matrix workspace with defined instrument
 * @param dEModeRequested -- energy conversion mode (direct/indirect/elastic)
 * @param updateMasks  --  if full detector positions calculations or just update masking requested
 * @param OutWSName    -- the name for the preprocessed detectors workspace to have in the analysis data service
 * 
 * @return          shared pointer to the workspace with preprocessed detectors information. 
 */
DataObjects::TableWorkspace_const_sptr ConvertToMDParent::preprocessDetectorsPositions( Mantid::API::MatrixWorkspace_const_sptr InWS2D,const std::string &dEModeRequested,
                                                                                       bool updateMasks, const std::string & OutWSName)
{

    DataObjects::TableWorkspace_sptr TargTableWS;
    Kernel::DeltaEMode::Type Emode;

    // Do we need to reuse output workspace
    bool storeInDataService(true);
    std::string tOutWSName(OutWSName);
    if(tOutWSName=="-"||tOutWSName.empty()) // TargTableWS is recalculated each time;
    {
      storeInDataService = false;
      tOutWSName = "ServiceTableWS";  // TODO: should be hidden?
    }
    else
    {
      storeInDataService = true;
    }

     // if output workspace exists in dataservice, we may try to use it
    if(storeInDataService && API::AnalysisDataService::Instance().doesExist(tOutWSName) ) 
    {
        TargTableWS = API::AnalysisDataService::Instance().retrieveWS<DataObjects::TableWorkspace>(tOutWSName);
        // get number of all histograms (may be masked or invalid)
        size_t nHist = InWS2D->getNumberHistograms();
        size_t nDetMap=TargTableWS->rowCount();
        if(nHist==nDetMap)
        {
          // let's take at least some precaution to ensure that instrument have not changed
          std::string currentWSInstrumentName = InWS2D->getInstrument()->getName();
          std::string oldInstrName            = TargTableWS->getLogs()->getPropertyValueAsType<std::string>("InstrumentName");

          if(oldInstrName==currentWSInstrumentName)
          { 
            if(!updateMasks) return TargTableWS;
            //Target workspace with preprocessed detectors exists and seems is correct one. 
            // We still need to update masked detectors information
            TargTableWS = this->runPreprocessDetectorsToMDChildUpdatingMasks(InWS2D,tOutWSName,dEModeRequested,Emode);
            return TargTableWS;
          }
        }
        else // there is a workspace in the data service with the same name but this ws is not suitable as target for this algorithm. 
        {    // Should delete this WS from the dataservice
          API::AnalysisDataService::Instance().remove(tOutWSName);
        }
    }
    // No result found in analysis data service or the result is unsatisfactory. Try to calculate target workspace.  

    TargTableWS =this->runPreprocessDetectorsToMDChildUpdatingMasks(InWS2D,tOutWSName,dEModeRequested,Emode);

    if(storeInDataService)
      API::AnalysisDataService::Instance().addOrReplace(tOutWSName,TargTableWS);
//    else
//      TargTableWS->setName(OutWSName);

  
   // check if we got what we wanted:

   // in direct or indirect mode input ws has to have input energy
    if(Emode==Kernel::DeltaEMode::Direct||Emode==Kernel::DeltaEMode::Indirect)
    {
       double   m_Ei  = TargTableWS->getLogs()->getPropertyValueAsType<double>("Ei");
       if(isNaN(m_Ei))
       {
         // Direct mode needs Ei
         if(Emode==Kernel::DeltaEMode::Direct)throw(std::invalid_argument("Input neutron's energy has to be defined in inelastic mode "));

         // Do we have at least something for Indirect?
         float *eFixed = TargTableWS->getColDataArray<float>("eFixed");
         if(!eFixed)
           throw(std::invalid_argument("Input neutron's energy has to be defined in inelastic mode "));

         uint32_t NDetectors = TargTableWS->getLogs()->getPropertyValueAsType<uint32_t>("ActualDetectorsNum");
         for(uint32_t i=0;i<NDetectors;i++)
           if(isNaN(*(eFixed+i)))throw(std::invalid_argument("Undefined eFixed energy for detector N: "+boost::lexical_cast<std::string>(i)));
       }
    }

    return TargTableWS;
}

DataObjects::TableWorkspace_sptr  ConvertToMDParent::runPreprocessDetectorsToMDChildUpdatingMasks(Mantid::API::MatrixWorkspace_const_sptr InWS2D,
                                                                                            const std::string &OutWSName,const std::string &dEModeRequested,Kernel::DeltaEMode::Type &Emode)
{
   // prospective result
    DataObjects::TableWorkspace_sptr TargTableWS;

    // if input workspace does not exist in analysis data service, we have to add it there to work with the Child Algorithm 
    std::string InWSName = InWS2D->getName();
    if(!API::AnalysisDataService::Instance().doesExist(InWSName))
    {
      throw std::runtime_error("Can not retrieve input matrix workspace "+InWSName+" from the analysis data service");
    }

    Mantid::API::Algorithm_sptr childAlg = createChildAlgorithm("PreprocessDetectorsToMD",0.,1.);
    if(!childAlg)throw(std::runtime_error("Can not create child ChildAlgorithm to preprocess detectors"));
    childAlg->setProperty("InputWorkspace",InWSName);
    childAlg->setProperty("OutputWorkspace",OutWSName);
    childAlg->setProperty("GetMaskState",true);
    childAlg->setProperty("UpdateMasksInfo",true);
    childAlg->setProperty("OutputWorkspace",OutWSName);

 // check and get energy conversion mode to define additional ChildAlgorithm parameters
    Emode = Kernel::DeltaEMode().fromString(dEModeRequested);
    if(Emode == Kernel::DeltaEMode::Indirect) 
      childAlg->setProperty("GetEFixed",true); 


    childAlg->execute();
    if(!childAlg->isExecuted())throw(std::runtime_error("Can not properly execute child algorithm PreprocessDetectorsToMD"));

    TargTableWS = childAlg->getProperty("OutputWorkspace");
    if(!TargTableWS)throw(std::runtime_error("Can not retrieve results of child algorithm PreprocessDetectorsToMD"));

    return TargTableWS;
}


} // namespace Mantid
} // namespace MDAlgorithms


