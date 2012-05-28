#include "MantidMDEvents/ConvToMDEventsBase.h"


namespace Mantid
{
namespace MDEvents
{

// logger for conversion  
   Kernel::Logger& ConvToMDEventsBase::convert_log =Kernel::Logger::get("MD-Algorithms");


/** method which initates all main class variables
  * @param pWS2D      -- shared pointer to input matirx workspace to process
  * @param detLoc     -- class with information about datecotrs, partially transformed for convenient Q calculations
  * @param WSD        -- class describing the target workspace. 
  *                      the algorithm uses target workspace limints, transformation matix from source to the target workspace and the parameters, needed for  
  *                      unit conversion (if any) 
  * @param pWSWrapper -- shared pointer to target MD Event workspace to add converted events to.
*/
size_t  ConvToMDEventsBase::initialize(Mantid::API::MatrixWorkspace_sptr pWS2D, const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
{
        TWS   = WSD;
        // set up output MD workspace wrapper
        pWSWrapper = inWSWrapper;
        
        // Copy ExperimentInfo (instrument, run, sample) to the output WS
        API::ExperimentInfo_sptr ei(pWS2D->cloneExperimentInfo());
        runIndex            = pWSWrapper->pWorkspace()->addExperimentInfo(ei);

        n_dims       = this->pWSWrapper->nDimensions();
       // allocate space for single MDEvent coordinates 
        Coord.resize(this->n_dims);

        // retrieve the class which does the conversion of workspace data into MD WS coordinates;
        pQConverter = MDTransfFactory::Instance().create(TWS.AlgID);

        inWS2D = pWS2D;

        pQConverter->initialize(TWS);
       // initialize units conversion which can/or can not be necessary depending on input ws/converter requested units;
       UnitConversion.initialize(detLoc,pWS2D,pQConverter->usedUnitID());

        
        size_t n_spectra =inWS2D->getNumberHistograms();
        return n_spectra;
};  

ConvToMDEventsBase::ConvToMDEventsBase()
{}



} // endNamespace MDAlgorithms
}
