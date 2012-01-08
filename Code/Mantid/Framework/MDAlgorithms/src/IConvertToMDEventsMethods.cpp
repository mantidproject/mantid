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

  


} // endNamespace MDAlgorithms
}