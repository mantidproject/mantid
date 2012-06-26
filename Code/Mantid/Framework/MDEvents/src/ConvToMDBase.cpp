#include "MantidMDEvents/ConvToMDBase.h"


namespace Mantid
{
namespace MDEvents
{

// logger for conversion  
   Kernel::Logger& ConvToMDBase::convert_log =Kernel::Logger::get("MD-Algorithms");


/** method which initates all main class variables
  * @param WSD        -- class describing the target workspace. 
  *                      the algorithm uses target workspace limints, transformation matix from source to the target workspace and the parameters, needed for  
  *                      unit conversion (if any) 
  * @param pWSWrapper -- shared pointer to target MD Event workspace to add converted events to.
*/
size_t  ConvToMDBase::initialize(const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
{
        pDetLoc = WSD.getDetectors();
        inWS2D  = WSD.getInWS();
            
        // set up output MD workspace wrapper
        pWSWrapper = inWSWrapper;
        
        // Copy ExperimentInfo (instrument, run, sample) to the output WS
        API::ExperimentInfo_sptr ei(inWS2D->cloneExperimentInfo());
        runIndex            = pWSWrapper->pWorkspace()->addExperimentInfo(ei);

        n_dims       = this->pWSWrapper->nDimensions();
       // allocate space for single MDEvent coordinates 
        Coord.resize(this->n_dims);

        // retrieve the class which does the conversion of workspace data into MD WS coordinates;
        pQConverter = MDTransfFactory::Instance().create(WSD.AlgID);

   
        // initialize the MD coordinates conversion class
        pQConverter->initialize(WSD);
       // initialize units conversion which can/or can not be necessary depending on input ws/converter requested units;
       ConvertToMD::EModes emode = WSD.getEMode();
       UnitConversion.initialize(WSD,pQConverter->inputUnitID(emode,inWS2D));

        
        size_t n_spectra =inWS2D->getNumberHistograms();
        return n_spectra;
};  

/** empty default constructor */
ConvToMDBase::ConvToMDBase()
{}



} // endNamespace MDAlgorithms
}
