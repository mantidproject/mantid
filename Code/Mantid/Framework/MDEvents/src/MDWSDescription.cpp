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
MDWSDescription::build_from_MDWS(const API::IMDEventWorkspace_const_sptr &pWS)
{
    this->nDims = pWS->getNumDims();
    // prepare all arrays:
    this->dimMin.resize(nDims);
    this->dimMax.resize(nDims);
    this->dimNames.resize(nDims);
    this->dimIDs.resize(nDims);
    this->dimUnits.resize(nDims);   
    this->nBins.resize(nDims);
    for(size_t i=0;i<nDims;i++){
        const Geometry::IMDDimension *pDim = pWS->getDimension(i).get();
        dimMin[i]  = pDim->getMinimum();
        dimMax[i]  = pDim->getMaximum();
        dimNames[i]= pDim->getName();
        dimIDs[i]  = pDim->getDimensionId();
        dimUnits[i]= pDim->getUnits();   
        nBins[i]   = pDim->getNBins();
    }

}
/**function compares old workspace description with the new workspace description, defined by the algorithm properties and 
 * selects/changes the properties which can be changed through input parameters given that target MD workspace exist   
 *
 * @param NewMDWorkspaceD -- MD workspace description, obtained from algorithm parameters
 *
 * @returns NewMDWorkspaceD -- modified md workspace description, which is compartible with existing MD workspace
 *
*/
void  
MDWSDescription::compareDescriptions(MDEvents::MDWSDescription &NewMDWorkspaceD)
{
    UNUSED_ARG(NewMDWorkspaceD);
}

/** function verifies the consistency of the min and max dimsnsions values  checking if all necessary 
 * values vere defined and min values are smaller then mav values */
void 
MDWSDescription::checkMinMaxNdimConsistent(Mantid::Kernel::Logger& g_log)const
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

MDWSDescription::MDWSDescription(size_t nDimesnions):
nDims(nDimesnions),
emode(-1),
Ei(std::numeric_limits<double>::quiet_NaN()),
dimMin(nDimesnions,-1),
dimMax(nDimesnions,1),
dimNames(nDimesnions,"mdn"),
dimIDs(nDimesnions,"mdn_"),
dimUnits(nDimesnions,"Momentum"),
convert_to_hkl(false),
u(1,0,0),
v(0,1,0),
is_uv_default(true),
defailtQNames(3),
detInfoLost(false)
{
    for(size_t i=0;i<nDimesnions;i++){
        dimIDs[i]= dimIDs[i]+boost::lexical_cast<std::string>(i);
    }
    defailtQNames[0]="Qh";
    defailtQNames[1]="Qk";
    defailtQNames[2]="Ql";

}
std::string DLLExport sprintfd(const double data, const double eps)
{
     // truncate to eps decimal points
     float dist = float((int(data/eps+0.5))*eps);
     return boost::str(boost::format("%d")%dist);

}

std::string makeAxisName(const Kernel::V3D &Dir,const std::vector<std::string> &QNames)
{
    double eps(1.e-3);
    std::string name("["),separator=",";
    for(size_t i=0;i<3;i++){
        double dist=std::fabs(Dir[i]);
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

}
}
