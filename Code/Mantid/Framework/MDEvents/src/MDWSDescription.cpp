#include "MantidMDEvents/MDWSDescription.h"

namespace Mantid
{
namespace MDEvents
{

/** function verifies the consistency of the min and max dimsnsions values  checking if all necessary 
 * values vere defined and min values are smaller then mav values */
void 
MDWSDescription::checkMinMaxNdimConsistent(Mantid::Kernel::Logger& g_log)const
{
  if(this->dim_min.size()!=this->dim_max.size()||this->dim_min.size()!=this->n_dims)
  {
      g_log.error()<<" number of specified min dimension values: "<<dim_min.size()<<", number of max values: "<<dim_max.size()<<
                     " and total number of target dimensions: "<<n_dims<<" are not consistent\n";
      throw(std::invalid_argument("wrong number of dimension limits"));
  }
    
  for(size_t i=0; i<this->dim_min.size();i++)
  {
    if(this->dim_max[i]<=this->dim_min[i])
    {
      g_log.error()<<" min value "<<dim_min[i]<<" not less then max value"<<dim_max[i]<<" in direction: "<<i<<std::endl;
      throw(std::invalid_argument("min limit not smaller then max limit"));
    }
  }
}

MDWSDescription::MDWSDescription(size_t nDimesnions):
n_dims(nDimesnions),
emode(-1),
Ei(std::numeric_limits<double>::quiet_NaN()),
dim_names(nDimesnions,"mdn"),
dim_IDs(nDimesnions,"mdn_"),
dim_units(nDimesnions,"Momentum"),
dim_min(nDimesnions,-1),
dim_max(nDimesnions,1),
is_uv_default(true),
u(1,0,0),
v(0,1,0),
defailt_qNames(3)
{
    for(size_t i=0;i<nDimesnions;i++){
        dim_IDs[i]= dim_IDs[i]+boost::lexical_cast<std::string>(i);
    }
    defailt_qNames[0]="Qh";
    defailt_qNames[1]="Qk";
    defailt_qNames[2]="Ql";

}

std::string makeAxisName(const Kernel::V3D &Dir,const std::vector<std::string> &QNames)
{
    double eps(1.e-3);
    std::string name("["),separator=",";
    for(size_t i=0;i<3;i++){
        double dist=abs(Dir[i]);
        if(i==2)separator="]";
        if(dist<eps){
            name+="0"+separator;
            continue;
        }
        if(Dir[i]<0){
           name+="-";
        }
        if(abs(dist-1)<eps){
            name+=QNames[i]+separator;
            continue;
        }
        // truncate to eps decimal points
        dist = float(int(dist/eps+0.5))*eps;
        name+= boost::str(boost::format("%d")%dist)+QNames[i]+separator;
    }

    return name;
}

}
}