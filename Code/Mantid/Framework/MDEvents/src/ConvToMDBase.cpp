#include "MantidMDEvents/ConvToMDBase.h"


namespace Mantid
{
  namespace MDEvents
  {

    // logger for conversion  
    Kernel::Logger ConvToMDBase::g_Log("MD-Algorithms");


    /** method which initates all main class variables
    * @param WSD        -- class describing the target workspace. 
    *                      the algorithm uses target workspace limints, transformation matix from source to the target workspace and the parameters, needed for  
    *                      unit conversion (if any) 
    * @param inWSWrapper -- shared pointer to target MD Event workspace to add converted events to.
    * @param ignoreZeros -- if true, 0 values on input histograms do not copied as events into resulting MD workspace. By false(default), they do.  
    */
    size_t  ConvToMDBase::initialize(const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper, bool ignoreZeros)
    {

      m_ignoreZeros = ignoreZeros;
      m_InWS2D = WSD.getInWS();
      // preprocessed detectors information:       
       // check if detector information has been precalculated:
      if(!WSD.m_PreprDetTable)throw(std::runtime_error("Detector information has to be precalculated before ConvToMDBase::initialize is deployed"));

      // number of valid spectra is equal to actual number of valid detectors in spectra-det map
      m_NSpectra = WSD.m_PreprDetTable->getLogs()->getPropertyValueAsType<uint32_t>("ActualDetectorsNum");
      m_detIDMap = WSD.m_PreprDetTable->getColVector<size_t>("detIDMap");
      m_detID    = WSD.m_PreprDetTable->getColVector<int>("DetectorID");

    

      // set up output MD workspace wrapper
      m_OutWSWrapper = inWSWrapper;
      // get the index which identify the run the source workspace came from.
      // This index will mark the workspace' events for diffetent worksapces to combine
      m_RunIndex            = WSD.getPropertyValueAsType<uint16_t>("RUN_INDEX");

      m_NDims       = m_OutWSWrapper->nDimensions();
      // allocate space for single MDEvent coordinates 
      m_Coord.resize(m_NDims);

      // retrieve the class which does the conversion of workspace data into MD WS coordinates;
      m_QConverter = MDTransfFactory::Instance().create(WSD.AlgID);


      // initialize the MD coordinates conversion class
      m_QConverter->initialize(WSD);
      // initialize units conversion which can/or can not be necessary depending on input ws/converter requested units;
      Kernel::DeltaEMode::Type emode = WSD.getEMode();
      m_UnitConversion.initialize(WSD,m_QConverter->inputUnitID(emode,m_InWS2D));


      size_t n_spectra =m_InWS2D->getNumberHistograms();

      // get property which controls multithreaded run. If present, this property describes number of threads (or one) deployed to run conversion
      // (this can be for debugging or other tricky reasons)
      m_NumThreads = -1;
      try
      {
        Kernel::Property *pProperty = m_InWS2D->run().getProperty("NUM_THREADS");
        Kernel::PropertyWithValue<double> *thrProperty = dynamic_cast<Kernel::PropertyWithValue<double> *>(pProperty);  
        if(thrProperty)
        {
          double nDThrheads = double(*(thrProperty));
          try
          {
            m_NumThreads = boost::lexical_cast<int>(nDThrheads);
            g_Log.information()<<"***--> NUM_THREADS property set to: "<<m_NumThreads<<std::endl;
            if(m_NumThreads<0)
                g_Log.information()<<"***--> This resets number of threads to the number of physical cores\n ";
            else if(m_NumThreads==0)
                g_Log.information()<<"***--> This disables multithreading\n ";
            else if(m_NumThreads>0)
                g_Log.information()<<"***--> Multithreading processing will launch "<<m_NumThreads<<" Threads\n";
          }
          catch(...){};
        }
      }
      catch(Kernel::Exception::NotFoundError &){}
   
     // Record any special coordinate system known to the description.
      m_coordinateSystem = WSD.getCoordinateSystem();

      return n_spectra;
    };  

    /** empty default constructor */
    ConvToMDBase::ConvToMDBase():m_NDims(0), // wrong non-initialized
      m_RunIndex(0), // defauld run index is 0
      m_NSpectra(0), // no valid spectra by default.
      m_NumThreads(-1), // run with all cores availible
      m_ignoreZeros(false), // 0-s added to workspace
      m_coordinateSystem(Mantid::API::None)
    { }



  } // endNamespace MDAlgorithms
}
