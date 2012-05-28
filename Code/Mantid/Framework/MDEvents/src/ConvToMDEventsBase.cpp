#include "MantidMDEvents/ConvToMDEventsBase.h"


namespace Mantid
{
namespace MDEvents
{

// logger for conversion  
   Kernel::Logger& ConvToMDEventsBase::convert_log =Kernel::Logger::get("MD-Algorithms");

/** function extracts the coordinates from additional workspace porperties and places them to proper position within 
  *  the vector of MD coodinates for the particular workspace.
  *
  *  @param Coord             -- vector of coordinates for current multidimensional event
  *  @param nd                -- number of the event's dimensions
  *  @param n_ws_properties   -- number of dimensions, provided by the workspace itself. E.g., processed inelastic matrix
                                  workspace with provides 4 dimensions, matrix workspace in elastic mode -- 3 dimensions, powder 
                                -- 1 for elastic and 2 for inelastic mode. Number of these properties is determined by the deployed algorithm
                                The coordinates, obtained from the workspace placed first in the array of coordinates, and the coordinates, 
                                obtained from dimensions placed after them. 
    *@returns        -- true if all coordinates are within the range allowed for the algorithm and false otherwise

 */

bool ConvToMDEventsBase::fillAddProperties(std::vector<coord_t> &Coord,size_t nd,size_t n_ws_properties)const
{
     for(size_t i=n_ws_properties;i<nd;i++){
         //HACK: A METHOD, Which converts TSP into value, correspondent to time scale of matrix workspace has to be developed and deployed!
         Kernel::Property *pProperty = (inWS2D->run().getProperty(TWS.dimNames[i]));
         Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(pProperty);  
         if(run_property){
                Coord[i]=coord_t(run_property->firstValue());
         }else{
              // e.g Ei can be a property and dimenson
              Kernel::PropertyWithValue<double> *proc_property = dynamic_cast<Kernel::PropertyWithValue<double> *>(pProperty);  
              if(!proc_property){
                 convert_log.error()<<" property: "<<this->TWS.dimNames[i]<<" is neither a time series (run) property nor a property with double value\n";
                 throw(std::invalid_argument(" can not interpret property, used as dimension"));
              }
              Coord[i]  = coord_t(*(proc_property));
         }
        if(Coord[i]<TWS.dimMin[i] || Coord[i]>=TWS.dimMax[i])return false;
     }
     return true;
}

/** method which initates all main class variables
  * @param pWS2D      -- shared pointer to input matirx workspace to process
  * @param detLoc     -- class with information about datecotrs, partially transformed for convenient Q calculations
  * @param WSD        -- class describing the target workspace. 
  *                      the algorithm uses target workspace limints, transformation matix from source to the target workspace and the parameters, needed for  
  *                      unit conversion (if any) 
  * @param pWSWrapper -- shared pointer to target MD Event workspace to add converted events to.
*/
size_t  ConvToMDEventsBase::initialize(Mantid::API::MatrixWorkspace_sptr pWS2D, ConvToMDPreprocDet &detLoc,const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
{
        TWS   = WSD;
        // set up output MD workspace wrapper
        pWSWrapper = inWSWrapper;
        
        // Copy ExperimentInfo (instrument, run, sample) to the output WS
        API::ExperimentInfo_sptr ei(pWS2D->cloneExperimentInfo());
        runIndex            = pWSWrapper->pWorkspace()->addExperimentInfo(ei);

        // set values may needed for unit conversion
        detLoc.setEmode(WSD.emode);
        detLoc.setEfix(WSD.Ei);

        // remember pointer to the preprocessed detectors information
        pDetLoc    = &detLoc;

        n_dims       = this->pWSWrapper->nDimensions();
       // allocate space for single MDEvent coordinates 
        Coord.resize(this->n_dims);

        // retrieve the class which does the conversion of workspace data into MD WS coordinates;
        pQConverter = MDTransfFactory::Instance().create(TWS.AlgID);

        inWS2D = pWS2D;
        
        size_t n_spectra =inWS2D->getNumberHistograms();
        return n_spectra;
};  

ConvToMDEventsBase::ConvToMDEventsBase()
{}



} // endNamespace MDAlgorithms
}
