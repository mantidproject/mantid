#include "MantidMDEvents/MDWSDescription.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidMDEvents/MDTransfFactory.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Strings.h"

#include <boost/lexical_cast.hpp>


namespace Mantid
{
namespace MDEvents
{

/// Template to check if a variable equal to NaN
template <class T>
inline bool isNaN(T val){
    volatile T buf=val;
    return (val!=buf);
}
/** set specific (non-default) dimension name 
 * @param nDim   -- number of dimension;
 * @param Name   -- the name to assighn into diemnsion names vector;
 */
void MDWSDescription::setDimName(unsigned int nDim,const std::string &Name)
{
    if(nDim>=nDims){
        std::string ERR = "setDimName::Dimension index: "+boost::lexical_cast<std::string>(nDim)+" out of total dimensions range: "+boost::lexical_cast<std::string>(nDims);
        throw(std::invalid_argument(ERR));
    }
    dimNames[nDim] = Name;
}
/** this is rather misleading function, as MD workspace does not currently have dimension units. 
  *It actually sets the units dimension names, which will be displayed along axis and have nothinbg in common with units, defined by unit factory */
void MDWSDescription::setDimUnit(unsigned int nDim,const std::string &Unit)
{
    if(nDim>=nDims){
        std::string ERR = "setDimUnit::Dimension index: "+boost::lexical_cast<std::string>(nDim)+" out of total dimensions range: "+boost::lexical_cast<std::string>(nDims);
        throw(std::invalid_argument(ERR));
    }
    dimUnits[nDim] = Unit;
}

/** method sets up the pointer to the class which contains detectors parameters
  * @param   -- det_loc the class which contaits the preprocessed detectors parameters
  *
  * Throws if the parameters were not defined
*/
void MDWSDescription::setDetectors(const ConvToMDPreprocDet &det_loc)
{
    pDetLocations = &det_loc;
    if(pDetLocations->nDetectors()==0) {
        throw(std::invalid_argument( " Preprocessed detectors positions are either empty or undefined. Nothing to do"));
    }

}
/** the method builds the MD ws description from existing matrix workspace and the requested transformation parameters. 
 *@param  pWS    -- input matrix workspace to be converted into MD workpsace
 *@param  QMode  -- momentum conversion mode. Any mode supported by Q conversion factory. Class just carries up the name of Q-mode, 
 *                  to the place where factory call to the solver is made , so no code modification is needed when new modes are added 
 *                  to the factory
 *@param  dEMode  -- energy analysis mode (string representation). Should correspond to energy analysis modes, supported by selected Q-mode
 *@param  dimPropertyNames -- the names of additional properties, which will be used as dimensions

*/
void MDWSDescription::buildFromMatrixWS(const API::MatrixWorkspace_const_sptr &pWS,const std::string &QMode,const std::string dEMode,
                                        const std::vector<std::string> &dimProperyNames)
{
    inWS = pWS;
    // fill additional dimensions values, defined by workspace properties;
    this->fillAddProperties(inWS,dimProperyNames,AddCoord);

    this->AlgID = QMode;

    // check and get energy conversion mode;
    MDTransfDEHelper dEChecker;
    emode = dEChecker.getEmode(dEMode);

    // get raw pointer to Q-transformation (do not delete this pointer!)
    MDTransfInterface* pQtransf =  MDTransfFactory::Instance().create(QMode).get();

    // get number of dimensions this Q transformation generates from the workspace. 
    unsigned int nMatrixDim = pQtransf->getNMatrixDimensions(emode,inWS);

    // number of MD ws dimensions is the sum of n-matrix dimensions and dimensions coming from additional coordinates
    nDims  = nMatrixDim + (unsigned int)AddCoord.size();
    this->resizeDimDescriptions(nDims);
    // check if all MD dimensions descriptors are set properly
    if(nDims!=dimNames.size()||nDims!=dimMin.size())
        throw(std::invalid_argument(" dimension limits vectors and dimension description vectors inconsistent as have different length"));


    //*********** fill in dimension id-s, dimension units and dimension names
    // get default dim ID-s. TODO: it should be possibility to owerride it later;
    std::vector<std::string> MatrDimID   = pQtransf->getDefaultDimID(emode,inWS);
    std::vector<std::string> MatrUnitID  = pQtransf->outputUnitID(emode,inWS);
    for(unsigned int i=0;i<nDims;i++)
    {
        if(i<nMatrixDim){
            dimIDs[i]  = MatrDimID[i];
            dimNames[i]= dimNames[i];
            dimUnits[i]= MatrUnitID[i];
        }else{
            dimIDs[i]  = dimProperyNames[i-nMatrixDim];
            dimNames[i]= dimProperyNames[i-nMatrixDim];
            dimUnits[i]= dimProperyNames[i-nMatrixDim];
        }
    }


    // in direct or indirect mode input ws has to have input energy
    if(emode==ConvertToMD::Direct||emode==ConvertToMD::Indir){
        if(isNaN(getEi(inWS)))throw(std::invalid_argument("Input neutron's energy has to be defined in inelastic mode "));
    }
    

    //Set up goniometer. Empty ws's goniometer returns unit transformation matrix
    this->GoniomMatr = inWS->run().getGoniometer().getR();
    
} 

/** the function builds MD event WS description from existing workspace. 
  * Primary used to obtain existing ws parameters 
*/
void MDWSDescription::buildFromMDWS(const API::IMDEventWorkspace_const_sptr &pWS)
{
    this->nDims = (unsigned int)pWS->getNumDims();
    // prepare all arrays:
    this->dimMin.resize(nDims);
    this->dimMax.resize(nDims);
    this->dimNames.resize(nDims);
    this->dimIDs.resize(nDims);
    this->dimUnits.resize(nDims);   
    this->nBins.resize(nDims);
    for(size_t i=0;i<nDims;i++)
    {
        const Geometry::IMDDimension *pDim = pWS->getDimension(i).get();
        dimMin[i]  = pDim->getMinimum();
        dimMax[i]  = pDim->getMaximum();
        dimNames[i]= pDim->getName();
        dimIDs[i]  = pDim->getDimensionId();
        dimUnits[i]= pDim->getUnits();   
        nBins[i]   = pDim->getNBins();
    }
    this->Wtransf = Kernel::DblMatrix(pWS->getWTransf()); 
    // unnecessary?
    //uint16_t num_experiments = pWS->getNumExperimentInfo();   
    //// target ws does not have experiment info, so this can be only powder. W-transf currently can be only unit matix
    //if (num_experiments==0)
    //{
    //    return;
    //}else{

    //}

}
/** When the workspace has been build from existing MDWrokspace, some target worskpace parameters can not be defined. 
    examples are emode or input energy, which is actually source workspace parameters, or some other parameters 
    defined by the transformation algorithm
*/
void MDWSDescription::setUpMissingParameters(const MDEvents::MDWSDescription &SourceMatrWS)
{
    this->inWS  = SourceMatrWS.inWS;
    this->emode = SourceMatrWS.emode;
    this->AlgID = SourceMatrWS.AlgID;

    this->AddCoord.assign(SourceMatrWS.AddCoord.begin(),SourceMatrWS.AddCoord.end());

    this->GoniomMatr = SourceMatrWS.GoniomMatr;

}


/**function compares old workspace description with the new workspace description, defined by the algorithm properties and 
 * selects/changes the properties which can be changed through input parameters given that target MD workspace exist   
 *
 * This situation occurs if the base description has been obtained from MD workspace, and one is building a description from 
 * other matrix workspace to add new data to the existing workspace. The workspaces have to copmarible
 *
 * @param NewMDWorkspaceD -- MD workspace description, obtained from algorithm parameters
 *
 * @returns NewMDWorkspaceD -- modified md workspace description, which is compartible with existing MD workspace
 *
*/
void  MDWSDescription::checkWSCorresponsMDWorkspace(MDEvents::MDWSDescription &NewMDWorkspaceD)
{
    if(this->nDims != NewMDWorkspaceD.nDims)
    {
        std::string ERR = "Dimension numebrs are inconsistent: this workspace has "+boost::lexical_cast<std::string>(this->nDims)+" dimensions and target one: "+
                           boost::lexical_cast<std::string>(NewMDWorkspaceD.nDims);
         throw(std::invalid_argument(ERR)); 
    }
    if(this->emode==ConvertToMD::Undef)
    {
        throw(std::invalid_argument("Workspace description has not been correctly defined, as emode has not been defined")); 
    }
    //TODO: More thorough checks may be nesessary to prevent adding different kind of workspaces e.g 4D |Q|-dE-T-P workspace to Q3d+dE ws
}




/// empty constructor
MDWSDescription::MDWSDescription(unsigned int nDimensions):
emode(ConvertToMD::Undef),
rotMatrix(9,0),       // set transformation matrix to 0 to certainly see rubbish if error later
Wtransf(3,3,true),
GoniomMatr(3,3,true)
{

    this->resizeDimDescriptions(nDimensions);
    this->dimMin.assign(nDims,std::numeric_limits<double>::quiet_NaN());
    this->dimMax.assign(nDims,std::numeric_limits<double>::quiet_NaN());

}
void MDWSDescription::resizeDimDescriptions(unsigned int nDimensions, size_t nBins)
{
    this->nDims = nDimensions;

    this->dimNames.assign(nDims,"mdn");
    this->dimIDs.assign(nDims,"mdn_");
    this->dimUnits.assign(nDims,"Momentum");
    this->nBins.assign(nDims,nBins);

    for(size_t i=0;i<nDims;i++)
    {
        dimIDs[i]  = dimIDs[i]+boost::lexical_cast<std::string>(i);
        dimNames[i]= dimNames[i]+boost::lexical_cast<std::string>(i);
    }

}
/**function sets up min-max values to the dimensions, described by the class
 *@param minVal  -- vector of minimal dimension's values
 *@param maxVal  -- vector of maximal dimension's values
 * 
*/
void MDWSDescription::setMinMax(const std::vector<double> &minVal,const std::vector<double> &maxVal)
{
    dimMin.assign(minVal.begin(),minVal.end());
    dimMax.assign(maxVal.begin(),maxVal.end());

    this->checkMinMaxNdimConsistent(dimMin,dimMax);
}

/**get vector of minimal and maximal values from the class */
void MDWSDescription::getMinMax(std::vector<double> &min,std::vector<double> &max)const
{
     min.assign(this->dimMin.begin(),this->dimMin.end());
     max.assign(this->dimMax.begin(),this->dimMax.end());
}
//******************************************************************************************************************************************
//*************   STATIC HELPER FUNCTIONS
//******************************************************************************************************************************************
/** Helper function to obtain the energy of incident neutrons from the input workspaec
  *
  *@param pHost the pointer to the algorithm to work with
  *
  *@returns the incident energy of the neutrons or quet NaN if can not retrieve one 
*/
double MDWSDescription::getEi(API::MatrixWorkspace_const_sptr inWS2D)
{
    if(!inWS2D.get()){
        throw(std::invalid_argument(" getEi: invoked on empty input workspace "));
    }
    Kernel::PropertyWithValue<double>  *pProp(NULL);
    try{
       pProp  =dynamic_cast<Kernel::PropertyWithValue<double>  *>(inWS2D->run().getProperty("Ei"));
    }catch(...){
    }
    if(!pProp){
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (*pProp); 
}
/** function extracts the coordinates from additional workspace porperties and places them to proper position within 
  *  the vector of MD coodinates for the particular workspace.
  *
  *  @param dimProperyNames  -- names of properties which should be treated as dimensions
  *
  *  @returns AddCoord       -- vector of additional coordinates (derived from WS properties) for current multidimensional event
 */
void MDWSDescription::fillAddProperties(Mantid::API::MatrixWorkspace_const_sptr inWS2D,const std::vector<std::string> &dimProperyNames,std::vector<coord_t> &AddCoord)
{
     size_t nDimPropNames = dimProperyNames.size();
     if(AddCoord.size()!=nDimPropNames)AddCoord.resize(nDimPropNames);

     for(size_t i=0;i<nDimPropNames;i++){
         //HACK: A METHOD, Which converts TSP into value, correspondent to time scale of matrix workspace has to be developed and deployed!
         Kernel::Property *pProperty = (inWS2D->run().getProperty(dimProperyNames[i]));
         Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pProperty);  
         if(run_property){
                AddCoord[i]=coord_t(run_property->firstValue());
         }else{
              // e.g Ei can be a property and dimenson
              Kernel::PropertyWithValue<double> *proc_property = dynamic_cast<Kernel::PropertyWithValue<double> *>(pProperty);  
              if(!proc_property){
                std::string ERR = " Can not interpret property, used as dimension.\n Property: "+dimProperyNames[i]+
                                  " is neither a time series (run) property nor a property with value<double>";
                 throw(std::invalid_argument(ERR));
              }
              AddCoord[i]  = coord_t(*(proc_property));
         }
     }
}

/** function verifies the consistency of the min and max dimsnsions values  checking if all necessary 
 * values vere defined and min values are smaller then mav values */
void MDWSDescription::checkMinMaxNdimConsistent(const std::vector<double> &minVal,const std::vector<double> &maxVal)
{
  if(minVal.size()!=maxVal.size())
  {
      std::string ERR = " number of specified min dimension values: "+boost::lexical_cast<std::string>(minVal.size())+
                        " and number of max values: "+boost::lexical_cast<std::string>(maxVal.size())+
                        " are not consistent\n";
      throw(std::invalid_argument(ERR));
  }
    
  for(size_t i=0; i<minVal.size();i++)
  {
    if(maxVal[i]<=minVal[i])
    {
      std::string ERR = " min value "+boost::lexical_cast<std::string>(minVal[i])+
                        " not less then max value"+boost::lexical_cast<std::string>(maxVal[i])+" in direction: "+
                         boost::lexical_cast<std::string>(i)+"\n";
      throw(std::invalid_argument(ERR));
    }
  }
}

/// function checks if source workspace still has information about detectors. Some ws (like rebinned one) do not have this information any more. 
bool MDWSDescription::isDetInfoLost(Mantid::API::MatrixWorkspace_const_sptr inWS2D)
{
    API::NumericAxis *pYAxis = dynamic_cast<API::NumericAxis *>(inWS2D->getAxis(1));
    if(pYAxis){
        // if this is numeric axis, then the detector's information has been lost:
        return true;
    }          
    return false;
}
/** function retrieves copy of the oriented lattice from the workspace */
boost::shared_ptr<Geometry::OrientedLattice> MDWSDescription::getOrientedLattice(Mantid::API::MatrixWorkspace_const_sptr inWS2D)
{
    // try to get the WS oriented lattice
    boost::shared_ptr<Geometry::OrientedLattice> orl;
    if(inWS2D->sample().hasOrientedLattice()){        
        orl = boost::shared_ptr<Geometry::OrientedLattice>(new Geometry::OrientedLattice(inWS2D->sample().getOrientedLattice()));      
    }
    return orl;

}

}
}
