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
      
      //BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      //mustBePositive->setLower(0);
      //declareProperty("spectrum_min",0, mustBePositive);
      //declareProperty("spectrum_max",0, mustBePositive->clone());
      
      declareProperty(new ArrayProperty<int>("spectrum_list"));
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


      // create the workspace that is going to hold the instrument 
      DataObjects::Workspace2D_sptr localWorkspace 
        = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D"));

      // load the instrument into this workspace
      runLoadInstrument(localWorkspace);

      // Get instrument which was loaded into the workspace
      boost::shared_ptr<Instrument> instrument = localWorkspace->getInstrument();

      // Get detectors stored in instrument and create dummy c-arrays for the purpose
      // of calling method of SpectraDetectorMap 
      const std::map<int, Geometry::IDetector*>& detCache = instrument->getDetectorCache();
      int number_spectra = static_cast<int>(detCache.size());
      int *spec = new int[number_spectra];
      int *udet = new int[number_spectra];

      std::map<int, Geometry::IDetector*>::const_iterator it;
      int counter = 0;
      for ( it = detCache.begin(); it != detCache.end(); ++it )
      {
        counter++;
        spec[counter-1] = counter;    // have no feeling of how best to number these spectra
                                      // and sure whether the way it is done here is the best way...
        udet[counter-1] = it->first;
      }

      boost::shared_ptr<SpectraDetectorMap> localmap=localWorkspace->getSpectraMap();
      localmap->populate(spec,udet,number_spectra,instrument.get());

      int spectraLength = 1; // put spectra lenght to 1. Since assumes histograms this mean x axis one longer
      localWorkspace->initialize(number_spectra, spectraLength+1, spectraLength);

      // Not sure this is really necessary - but it should not do any harm
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

      counter = 0;
      std::vector<double> x; x.push_back(1.0); x.push_back(2.0);
      std::vector<double> v(1.0,1);
      std::vector<double> e(1.0,1);
      for (int i = 1; i <= number_spectra; ++i)
      {
        localWorkspace->setData(counter, v, e);
        localWorkspace->setX(counter, x);
        localWorkspace->setErrorHelper(counter,GaussianErrorHelper::Instance());
        localWorkspace->getAxis(1)->spectraNo(counter)= i;  // Not entirely sure if this 100% ok
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
      std::string instrumentID = m_filename.substr(stripPath+1);  // get the 1st 3 letters of filename part
      // force ID to upper case
      //std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID;
      
      Algorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
      loadInst->setPropertyValue("Filename", m_filename);
      loadInst->setProperty<Workspace_sptr>("Workspace",localWorkspace);

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
