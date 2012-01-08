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
dim_units(nDimesnions,"Momentum"),
dim_min(nDimesnions,-1),
dim_max(nDimesnions,1)
{}
}
}