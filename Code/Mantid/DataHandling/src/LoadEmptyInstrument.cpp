//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"

#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadEmptyInstrument)

    using namespace Kernel;
    using namespace API;

    // Initialise the logger
    Logger& LoadEmptyInstrument::g_log = Logger::get("LoadEmptyInstrument");

    /// Empty default constructor
    LoadEmptyInstrument::LoadEmptyInstrument() : 
      Algorithm(), m_filename()
    {
    }


    /// Initialisation method.
    void LoadEmptyInstrument::init()
    {
      std::vector<std::string> exts;
      exts.push_back("XML");
      exts.push_back("xml");			
      declareProperty("Filename","",new FileValidator(exts));
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));
      
      BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
      mustBePositive->setLower(0.0);
      declareProperty("detector_value",1.0, mustBePositive);
      declareProperty("monitor_value",2.0, mustBePositive->clone());
    }

    /** Executes the algorithm. Reading in the file and creating and populating
     *  the output workspace
     * 
     *  @throw Exception::FileError If the RAW file cannot be found/opened
     *  @throw std::invalid_argument If the optional properties are set to invalid values
     */
    void LoadEmptyInstrument::exec()
    {
      // Retrieve the filename from the properties
      m_filename = getPropertyValue("Filename");

      // Get other properties
      double detector_value = getProperty("detector_value");
      double monitor_value = getProperty("monitor_value");

      // create the workspace that is going to hold the instrument 
      DataObjects::Workspace2D_sptr localWorkspace 
        = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D"));

      // load the instrument into this workspace
      runLoadInstrument(localWorkspace);

      // Get instrument which was loaded into the workspace
      boost::shared_ptr<IInstrument> instrument = localWorkspace->getInstrument();

      // Get detectors stored in instrument and create dummy c-arrays for the purpose
      // of calling method of SpectraDetectorMap 
      const std::map<int, Geometry::IDetector_sptr> detCache = instrument->getDetectors();
      int number_spectra = static_cast<int>(detCache.size());
      int *spec = new int[number_spectra];
      int *udet = new int[number_spectra];

      std::map<int, Geometry::IDetector_sptr>::const_iterator it;
      int counter = 0;
      for ( it = detCache.begin(); it != detCache.end(); ++it )
      {
        counter++;
        spec[counter-1] = counter;    // have no feeling of how best to number these spectra
                                      // and sure whether the way it is done here is the best way...
        udet[counter-1] = it->first;
      }

      SpectraMap_sptr localmap=localWorkspace->getSpectraMap();
      localmap->populate(spec,udet,number_spectra);

      int spectraLength = 1; // put spectra lenght to 1. Since assumes histograms this mean x axis one longer
      localWorkspace->initialize(number_spectra, spectraLength+1, spectraLength);

      // Not sure this is really necessary - but it should not do any harm
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

      counter = 0;
      std::vector<double> x; x.push_back(1.0); x.push_back(2.0);
      std::vector<double> v(detector_value, 1);
      std::vector<double> v_monitor(monitor_value, 1);
      std::vector<double> e(detector_value, 1);

      for ( it = detCache.begin(); it != detCache.end(); ++it )
      {
        if ( (it->second)->isMonitor() )
          localWorkspace->setData(counter, v_monitor, e);
        else
          localWorkspace->setData(counter, v, e);
        localWorkspace->setX(counter, x);
        localWorkspace->setErrorHelper(counter,GaussianErrorHelper::Instance());
        localWorkspace->getAxis(1)->spectraNo(counter)= counter+1;  // Not entirely sure if this 100% ok
        ++counter;
      }

      setProperty("OutputWorkspace",localWorkspace);
        
      
      // Clean up
      delete[] spec;
      delete[] udet;
    }


    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    void LoadEmptyInstrument::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");      
      if ( directoryName.empty() ) directoryName = "../Instrument";  // This is the assumed deployment directory for IDFs

      const int stripPath = m_filename.find_last_of("\\/");

      std::string fullPathIDF;
      if (stripPath != std::string::npos)
      {
        fullPathIDF = m_filename;   // since if path already provided don't modify m_filename
      }
      else
      {
        //std::string instrumentID = m_filename.substr(stripPath+1);
        fullPathIDF = directoryName + "/" + m_filename;
      }

      
      Algorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
      loadInst->setPropertyValue("Filename", fullPathIDF);
      loadInst->setProperty<MatrixWorkspace_sptr>("Workspace",localWorkspace);

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->execute();
      }
      catch (std::runtime_error& err)
      {
        g_log.error("Unable to successfully run LoadInstrument sub-algorithm");
      }
    }



  } // namespace DataHandling
} // namespace Mantid
