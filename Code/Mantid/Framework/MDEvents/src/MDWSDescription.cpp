#include "MantidMDEvents/MDWSDescription.h"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

namespace Mantid
{
namespace MDEvents
{
/** the function builds MD event WS description from existing workspace. 
  * Primary used to obtain existing ws parameters 
*/
void 
MDWSDescription::buildFromMDWS(const API::IMDEventWorkspace_const_sptr &pWS)
{
    this->nDims = pWS->getNumDims();
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
/**function compares old workspace description with the new workspace description, defined by the algorithm properties and 
 * selects/changes the properties which can be changed through input parameters given that target MD workspace exist   
 *
 * @param NewMDWorkspaceD -- MD workspace description, obtained from algorithm parameters
 *
 * @returns NewMDWorkspaceD -- modified md workspace description, which is compartible with existing MD workspace
 *
*/
void  MDWSDescription::compareDescriptions(MDEvents::MDWSDescription &NewMDWorkspaceD)
{
    if(this->nDims != NewMDWorkspaceD.nDims)
    {
        std::string ERR = "Dimension numebrs are inconsistent: this workspace has "+boost::lexical_cast<std::string>(this->nDims)+" dimensions and target one: "+
                           boost::lexical_cast<std::string>(NewMDWorkspaceD.nDims);
         throw(std::invalid_argument(ERR)); 
    }
    if(this->emode<-1)
    {
        throw(std::invalid_argument("Workspace description has not been correctly defined, as emode has not been defined")); 
    }
    if(this->emode>0)
    {
         volatile double Ei = this->Ei;
        if(this->Ei!=Ei){
           std::string ERR = "Workspace description has not been correctly defined, as emode "+boost::lexical_cast<std::string>(this->emode)+" request input energy to be defined";
           throw(std::invalid_argument(ERR)); 
        }
    }
    //TODO: More thorough checks may be nesessary to prevent adding different kind of workspaces e.g 4D |Q|-dE-T-P workspace to Q3d+dE ws
}
/** When the workspace has been build from existing MDWrokspace, some target worskpace parameters can not be defined. 
   examples are emode or input energy, which is actually source workspace parameters
*/
void MDWSDescription::setUpMissingParameters(const MDEvents::MDWSDescription &SourceMDWSDescr)
{
    this->emode = SourceMDWSDescr.emode;
    this->Ei    = SourceMDWSDescr.Ei;
    if(SourceMDWSDescr.pLatt.get()){
        this->pLatt = std::auto_ptr<Geometry::OrientedLattice>(new Geometry::OrientedLattice(*(SourceMDWSDescr.pLatt)));
    }
    this->GoniomMatr = SourceMDWSDescr.GoniomMatr;

}


/** function verifies the consistency of the min and max dimsnsions values  checking if all necessary 
 * values vere defined and min values are smaller then mav values */
void MDWSDescription::checkMinMaxNdimConsistent(Mantid::Kernel::Logger& g_log)const
{
  if(this->dimMin.size()!=this->dimMax.size()||this->dimMin.size()!=this->nDims)
  {
      g_log.error()<<" number of specified min dimension values: "<<dimMin.size()<<", number of max values: "<<dimMax.size()<<
                     " and total number of target dimensions: "<<nDims<<" are not consistent\n";
      throw(std::invalid_argument("wrong number of dimension limits"));
  }
    
  for(size_t i=0; i<this->dimMin.size();i++)
  {
    if(this->dimMax[i]<=this->dimMin[i])
    {
      g_log.error()<<" min value "<<dimMin[i]<<" not less then max value"<<dimMax[i]<<" in direction: "<<i<<std::endl;
      throw(std::invalid_argument("min limit not smaller then max limit"));
    }
  }
}
//
std::vector<std::string> MDWSDescription::getDefaultDimIDQ3D(int dEMode)const
{
    std::vector<std::string> rez;
     if(dEMode==0)
     {
          rez.resize(3);
     }else{
       if (dEMode==1||dEMode==2)
       {
            rez.resize(4);
            rez[3]=default_dim_ID[dE_ID];
       }else{
            throw(std::invalid_argument("Unknown dE mode provided"));
       }
     }
     rez[0]=default_dim_ID[Q1_ID];
     rez[1]=default_dim_ID[Q2_ID];
     rez[2]=default_dim_ID[Q3_ID];
     return rez;
}



std::vector<std::string> MDWSDescription::getDefaultDimIDModQ(int dEMode)const
{
     std::vector<std::string> rez;

     if(dEMode==0){
          rez.resize(1);
     }else{
         if (dEMode==1||dEMode==2){
            rez.resize(2);
            rez[1]=default_dim_ID[dE_ID];
         }else{
             throw(std::invalid_argument("Unknown dE mode provided"));
         }
     }
     rez[0]=default_dim_ID[ModQ_ID];
     return rez;
}


/// empty constructor
MDWSDescription::MDWSDescription(size_t nDimesnions):
nDims(nDimesnions),
emode(-2),
Ei(std::numeric_limits<double>::quiet_NaN()),
dimMin(nDimesnions,-1),
dimMax(nDimesnions,1),
dimNames(nDimesnions,"mdn"),
dimIDs(nDimesnions,"mdn_"),
dimUnits(nDimesnions,"Momentum"),
convert_to_factor(NoScaling),
rotMatrix(9,0),       // set transformation matrix to 0 to certainly see rubbish if error
Wtransf(3,3,true),
GoniomMatr(3,3,true),
detInfoLost(false),
default_dim_ID(nDefaultID),
QScalingID(NCoordScalings)
{
    for(size_t i=0;i<nDimesnions;i++)
    {
        dimIDs[i]  = dimIDs[i]+boost::lexical_cast<std::string>(i);
        dimNames[i]=dimNames[i]+boost::lexical_cast<std::string>(i);
    }

 // this defines default dimension ID-s which are used to indentify dimensions when using the target MD workspace later
     // for ModQ transformation:
     default_dim_ID[ModQ_ID]="|Q|";
     // for Q3D transformation
     default_dim_ID[Q1_ID]="Q1";
     default_dim_ID[Q2_ID]="Q2";
     default_dim_ID[Q3_ID]="Q3";
     default_dim_ID[dE_ID]="DeltaE";

    QScalingID[NoScaling]="Q in A^-1";
    QScalingID[SingleScale]="Q in lattice units";
    QScalingID[OrthogonalHKLScale]="Orthogonal HKL";
    QScalingID[HKLScale]="HKL";


}

MDWSDescription & MDWSDescription::operator=(const MDWSDescription &rhs)
{
    this->nDims = rhs.nDims;
    this->emode = rhs.emode;
    this->Ei    = rhs.Ei;
    // prepare all arrays:
    this->dimMin = rhs.dimMin;
    this->dimMax = rhs.dimMax;
    this->dimNames=rhs.dimNames;
    this->dimIDs  =rhs.dimIDs;
    this->dimUnits=rhs.dimUnits;   
    this->nBins   =rhs.nBins;

    this->convert_to_factor= rhs.convert_to_factor;

    this->rotMatrix     = rhs.rotMatrix;
    this->AlgID         = rhs.AlgID;

    this->detInfoLost   = rhs.detInfoLost;

    if(rhs.pLatt.get()){
        this->pLatt = std::auto_ptr<Geometry::OrientedLattice>(new Geometry::OrientedLattice(*(rhs.pLatt)));
    }
    this->Wtransf    = rhs.Wtransf;
    this->GoniomMatr = rhs.GoniomMatr;

    return *this;

}


std::string makeAxisName(const Kernel::V3D &Dir,const std::vector<std::string> &QNames)
{
    double eps(1.e-3);
    Kernel::V3D DirCryst(Dir);

   // DirCryst.toMillerIndexes(eps);

    std::string name("["),separator=",";
    for(size_t i=0;i<3;i++){
        double dist=std::fabs(DirCryst[i]);
        if(i==2)separator="]";
        if(dist<eps){
            name+="0"+separator;
            continue;
        }
        if(Dir[i]<0){
           name+="-";
        }
        if(std::fabs(dist-1)<eps){
            name+=QNames[i]+separator;
            continue;
        }
        name+= sprintfd(dist,eps)+QNames[i]+separator;
    }

    return name;
}
std::string DLLExport sprintfd(const double data, const double eps)
{
     // truncate to eps decimal points
     float dist = float((int(data/eps+0.5))*eps);
     return boost::str(boost::format("%d")%dist);

}


CoordScaling MDWSDescription::getQScaling(const std::string &ScID)const
{
    CoordScaling theScaling(NCoordScalings);
    for(size_t i=0;i<NCoordScalings;i++)
    {
        if(QScalingID[i].compare(ScID)==0)
        {
            theScaling = (CoordScaling)i;
            break;
        }
    }
    if(theScaling==NCoordScalings) throw(std::invalid_argument(" The scale with ID: "+ScID+" is unavalible"));

    return theScaling;
}


}
}
