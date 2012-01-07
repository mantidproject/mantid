#include "MantidMDAlgorithms/IConvertToMDEventsMethods.h"
namespace Mantid
{
namespace MDAlgorithms
{

/** function extracts the coordinates from additional workspace porperties and places them to proper position within the vector of MD coodinates for 
    the particular workspace.

    @param Coord             -- vector of coordinates for current multidimensional event
    @param nd                -- number of the event's dimensions
    @param n_ws_properties   -- number of dimensions, provided by the workspace itself. E.g., processed inelastic matrix
                                workspace with provides 4 dimensions, matrix workspace in elastic mode -- 3 dimensions, powder 
                                -- 1 for elastic and 2 for inelastic mode. Number of these properties is determined by the deployed algorithm
                                The coordinates, obtained from the workspace placed first in the array of coordinates, and the coordinates, 
                                obtained from dimensions placed after them. 
    *@returns        -- true if all coordinates are within the range allowed for the algorithm and false otherwise

 */
bool 
IConvertToMDEventsMethods::fillAddProperties(std::vector<coord_t> &Coord,size_t nd,size_t n_ws_properties)
{
     for(size_t i=n_ws_properties;i<nd;i++){
         //HACK: A METHOD, Which converts TSP into value, correspondent to time scale of matrix workspace has to be developed and deployed!
         Kernel::Property *pProperty = (inWS2D->run().getProperty(TWS.dim_names[i]));
         Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pProperty);  
         if(run_property){
                Coord[i]=run_property->firstValue();
         }else{
              // e.g Ei can be a property and dimenson
              Kernel::PropertyWithValue<double> *proc_property = dynamic_cast<Kernel::PropertyWithValue<double> *>(pProperty);  
              if(!proc_property){
                 convert_log.error()<<" property: "<<this->TWS.dim_names[i]<<" is neither a time series (run) property nor a property with double value\n";
                 throw(std::invalid_argument(" can not interpret property, used as dimension"));
              }
              Coord[i]  = *(proc_property);
         }
        if(Coord[i]<TWS.dim_min[i] || Coord[i]>=TWS.dim_max[i])return false;
     }
     return true;
}

  

/** function verifies the consistency of the min and max dimsnsions values  checking if all necessary 
 * values vere defined and min values are smaller then mav values
*/
void 
MDWSDescription::checkMinMaxNdimConsistent(Mantid::Kernel::Logger& g_log)const
{
  if(this->dim_min.size()!=this->dim_max.size()||this->dim_min.size()!=this->n_activated_dimensions)
  {
      g_log.error()<<" number of specified min dimension values: "<<dim_min.size()<<", number of max values: "<<dim_max.size()<<
                     " and total number of target dimensions: "<<n_activated_dimensions<<" are not consistent\n";
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

} // endNamespace MDAlgorithms
}