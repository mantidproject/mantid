/*WIKI* 
== Summary ==

Transforms a workspace into MDEvent workspace with dimensions defined by user. 
   
Gateway for set of subalgorithms, combined together to convert input 2D matrix workspace or event workspace with any units along X-axis into  multidimensional event workspace. 

Depending on the user input and the data, find in the input workspace, the algorithms transform the input workspace into 1 to 4 dimensional MDEvent workspace and adds to this workspace additional dimensions, which are described by the workspace properties and requested by user.

*WIKI*/

#include "MantidMDAlgorithms/ConvertToMDEvents.h"

#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayLengthValidator.h"
//
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceValidators.h"
//
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventFactory.h"
//
#include "MantidDataObjects/Workspace2D.h"

#include <algorithm>
#include <float.h>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDAlgorithms
{

// logger for the algorithm workspaces  
Kernel::Logger& ConvertToMDEvents::convert_log =Kernel::Logger::get("MD-Algorithms");
// the variable describes the locations of the preprocessed detectors, which can be stored and reused if the algorithm runs more then once;
PreprocessedDetectors ConvertToMDEvents::det_loc;
//
Mantid::Kernel::Logger & 
ConvertToMDEvents::getLogger(){return convert_log;}
//
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToMDEvents)


// Sets documentation strings for this algorithm
void ConvertToMDEvents::initDocs()
{
    this->setWikiSummary("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, it will be replaced");
    this->setOptionalMessage("Create a MDEventWorkspace with selected dimensions, e.g. the reciprocal space of momentums (Qx, Qy, Qz) or momentums modules |Q|, energy transfer dE if availible and any other user specified log values which can be treated as dimensions. If the OutputWorkspace exists, it will be replaced");
//TODO:    "If the OutputWorkspace exists, then events are added to it." 
}
//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToMDEvents::~ConvertToMDEvents()
{
}
//
//const double rad2deg = 180.0 / M_PI;
//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void 
ConvertToMDEvents::init()
{
      auto ws_valid = boost::make_shared<CompositeValidator>();
      //
      ws_valid->add<InstrumentValidator>();
      // the validator which checks if the workspace has axis and any units
      ws_valid->add<WorkspaceUnitValidator>("");


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace (Matrix 2D or Event) with units along X-axis and defined instrument with defined sample");
   
     declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
                  "Name of the output MDEventWorkspace");
     declareProperty(new PropertyWithValue<bool>("OverwriteExisting", true, Direction::Input),
              "Unselect this if you want to add new events to the workspace, which already exist. Can be very inefficient for file-based workspaces.");

     Strings Q_modes = ParamParser.getQModes();
     /// this variable describes default possible ID-s for Q-dimensions   
     declareProperty("QDimensions",Q_modes[modQ],boost::make_shared<StringListValidator>(Q_modes),
         "You can to transfer source workspace into target MD workspace directly by supplying string ""CopyToMD""\n"
         " (No Q analysis, or Q conversion is performed),\n"
         "into mod(Q) (1 dimension) providing ""|Q|"" string or into 3 dimensions in Q space ""Q3D"". \n"
         " First mode used for copying data from input workspace into multidimensional target workspace, second -- mainly for powder analysis\n"
         "(though crystal as powder is also analysed in this mode) and the third -- for crystal analysis.\n",Direction::InOut); 
     // this switch allows to make units expressed in HKL, hkl is currently not supported by units conversion so the resulting workspace can not be subject to unit conversion
     declareProperty(new PropertyWithValue<bool>("QinHKL", true, Direction::Input),
         " Setting this property to true will normalize three momentums obtained in Q3D mode by reciprocal lattice vectors 2pi/a,2pi/b and 2pi/c\n"
         " ignored in mod|Q| and ""CopyToMD"" modes and if a reciprocal lattice is not defined in the input workspace");
     /// this variable describes implemented modes for energy transfer analysis
          Strings dE_modes = ParamParser.getDEModes();
     declareProperty("dEAnalysisMode",dE_modes[Direct],boost::make_shared<StringListValidator>(dE_modes),
        "You can analyse neutron energy transfer in direct, indirect or elastic mode. The analysis mode has to correspond to experimental set up.\n"
        " Selecting inelastic mode increases the number of the target workspace dimensions by one. (by DeltaE -- the energy transfer)\n"
        """NoDE"" choice corresponds to ""CopyToMD"" analysis mode and is selected automatically if the QDimensions is set to ""CopyToMD""",Direction::InOut);                
     
    declareProperty(new ArrayProperty<std::string>("OtherDimensions",Direction::Input),
        " List(comma separated) of additional to Q and DeltaE variables which form additional (orthogonal) to Q dimensions"
        " in the target workspace (e.g. Temperature or Magnetic field).\n"
        " These variables had to be logged during experiment and the names of these variables "
        " have to coincide with the log names for the records of these variables in the source workspace");

    // this property is mainly for subalgorithms to set-up as they have to identify if they use the same instrument. 
    declareProperty(new PropertyWithValue<bool>("UsePreprocessedDetectors", true, Direction::Input), 
        "Store the part of the detectors transformation into reciprocal space to save/reuse it later.\n"
        " Useful if one expects to analyse number of different experiments obtained on the same instrument.\n"
        "<span style=""color:#FF0000""> Dangerous if one uses number of workspaces with modified derived instrument one after another. </span>"
        " In this case switch has to be set to false, as first instrument would be used for all workspaces othewise and no check for its validity is performed."); 

    declareProperty(new ArrayProperty<double>("MinValues"),
        "It has to be N comma separated values, where N is defined as: \n"
        "a) 1+N_OtherDimensions if the first dimension (QDimensions property) is equal to |Q| or \n"
        "b) 3+N_OtherDimensions if the first (3) dimensions (QDimensions property) equal  Q3D or \n"
        "c) (1 or 2)+N_OtherDimesnions if QDimesnins property is emtpty. \n"
        " In case c) the target workspace dimensions are defined by the [[units]] of the input workspace axis.\n\n"
         " This property contains minimal values for all dimensions.\n"
         " Momentum values expected to be in [A^-1] and energy transfer (if any) expressed in [meV]\n"
         " In case b), the target dimensions for Q3D are either momentums if QinHKL is false or are momentums divided by correspondent lattice parameters if QinHKL is true\n"
         " All other values are in the [[units]] they are expressed in their log files\n"
         " Values lower then the specified one will be ignored and not transferred into the target MD workspace\n");
//TODO:    " If a minimal target workspace range is higher then the one specified here, the target workspace range will be used instead " );

   declareProperty(new ArrayProperty<double>("MaxValues"),
         " A list of the same size and the same units as MinValues list"
         " Values higher or equal to the specified by this list will be ignored\n");
//TODO:    "If a maximal target workspace range is lower, then one of specified here, the target workspace range will be used instead" );
    
    declareProperty(new ArrayProperty<double>("Uproj"),
     "Optional: First base vector (in hkl) defining fractional or crystal catrezian coordinate system for neutron diffraction;\n"
     "If nothing is specified as input, it will try to recover this vector from the input workspace's oriented lattice,\n"
    " where it should define the initial orientation of the crystal wrt the beam. \n"
    " If no oriented lattice is not found, the workspace is processed with unit coordinate transformation matrix or in powder mode.\n"); 
    declareProperty(new ArrayProperty<double>("Vproj"),
    "Optional:  Second base vector (in hkl) defining fractional rystal catrezian coordinate system for neutron diffraction; \n"
    "If nothing is specified as input, it will try to recover this vector from the input workspace's oriented lattice\n"
    "and if this fails, proceed as for property u above.");

   // Box controller properties. These are the defaults
    this->initBoxControllerProps("5" /*SplitInto*/, 1000 /*SplitThreshold*/, 20 /*MaxRecursionDepth*/);
    // additional box controller settings property. 
    auto mustBeMoreThen1 = boost::make_shared<BoundedValidator<int> >();
    mustBeMoreThen1->setLower(1);

    declareProperty(
      new PropertyWithValue<int>("MinRecursionDepth", 1,mustBeMoreThen1),
      "Optional. If specified, then all the boxes will be split to this minimum recursion depth. 1 = one level of splitting, etc.\n"
      "Be careful using this since it can quickly create a huge number of boxes = (SplitInto ^ (MinRercursionDepth * NumDimensions)).\n"
      "But setting this property equal to MaxRecursionDepth property is necessary if one wants to generate multiple file based workspaces in order to merge them later\n");
    setPropertyGroup("MinRecursionDepth", getBoxSettingsGroupName());

 
}

 //----------------------------------------------------------------------------------------------
/* Execute the algorithm.   */
void ConvertToMDEvents::exec()
{
    // initiate all availible subalgorithms for further usage (it will do it only once, first time the algorithm is executed);
    this->subAlgFactory.init(ParamParser);

  // initiate class which would deal with any dimension workspaces, handling 
  if(!pWSWrapper.get()){
    pWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
  }
  // -------- Input workspace
  this->inWS2D = getProperty("InputWorkspace");
  if(!inWS2D)
  {
    convert_log.error()<<" can not obtain input matrix workspace from analysis data service\n";
  }
  // ------- Is there any output workspace?
  // shared pointer to target workspace
  API::IMDEventWorkspace_sptr spws = getProperty("OutputWorkspace");
  bool create_new_ws(false);
  if(!spws.get())
  {
    create_new_ws = true;
  }else{ 
      bool should_overwrite = getProperty("OverwriteExisting");
      if (should_overwrite){
          create_new_ws=true;
      }else{
          create_new_ws=false;
      }
  }

  if (create_new_ws){
    //identify if u,v are present among input parameters and use defaults if not
    std::vector<double> ut = getProperty("UProj");
    std::vector<double> vt = getProperty("VProj");
    this->checkUVsettings(ut,vt,TWS);
  }
  // Collate and Analyze the requests to the job, specified by the input parameters

 // what dimension names requested by the user by:
    //a) Q selector:
    std::string Q_mod_req                    = getProperty("QDimensions");
    //b) the energy exchange mode
    std::string dE_mod_req                   = getProperty("dEAnalysisMode");
    //c) other dim property;
    std::vector<std::string> other_dim_names = getProperty("OtherDimensions");
    //d) part of the procedure, specifying the target dimensions units. Currently only Q3D target units be converted to hkl
    bool convert_to_hkl                      = getProperty("QinHKL");

    // Identify the algorithm to deploy on workspace. Also fills in the target workspace description
    std::string algo_id = ParamParser.identifyTheAlg(inWS2D,Q_mod_req,dE_mod_req,other_dim_names,convert_to_hkl,pWSWrapper->getMaxNDim(),TWS);

    // set the min and max values for the dimensions from the input porperties
    TWS.dimMin = getProperty("MinValues");
    TWS.dimMax = getProperty("MaxValues");
   // verify that the number min/max values is equivalent to the number of dimensions defined by properties and min is less the
    TWS.checkMinMaxNdimConsistent(convert_log);    

    if(create_new_ws)
    {
        // set up target coordinate system
        TWS.rotMatrix = getTransfMatrix(inWS2D->name(),TWS);
        // identify/set the (multi)dimension's names to use
        // build meaningfull dimension names for Q-transformation if it is Q-transformation indeed 
        // also (temporary) redefines transformation matrix in convert to hkl mode
        this->buildDimensions(TWS);
    }
    else // user input is mainly ignored and everything is in old workspac
    {  
        // existing workspace defines target coordinate system:
        TWS.rotMatrix = getTransfMatrix(spws,inWS2D);
        // dimensions are already build

        MDEvents::MDWSDescription OLDWSD;
        OLDWSD.build_from_MDWS(spws);
        // compare the descriptions which come from existing workspace and select the one, which satisfy existing workspace
        OLDWSD.compareDescriptions(TWS);

        throw(Kernel::Exception::NotImplementedError("Adding to existing MD workspace not Yet Implemented"));
    }

    // Check what to do with detectors:  
    if(TWS.detInfoLost)
    { // in NoQ mode one may not have DetPositions any more. Neither this information is needed for anything except data conversion interface. 
         buildFakeDetectorsPositions(inWS2D,det_loc);
    }
    else  // preprocess or not the detectors positions
    {
          bool reuse_preprocecced_detectors = getProperty("UsePreprocessedDetectors");
          if(!(reuse_preprocecced_detectors&&det_loc.isDefined(inWS2D))){
            // amount of work:
            const size_t nHist = inWS2D->getNumberHistograms();
            pProg = std::auto_ptr<API::Progress >(new API::Progress(this,0.0,1.0,nHist));
            processDetectorsPositions(inWS2D,det_loc,convert_log,pProg.get());
            if(det_loc.det_id.empty()){
                g_log.error()<<" no valid detectors identified associated with spectra, nothing to do\n";
                throw(std::invalid_argument("no valid detectors indentified associated with any spectra"));
            }
          }
    }

 // create and initate new workspace
  if(create_new_ws)  
  {
    spws = pWSWrapper->createEmptyMDWS(TWS);
    if(!spws)
    {
      g_log.error()<<"can not create target event workspace with :"<<TWS.nDims<<" dimensions\n";
      throw(std::invalid_argument("can not create target workspace"));
    }
    // Build up the box controller
    Mantid::API::BoxController_sptr bc = pWSWrapper->pWorkspace()->getBoxController();
    // Build up the box controller, using the properties in BoxControllerSettingsAlgorithm
    this->setBoxController(bc);
    // split boxes;
    spws->splitBox();
  // Do we split more due to MinRecursionDepth?
    int minDepth = this->getProperty("MinRecursionDepth");
    int maxDepth = this->getProperty("MaxRecursionDepth");
    if (minDepth>maxDepth) throw std::invalid_argument("MinRecursionDepth must be >= MaxRecursionDepth ");
    spws->setMinRecursionDepth(size_t(minDepth));  
  }

  //DO THE JOB:

  // get pointer to appropriate  algorithm, (will throw if logic is wrong and subalgorithm is not found among existing)
  IConvertToMDEventsMethods * algo =  subAlgFactory.getAlg(algo_id);
  // initate conversion and estimate amout of job to dl
  size_t n_steps = algo->setUPConversion(inWS2D,det_loc,TWS, pWSWrapper);
  // progress reporter
  pProg = std::auto_ptr<API::Progress >(new API::Progress(this,0.0,1.0,n_steps)); 

  algo->runConversion(pProg.get());
  
  //JOB COMPLETED:
  setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(spws));

  // free the algorithm from the responsibility for the target workspace to allow it to be deleted if necessary
  pWSWrapper->releaseWorkspace();
  // free up the sp to the input workspace, which would be deleted if nobody needs it any more;
  inWS2D.reset();
  return;
}




/** The matrix to convert neutron momentums into the target coordinate system   */
std::vector<double> ConvertToMDEvents::getTransfMatrix(const std::string &inWsName,MDEvents::MDWSDescription &TargWSDescription)const
{
  
    Kernel::Matrix<double> mat(3,3,true);
    Kernel::Matrix<double> umat;
    

    bool has_lattice(true);
    if(!TargWSDescription.pLatt.get())  has_lattice=false;

    // warn about 3D case without lattice
    if(!has_lattice && ParamParser.getQMode(TargWSDescription.AlgID)==Q3D)
    {
        convert_log.warning()<<
        " Can not obtain transformation matrix from the input workspace: "<<inWsName<<
        " as no oriented lattice has been defined. \n"
        " Will use unit transformation matrix\n";
    }
    //
    if(has_lattice){
      if(TargWSDescription.is_uv_default){
         // we need to set up u,v for axis caption as it is defined in workspace UB matrix;
         TargWSDescription.u = TargWSDescription.pLatt->getuVector();
         TargWSDescription.v = TargWSDescription.pLatt->getvVector(); 
         umat  = TargWSDescription.pLatt->getU();
      }else{
         // thansform the lattice above into the Cartezian coordinate system related to projection vectors u,v;
         umat = TargWSDescription.pLatt->setUFromVectors(TargWSDescription.u,TargWSDescription.v);
      }

      // Obtain the transformation matrix to Cartezian related to Crystal
      mat = TargWSDescription.GoniomMatr*umat ;
     // and this is the transformation matrix to notional
     //mat = gon*Latt.getUB();
      mat.Invert();
    }
 
    std::vector<double> rotMat = mat.get_vector();
    return rotMat;
}
/** The matrix to convert neutron momentums into the target coordinate system, if the target coordinate system is already defined by existing 
  *   MD workspace */
std::vector<double> ConvertToMDEvents::getTransfMatrix( API::IMDEventWorkspace_sptr spws,API::MatrixWorkspace_sptr inWS)const
{
    UNUSED_ARG(spws);
    UNUSED_ARG(inWS);  
    throw(Kernel::Exception::NotImplementedError("Not yet implemented"));

}

/** Build meaningful dimension namse for different conversion modes
  * Currently modifies Q3D mode
  * Currently modifies the coordinate transformation matrix, if it is Q3D mode converted in hkl
  */
void ConvertToMDEvents::buildDimensions(MDEvents::MDWSDescription &TargWSDescription)
{
    // non-energy transformation modes currently do not change any units and dimension names
    //if(TargWSDescription.emode<0)return;
     for(size_t i=0;i<TargWSDescription.dimIDs.size();i++){
         if(TargWSDescription.dimIDs[i].empty()){
               TargWSDescription.dimIDs[i]="Dim"+boost::lexical_cast<std::string>(i);              
         }
         if(TargWSDescription.dimNames[i].empty()){
                TargWSDescription.dimNames[i]=TargWSDescription.dimIDs[i]; 
         }

     }
     Q_state Q;
     AnalMode Mode;
     CnvrtUnits cUnits;
     InputWSType WS;
     this->ParamParser.getAlgoModes(TargWSDescription.AlgID,Q,Mode,cUnits,WS);

    // Q3D mode needs special treatment for dimension names:
    if(Q==Q3D){
        std::vector<Kernel::V3D> dim_directions(3);
        Kernel::Matrix<double> Bm(3,3,true);
        if(TargWSDescription.pLatt.get())
        {
            Bm=TargWSDescription.pLatt->getB();
        }
        if(TargWSDescription.is_uv_default)
        {
            dim_directions[0]=Bm*Kernel::V3D(1,0,0);
            dim_directions[0].normalize();
            dim_directions[1]=Bm*Kernel::V3D(0,1,0);
            dim_directions[1].normalize();
            dim_directions[2]=Bm*Kernel::V3D(0,0,1);
            dim_directions[2].normalize();
        }else{
            for(int i=0;i<3;i++){
                for(int j=0;j<3;j++){
                    Bm[i][j]*= TargWSDescription.pLatt->a(i);
                }
            }
            dim_directions[0]=Bm*TargWSDescription.u;
            Kernel::V3D vp   =Bm*TargWSDescription.v;
            dim_directions[2]=dim_directions[0].cross_prod(vp);
            dim_directions[2].normalize();
            dim_directions[1]=dim_directions[2].cross_prod(dim_directions[0]);
            dim_directions[1].normalize();

        }

        for(int i=0;i<3;i++)
        {
            TargWSDescription.dimNames[i]=MDEvents::makeAxisName(dim_directions[i],this->ParamParser.getDefaultQNames(Q3D,Mode));
            if(TargWSDescription.convert_to_hkl){
                // lattice wave vector
                double cr(1);
                if(TargWSDescription.pLatt.get()){
                    cr=TargWSDescription.pLatt->a(i)/(2*M_PI);
                }
                for(int j=0;j<3;j++){
                    TargWSDescription.rotMatrix[3*i+j]*=cr;
                }
                TargWSDescription.dimUnits[i] = "in "+MDEvents::sprintfd(1/cr,1.e-3)+" A^-1";
            }

        }
    } 

}

//
void 
ConvertToMDEvents::checkUVsettings(const std::vector<double> &ut,const std::vector<double> &vt,MDEvents::MDWSDescription &TargWSDescription)const
{
    Kernel::V3D u,v;
//identify if u,v are present among input parameters and use defaults if not
    bool u_default(true),v_default(true);
    if(!ut.empty()){
        if(ut.size()==3){ u_default =false;
        }else{convert_log.warning() <<" u projection vector specified but its dimensions are not equal to 3, using default values [1,0,0]\n";
        }
    }
    if(!vt.empty()){
        if(vt.size()==3){ v_default  =false;
        }else{ convert_log.warning() <<" v projection vector specified but its dimensions are not equal to 3, using default values [0,1,0]\n";
        }
    }
    if(u_default){  
        u[0] = 1;         u[1] = 0;        u[2] = 0;
    }else{
        u[0] = ut[0];     u[1] = ut[1];    u[2] = ut[2];
    }
    if(v_default){
        v[0] = 0;         v[1] = 1;        v[2] = 0;
    }else{
        v[0] = vt[0];     v[1] = vt[1];    v[2] = vt[2];
    }
    if(u_default&&v_default){
        TargWSDescription.is_uv_default=true;
    }else{
        TargWSDescription.is_uv_default=false;
    }
    TargWSDescription.u=u;
    TargWSDescription.v=v;
}

/** Constructor 
 *  picks up an instanciates all known sub-algorithms for ConvertToMDEvents
*/
ConvertToMDEvents::ConvertToMDEvents():
// initiate target ws description to be not empty and have 4 dimensions (It will be redefined later, but defailt_qNames are defined only 
// when N-dim constructor wass called
TWS(4)
{
 
}


} // namespace Mantid
} // namespace MDAlgorithms


