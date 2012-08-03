#include "MantidMDAlgorithms/ConvertToMDEventsWSBase.h"
#include "MantidAPI/ExperimentInfo.h"
namespace Mantid
{
namespace MDAlgorithms
{

// logger for loading workspaces  
   Kernel::Logger& ConvertToMDEventsWSBase::convert_log =Kernel::Logger::get("MD-Algorithms");

/** Helper function to obtain the units set along X-axis of the input workspace. 
  *
  *@param pHost the pointer to the algorithm to work with
  *
  *@returns the name(ID) of the unit, specified along X-axis of current workspace
*/
Kernel::Unit_sptr    
ConvertToMDEventsWSBase::getAxisUnits()const{
    if(!this->inWS2D.get()){
        convert_log.error()<<"getAxisUnits: invoked when input workspace is undefined\n";
        throw(std::logic_error(" should not be able to call this function when workpsace is undefined"));
    }
    API::NumericAxis *pAxis = dynamic_cast<API::NumericAxis *>(this->inWS2D->getAxis(0));
    if(!pAxis){
        convert_log.error()<<"getAxisUnits: can not obtained when first workspace axis is undefined or not numeric\n";
        throw(std::logic_error(" should not be able to call this function when X-axis is wrong"));
    }
    return this->inWS2D->getAxis(0)->unit();
}
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

bool ConvertToMDEventsWSBase::fillAddProperties(std::vector<coord_t> &Coord,size_t nd,size_t n_ws_properties)
{
     for(size_t i=n_ws_properties;i<nd;i++){
         //HACK: A METHOD, Which converts TSP into value, correspondent to time scale of matrix workspace has to be developed and deployed!
       std::vector<std::string> dimNames = TWS.getDimNames();
       Kernel::Property *pProperty = (inWS2D->run().getProperty(dimNames[i]));
       Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pProperty);  
       if(run_property){
                Coord[i]=coord_t(run_property->firstValue());
       }else{
              // e.g Ei can be a property and dimenson
              Kernel::PropertyWithValue<double> *proc_property = dynamic_cast<Kernel::PropertyWithValue<double> *>(pProperty);  
              if(!proc_property){
                 convert_log.error()<<" property: "<<dimNames[i]<<" is neither a time series (run) property nor a property with double value\n";
                 throw(std::invalid_argument(" can not interpret property, used as dimension"));
              }
              Coord[i]  = coord_t(*(proc_property));
       }
       std::vector<double> dimMin = TWS.getDimMin();
       std::vector<double> dimMax = TWS.getDimMax();
       if(Coord[i]<dimMin[i] || Coord[i]>=dimMax[i])return false;
     }
     return true;
}

/** method which initates all main class variables
   * @param WSD        -- class describing the target workspace. 
  *                      the algorithm uses target workspace limints, transformation matix from source to the target workspace and the parameters, needed for  
  *                      unit conversion (if any) 
  * @param pWSWrapper -- shared pointer to target MD Event workspace to add converted events to.
*/
size_t  ConvertToMDEventsWSBase::setUPConversion(const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
{
        TWS   = WSD;
        inWS2D= TWS.getInWS();
        pWSWrapper = inWSWrapper;
        
        // Copy ExperimentInfo (instrument, run, sample) to the output WS
        API::ExperimentInfo_sptr ei(inWS2D->cloneExperimentInfo());
        runIndex            = pWSWrapper->pWorkspace()->addExperimentInfo(ei);

  
        // remember pointer to the preprocessed detectors information
        pDetLoc    = TWS.getDetectors();

        n_dims       = this->pWSWrapper->nDimensions();       
        
        dim_min = TWS.getDimMin();
        dim_max = TWS.getDimMax();
        
        size_t n_spectra =inWS2D->getNumberHistograms();
        return n_spectra;
};  

ConvertToMDEventsWSBase::ConvertToMDEventsWSBase()
{}



} // endNamespace MDAlgorithms
}
