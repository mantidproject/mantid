#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"

#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/NumericAxis.h"

namespace Mantid
{
namespace MDAlgorithms
{

Kernel::Logger& ConvertToMDEventsParams::convert_log =Kernel::Logger::get("MD-Algorithms");


/** function checks if the candidate belongs to the group and returns its number in the group or -1 if the candidate is not a group member */
int is_member(const std::vector<std::string> &group,const std::string &candidate)
{
    int num(-1);
    for(size_t i=0;i<group.size();i++){
        if(candidate.compare(group[i])==0){
            num = int(i);
            return num;
        }
    }
    return num;
}

/** Identify the Momentum conversion mode requested by user
  * 
  *@param Q_mode_req    -- What conversion algorithm user wants to deploy (Q3d, modQ, no Q)
  *@param ws_dim_names  -- vector of input workspace dimensions names 
  *@param ws_dim_units  -- vector of input workspace dimensions units ID-s
  *@param out_dim_names [out] -- vector of dimension names for momentuns in target workspace
  *@param out_dim_units [out] -- vector of units for target workspace
  *@param nQ_dims [out]       -- number of Q or other dimensions. When converting into Q, it is 1 or 3 dimensions, if NoQ -- workspace dimensions are copied.
*/
std::string 
ConvertToMDEventsParams::parseQMode(const std::string &Q_mode_req,const Strings &ws_dim_names,const Strings &ws_dim_units,Strings &out_dim_ID,Strings &out_dim_units, 
                                    int &nQ_dims)const
{
    std::string Q_MODE_ID("Unknown");
    if(is_member(Q_modes,Q_mode_req)<0){
         convert_log.error()<<" Q-mode: "<<Q_mode_req<<" not recognized\n";
         throw(std::invalid_argument(" Non-existing Q-mode"));
    }
    // Q_mode (one of 3 possible)  
    if(Q_mode_req.compare(Q_modes[NoQ])==0)
    { 
        nQ_dims      = int(ws_dim_names.size());
        Q_MODE_ID    = Q_modes[NoQ];
        out_dim_ID   = ws_dim_names;
        out_dim_units= ws_dim_units;
    }
    if(Q_mode_req.compare(Q_modes[modQ])==0) // At the moment we assume that |Q| make sense for inelastic only,      
    {  // so the only one variable is availible form the workspace. 
        nQ_dims=1;      
        out_dim_ID.resize(1);
        out_dim_units.resize(1);
        out_dim_ID[0] = default_dim_ID[modQ_ID];
        out_dim_units[0] = native_elastic_unitID;
        Q_MODE_ID = Q_modes[modQ];

    }
    if((Q_mode_req.compare(Q_modes[Q3D])==0))
    {
       nQ_dims=3;
       out_dim_ID.resize(3);       
       out_dim_ID[0]= default_dim_ID[Q1_ID];
       out_dim_ID[1]= default_dim_ID[Q2_ID];
       out_dim_ID[2]= default_dim_ID[Q3_ID];
       Q_MODE_ID = Q_modes[Q3D];
       out_dim_units.assign(3,native_elastic_unitID);

    }
    return Q_MODE_ID;
}
/** Identify the Energy conversion mode requested by user 
  * 
  *@param Q_MODE_ID     -- the momentum conversion mode. Energy conversion depends on it
  *@param dE_mode_req   -- What conversion algorithm user wants to deploy (direct/indirect elastic)
  *@param ws_dim_units  -- vector of input workspace dimensions units ID-s
  *@param out_dim_names [out] -- vector of names for target workspace, if inelastic, one of the dimension units have to be DeltaE
  *@param out_dim_units [out] -- vector of units for target workspace, if inelastic, one of the dimension units have to be DeltaE
  *@param ndE_dims [out]      -- number of additional dimensions, if inelastic, it would be one dimension more.
  *@param natural_units [out] -- name of the units, the algorithm expects to work with.
*/
std::string 
ConvertToMDEventsParams::parseDEMode(const std::string &Q_MODE_ID,const std::string &dE_mode_req,const Strings &ws_dim_units,                                
                                Strings &out_dim_IDs,Strings &out_dim_units,int &ndE_dims,std::string &natural_units)const
{
    if(is_member(dE_modes,dE_mode_req)<0){
         convert_log.error()<<" dE-mode: "<<dE_mode_req<<" not recognized\n";
         throw(std::invalid_argument(" Non-existing dE-mode"));
    }
    // set default number of additional dimensions, neded for anargy analysis mode to 0 (no needed)
    ndE_dims = 0;

    std::string DE_MODE_ID= dE_mode_req;
    // no_Q mode can only be compartible with no_dE mode
    if((Q_MODE_ID.compare(Q_modes[NoQ])==0)){
        DE_MODE_ID = dE_modes[ANY_Mode];
      // no-Q mode -- no conversion, so natural units are the one, already used by the workspace
        natural_units=ws_dim_units[0];   
        if(DE_MODE_ID.compare(dE_mode_req)!=0){
            convert_log.warning()<<" No Q mode selected together with dEAnalysisMode: "<<dE_mode_req<<std::endl;
            convert_log.warning()<<" No Q mode not comparible with any energy analyzsis mode, so the energy analysis mode ignored\n";
        }
    }
    // inelastic modes have one additional dimension and need special units on X-axis
    if((DE_MODE_ID.compare(dE_modes[Direct])==0)||(DE_MODE_ID.compare(dE_modes[Indir])==0)){
        ndE_dims = 1;
        out_dim_IDs.push_back(default_dim_ID[dE_ID]);
        out_dim_units.push_back("DeltaE");
        // natural units defined in subalgorithm doing the conversion and their ID has to be defined correctly in class constructor
        natural_units = native_inelastic_unitID;
    }

    if(DE_MODE_ID.compare(dE_modes[Elastic])==0){
        natural_units = native_elastic_unitID;
    }    
    return DE_MODE_ID;
}

/** Identify the Unit conversion mode, deployed by the subalgorith 
  * 
  *@param Q_MODE_ID       -- the momentum conversion mode. Unit conversion depends on it
  *@param UnitsToConvert2 -- the units one needs to convert ws to before deplpying Q-dE transformation. 
  *@param ws_dim_units    -- vector of input workspace dimensions units ID-s
  *
  *@returns CONV_MODE_ID -- the string identifier, which says what energy mode is deployed. 
*/
std::string
ConvertToMDEventsParams::parseConvMode(const std::string &Q_MODE_ID,const std::string &UnitsToConvert2,const std::vector<std::string> &ws_dim_units)const
{
    std::string CONV_MODE_ID("Unknown");
    // IDENTIFY UNITS CONVERSION MODE
    // get all known units:
    std::vector<std::string> AllKnownUnits= Kernel::UnitFactory::Instance().getKeys();
    // check if unit conversion is possible at all:
    if(Q_MODE_ID.compare(Q_modes[NoQ])!=0){
        if(is_member(AllKnownUnits,ws_dim_units[0])<0){
            convert_log.error() <<" Unknown unit"<<ws_dim_units[0]<<" along X-axis provided for conversion\n";
            throw(std::invalid_argument("ConvertToMDEvents needs to known units conversion"));
        }  
    }else{ // NoQ mode -- no conversion
        CONV_MODE_ID =ConvModes[ConvertNo];  
        return CONV_MODE_ID;
    }
    // are the existing units already what is needed, so no conversion?    
    if(ws_dim_units[0].compare(UnitsToConvert2)==0){
        CONV_MODE_ID = ConvModes[ConvertNo];
    }else{
        // is a quick conversion availible?
        double factor,power;
        const Kernel::Unit_sptr pThisUnit=Kernel::UnitFactory::Instance().create(ws_dim_units[0]);
        if(pThisUnit->quickConversion(UnitsToConvert2,factor,power)){
            CONV_MODE_ID = ConvModes[ConvFast];
        }else{
            if(ws_dim_units[0].compare("TOF")==0){ // may be it is TOF?
                CONV_MODE_ID = ConvModes[ConvFromTOF];
            }else{                                 // convert via TOF
                CONV_MODE_ID = ConvModes[ConvByTOF];
            }
        }
    } 

    return CONV_MODE_ID;
}



/** identify what kind of input workspace is provided as input argument
 *
 *@param inMatrixWS  a pointer to the workspace, obtained from analysis data service
 *
 *@returns   -- the ID of the workspace of one of the supported types. Throws if can not dynamiucally cast the pointer to the workspace:
*/
std::string 
ConvertToMDEventsParams::parseWSType(API::MatrixWorkspace_const_sptr inMatrixWS)const
{
    const DataObjects::EventWorkspace *pWSEv= dynamic_cast<const DataObjects::EventWorkspace *>(inMatrixWS.get());
    if(pWSEv){
        return SupportedWS[EventWSType];
    }

    const DataObjects::Workspace2D *pMWS2D = dynamic_cast<const DataObjects::Workspace2D *>(inMatrixWS.get());
    if(pMWS2D){
        return SupportedWS[Workspace2DType];
    }
 
    convert_log.error()<<" Unsupported workspace type provided. Currently supported types are:\n";
    for(int i=0;i<NInWSTypes;i++){
        convert_log.error()<<" WS ID: "<<SupportedWS[i];
    }
    convert_log.error()<<std::endl;
    throw(std::invalid_argument("Unsupported worspace type provided"));
    return "";
}


/**  
  *  The dimensions, which can be obtained from workspace are determined by the availible algorithms.
  *  E.g. an inelastic algorithm can transform matrix workspace into 2D-4D workpsace depending on what requested.
  *  If additional algorithms can be generated through algorithm template, this function shluld be modified accordingly
  *  This function identifies the algoritnm, which should be deploued over particular matrix workspace;
  *
  * @param inMatrixWS -- const pointer to const matrix workspace, which provides information about availible axis
  * @param Q_mode_req     -- what to do with Q-dimensions e.g. calculate either mod|Q| or Q3D;
  * @param dE_mode_req    -- desirable dE analysis mode (elastic, direct/indirect)
  *
  * @return out_dim_IDs        -- the vector of strings, with each string identify the dimensions ID-s, derived from current workspace by the algorithm
  * @return out_dim_units      -- vector of units for target workspace, if inelastic, one of the dimension units have to be DeltaE
  * @return is_det_info_lost   -- if Y axis of matix workspace contains some meaningful info, the detector information is certainly lost. 
  * @return algo_id            -- the string, which describes the algorithm, which should be deployed on ws.

*/
std::string 
ConvertToMDEventsParams::identifyMatrixAlg(API::MatrixWorkspace_const_sptr inMatrixWS, const std::string &Q_mode_req, const std::string &dE_mode_req,
                                     Strings &out_dim_IDs,Strings &out_dim_units, bool &is_detector_information_lost)
{
    // the key which would describe the algorithm to deploy on the matrix
    std::string algo_id;

    // dimension names present in input workspace
    std::vector<std::string> ws_dim_names;
    // unit IS-s the input workspace dimensions have
    std::vector<std::string> ws_dim_units;


   // get the X axis of input workspace, it has to be there; if not axis throws invalid index
    API::NumericAxis *pXAxis = dynamic_cast<API::NumericAxis *>(inMatrixWS->getAxis(0));
    if(!pXAxis ){
        convert_log.error()<<"Can not retrieve X axis from the source workspace: "<<inMatrixWS->getName()<<std::endl;
        throw(std::invalid_argument("Input workspace has to have X-axis"));
    }else{
        std::string Dim1Unit   = pXAxis->unit()->unitID();
        ws_dim_names.push_back(pXAxis->title());
        ws_dim_units.push_back(Dim1Unit);
    }
    // get optional Y axis which can be used in NoQ-kind of algorithms
    is_detector_information_lost= false;
    API::NumericAxis *pYAxis = dynamic_cast<API::NumericAxis *>(inMatrixWS->getAxis(1));
    if(pYAxis){
        std::string Dim2Unit = pYAxis->unit()->unitID();
        ws_dim_names.push_back(pYAxis->title());
        ws_dim_units.push_back(Dim2Unit);
        // if this is numeric axis, then the detector's information has been lost:
        is_detector_information_lost=true;
    }          


    std::string Q_MODE_ID,DE_MODE_ID,CONV_MODE_ID,WS_ID;
    int nQ_dims(0),ndE_dims(0);
    // identify, what kind of input workspace is there:
    WS_ID = parseWSType(inMatrixWS);
    algo_id =WS_ID;
    // identify Q_mode
    Q_MODE_ID = parseQMode (Q_mode_req,ws_dim_names,ws_dim_units,out_dim_IDs,out_dim_units,nQ_dims);  
    algo_id += Q_MODE_ID;
    // identify dE mode    
    DE_MODE_ID= parseDEMode(Q_MODE_ID,dE_mode_req,ws_dim_units,out_dim_IDs,out_dim_units,ndE_dims,natural_units);
    // identify conversion mode;
    algo_id+=DE_MODE_ID; 
    CONV_MODE_ID=parseConvMode(Q_MODE_ID,natural_units,ws_dim_units);
    algo_id+=CONV_MODE_ID;

    //the_WSalgID = Q_MODE_ID+DE_MODE_ID+CONV_MODE_ID;

    return algo_id;

}

/** function returns the algorithm ID as function of different integer conversion modes. 
  * This ID should coinside with the ID, obtained by identifyTheAlg method.
*/
std::string
ConvertToMDEventsParams::getAlgoID(Q_state Q,AnalMode Mode,CnvrtUnits Conv,InputWSType WS)const
{
    return SupportedWS[WS]+Q_modes[Q]+dE_modes[Mode]+ConvModes[Conv];
    
}


/** function processes the input arguments and tries to establish what subalgorithm should be deployed; 
    *
    * @param inWS           -- input workspace (2D or Events)
    * @param Q_mode_req     -- what to do with Q-dimensions e.g. calculate either mod|Q| or Q3D;
    * @param dE_mode_req    -- desirable dE analysis mode (elastic, direct/indirect)
    * @param other_dim_names  -- vector of other dimension names requested by the algorithm
    * @param convert_to_hkl   -- part of the procedure to convert to any uints. Currently  in Q3D mode Q dimensions can be converted to hkl
    * @param dim_names_requested [out] -- dimension names for the target workspace
    * @param dim_units_requested [out] -- dimension units for the target workspace
*/
std::string 
ConvertToMDEventsParams::identifyTheAlg(API::MatrixWorkspace_const_sptr inWS,const std::string &Q_mode_req, 
                                 const std::string &dE_mode_req,const std::vector<std::string> &other_dim_names,
                                 bool convert_to_hkl,size_t maxNdim,
                                 MDEvents::MDWSDescription &TargWSDescription)
{

   Strings dim_IDs_requested,dim_units_requested;
   Strings ws_dim_IDs,ws_dim_units;
   std::string the_algID;
   bool is_detector_info_lost;

   // identify the matrix conversion part of subalgorithm as function of user input and workspace Matrix parameters (axis)
   the_algID = identifyMatrixAlg(inWS, Q_mode_req, dE_mode_req,ws_dim_IDs,ws_dim_units,is_detector_info_lost);
   if(the_algID.find("Unknown")!=std::string::npos){
       convert_log.error()<<" Input parameters indentify uncomplete algorithm ID: "<<the_algID<<std::endl;
       throw(std::logic_error("can not parse input parameters propertly"));
   }

   // retrieve additional dimension names and dimension units, which can be derived from the workspace properties;
   std::vector<std::string> all_add_dim_names,all_add_dim_units;
   this->getAddDimensionNames(inWS,all_add_dim_names,all_add_dim_units);

   // check if additional dimension names can satisfy the user requests:
   std::vector<std::string> add_dim_names,add_dim_units;
   for(size_t i=0;i<other_dim_names.size();i++){
       int n_dim=is_member(all_add_dim_names,other_dim_names[i]);
       if(n_dim<0){
           convert_log.error()<<" Dimension :"<<other_dim_names[i]<<" requested but can not be derived from the input workspace\n";
           throw(std::invalid_argument(" Undefined dimension"));
       }
       add_dim_names.push_back(all_add_dim_names[n_dim]);
       add_dim_units.push_back(all_add_dim_units[n_dim]);
   }
   // assign output:

   dim_IDs_requested   = ws_dim_IDs;
   dim_units_requested = ws_dim_units;
   // add additional dimensions (from properties)
   dim_IDs_requested.insert(dim_IDs_requested.end(),add_dim_names.begin(),add_dim_names.end());
   dim_units_requested.insert(dim_units_requested.end(),add_dim_units.begin(),add_dim_units.end());

   size_t nDims      = dim_IDs_requested.size();

   // Sanity checks:
    if(nDims<3&&(the_algID.find(Q_modes[Q3D])!=std::string::npos)){
        convert_log.error()<<"Algorithm with ID:"<<the_algID<<" should produce at least 3 dimensions and it requested to provie just:"<<nDims<<" dims \n";
        throw(std::logic_error("can not parse input parameters propertly"));
    }
    // we have can currenlty instanciate only N input dimensions. See the wsWrapper constructor to change that. 
    if(nDims>maxNdim){
        convert_log.error()<<"Can not currently deal with more then: "<<maxNdim<< " dimesnions, but requested: "<<nDims<<std::endl;
        throw(std::invalid_argument(" Too many dimensions requested "));
    }
    // get emode
    int emode;
    // if not found NoQ mode, then dE should be availible
    if (Q_mode_req.find(Q_modes[NoQ])==std::string::npos){
        emode = getEMode(the_algID);
    }else{
        emode = -1;  // no coordinate conversion
    }

    // any inelastic mode  needs Ei to be among the input workspace properties 
    if((emode == 1)||(emode == 2))
    {        
        if(!inWS->run().hasProperty("Ei")){
            convert_log.error()<<" Conversion sub-algorithm with ID: "<<the_algID<<" (inelastic) needs input energy to be present among run properties\n";
            throw(std::invalid_argument(" Needs Input energy to be present for inelastic modes"));
        }
        TargWSDescription.Ei = getEi(inWS);
    }
    // detector information can be currently lost in NoQ mode only, when no conversion from detectors position to Q occurs
    if(is_detector_info_lost && emode!=-1){
        convert_log.error()<<" Algorithm with ID: "<<the_algID<<" emode: "<<emode<<" request workspace with isntrument and full detectord information attached\n"
                           <<" but the detector information on input workspace has been lost\n";
        throw(std::invalid_argument(" input workspace do not have full detector information attached to it"));
    }
   
 
    // set up the target workspace description;
    TargWSDescription.nDims          = nDims;
    TargWSDescription.emode          = emode;
    TargWSDescription.detInfoLost    = is_detector_info_lost;
    TargWSDescription.convert_to_hkl = convert_to_hkl;
    TargWSDescription.dimNames       = dim_IDs_requested;
    TargWSDescription.dimIDs         = dim_IDs_requested;
    TargWSDescription.dimUnits       = dim_units_requested;
    TargWSDescription.AlgID          = the_algID;

 
    return the_algID;

}
/** function returns the list of the property names, which can be treated as additional dimensions present in current matrix workspace 
 * TODO: Currenly logically wrong (at least for inelastic)  Specific processed properties have to be introudced
 * 
 * @param inMatrixWS -- shared pointer to input workspace for analysis
 * @param add_dim_names [out] -- the ID-s for the dimension names, which can be obtained from the workspace
 * @param add_dim_units [out] -- the Units ID-s (if any) existing dimensions
*/
void
ConvertToMDEventsParams::getAddDimensionNames(API::MatrixWorkspace_const_sptr inMatrixWS,std::vector<std::string> &add_dim_names,std::vector<std::string> &add_dim_units)const
{   
    // get dimension names from properties
    // TODO: this should be only special processed properties, not all of them, as it is at the moment
    const std::vector< Kernel::Property * > run_properties =  inMatrixWS->run().getProperties();  

    add_dim_names.resize(run_properties.size());    
    add_dim_units.resize(run_properties.size());

    // extract names for all properties, which can be treated as dimension names;
    for(size_t i=0;i<run_properties.size();i++){
        add_dim_names[i]=run_properties[i]->name();
        std::string UnitID = run_properties[i]->units();
       // if(UnitID.empty()||(UnitID.compare("Empty")==0)){ // it is questionable if we want to have unit ID equal to the dimension name and not empty
        //    UnitID =add_dim_names[i];
        //}
        add_dim_units[i]=UnitID;
    }

}



/** Helper Static function to obtain current analysis mode 
  *
  *@param pHost the pointer to the algorithm to work with
  *
  *@returns the mode 0-elastic, 1--direct, 2 indirect. Throws if the mode is not defined or should not be defined 
  (NoQ mode -- no analysis expected)
*/
int  
ConvertToMDEventsParams::getEMode(const std::string &AlgID)const{
    if(AlgID.empty()){
        convert_log.error()<<"getEMode: emode undefined\n";
        throw(std::logic_error(" should not call this function when emode is undefined"));
    }
    if(AlgID.find(dE_modes[Elastic])!=std::string::npos){
        // elastic emode
        return (int)Elastic;
    }
    if(AlgID.find(dE_modes[Direct])!=std::string::npos){
        // direct emode
        return (int)Direct;
    }
    if(AlgID.find(dE_modes[Indir])!=std::string::npos){
        // indirect emode
        return (int)Indir;
    }
    convert_log.error()<<"getEMode: emode for algorithm with ID: "<<AlgID<<" not defined \n";
    throw(std::logic_error(" can not identify correct emode"));
    return -1;
}

/** Helper function to obtain the energy of incident neutrons from the input workspaec
  *
  *@param pHost the pointer to the algorithm to work with
  *
  *@returns the incident energy of the neutrons. 
  *         Throws if the energy property is not defined or can not be retrieved from the workspace
*/
double
ConvertToMDEventsParams::getEi(API::MatrixWorkspace_const_sptr inWS2D)const
{
    if(!inWS2D.get()){
        convert_log.error()<<"getEi: invoked on empty input workspace \n";
        throw(std::logic_error(" should not call this function when input workpace is undefined"));
    }
    Kernel::PropertyWithValue<double>  *pProp(NULL);
    try{
       pProp  =dynamic_cast<Kernel::PropertyWithValue<double>  *>(inWS2D->run().getProperty("Ei"));
    }catch(...){
    }
    if(!pProp){
        //convert_log.error()<<"getEi: can not obtain incident energy of neutrons\n";
        //throw(std::logic_error(" should not call this function when incident energy is undefined"));
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*pProp); 
}



/// Constructor:
ConvertToMDEventsParams::ConvertToMDEventsParams():
Q_modes(NQStates),
dE_modes(4),
ConvModes(NConvUintsStates),
SupportedWS(NInWSTypes),
 /// the ID of the unit, which is used in the expression to converty to QND. All other related elastic units should be converted to this one. 
native_elastic_unitID("Momentum"),// currently it is Q
/// the ID of the unit, which is used in the expression to converty to QND. All other related inelastic units should be converted to this one. 
native_inelastic_unitID("DeltaE"), // currently it is energy transfer (DeltaE)
default_dim_ID(nDefaultID)
{
     // strings to indentify possible momentum analysis modes
     Q_modes[modQ] = "|Q|";
     Q_modes[Q3D]  = "QhQkQl";    
     Q_modes[NoQ]  = "CopyToMD";    // no Q dimension; 
     // strings to indentify possible energy conversion modes
     dE_modes[ANY_Mode]  = "NoDE"; // no Q uses it to run without conversion. 
     dE_modes[Direct]    = "Direct";
     dE_modes[Indir]     = "Indirect";
     dE_modes[Elastic]   = "Elastic";
     // possible unit conversion modes
     ConvModes[ConvertNo]  = "CnvNo";
     ConvModes[ConvFast]   = "CnvFast";
     ConvModes[ConvByTOF]  = "CnvByTOF";
     ConvModes[ConvFromTOF]= "CnvFromTOF";
     // possible input workspace ID-s
     SupportedWS[Workspace2DType] = "WS2D";
     SupportedWS[EventWSType]     = "WSEvent";

     // this defines default dimension ID-s which are used to indentify dimensions when using the target MD workspace later
     // for modQ transformation:
     default_dim_ID[modQ_ID]="|Q|";
     // for Q3D transformation
     default_dim_ID[Q1_ID]="Q1";
     default_dim_ID[Q2_ID]="Q2";
     default_dim_ID[Q3_ID]="Q3";
     default_dim_ID[dE_ID]="DeltaE";

}

}
}
