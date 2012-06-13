/*WIKI*
This algorithm is intended to append the geometry information into a raw NeXus file.
It is initially for use only at the SNS, as it is needed for the currently upgrade program.
But there is nothing preventing it being used elsewhere.

The algorithm takes the geometry information in the IDF togther with the log values in a given NeXus file
and calculates the resolved positions of all the detectors and then writes this into the NeXus file specified.

*WIKI*/

#include "MantidDataHandling/AppendGeometryToSNSNexus.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidNexusCPP/NeXusException.hpp"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace ::NeXus;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  //DECLARE_ALGORITHM(AppendGeometryToSNSNexus)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  AppendGeometryToSNSNexus::AppendGeometryToSNSNexus()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  AppendGeometryToSNSNexus::~AppendGeometryToSNSNexus()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string AppendGeometryToSNSNexus::name() const { return "AppendGeometryToSNSNexus";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int AppendGeometryToSNSNexus::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string AppendGeometryToSNSNexus::category() const { return "DataHandling\\DataAcquisition";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void AppendGeometryToSNSNexus::initDocs()
  {
    this->setWikiSummary("Appends the resolved instrument geometry to a NeXus file.");
    this->setOptionalMessage("Appends the resolved instrument geometry to a NeXus file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void AppendGeometryToSNSNexus::init()
  {
      // Declare potential extensions for input NeXus file
      std::vector<std::string> extensions;
      extensions.push_back(".nxs");
      extensions.push_back(".h5");

      declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, extensions),
                      "The name of the NeXus file to append geometry to.");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void AppendGeometryToSNSNexus::exec()
  {

      // Retrieve filename from the properties
      m_filename = getPropertyValue("Filename");

      // Let's look for the instrument name
      m_instrument = getInstrumentName(m_filename);

      // Temp workspace name to load the instrument into
      std::string workspaceName = "__" + m_instrument + "_geometry_ws";

      // Now what is the instrument definition filename ?
      m_idf_filename = ExperimentInfo::getInstrumentFilename(m_instrument);
      g_log.debug() << "Loading instrument definition from " << m_idf_filename << "." << std::endl;

      // Let's load the empty instrument
      IAlgorithm_sptr alg = AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
      alg->initialize();
      alg->setPropertyValue("Filename", m_idf_filename);
      alg->setPropertyValue("OutputWorkspace", workspaceName);
      alg->execute();

      MatrixWorkspace_sptr ws;
      ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);

      // Get the number of histograms/detectors
      const size_t numHistograms = ws->getNumberHistograms();

      g_log.notice() << "Number of Histograms = " << numHistograms << std::endl;

      for (size_t i=0; i < numHistograms; ++i)
      {
        Geometry::IDetector_const_sptr det = ws->getDetector(i);
        if (det->isMonitor())
            // TODO: Do the correct thing for the monitors
        else
        {
            // Detector

        }

      }


      // Clean up the workspace
      AnalysisDataService::Instance().remove(workspaceName);
  }


  //----------------------------------------------------------------------------------------------



  //----------------------------------------------------------------------------------------------
 std::string AppendGeometryToSNSNexus::getInstrumentName(const std::string &nxfilename)
 {
     std::string instrument;

     // Open the NeXus file
     ::NeXus::File nxfile(nxfilename);
     // What is the first entry ?
     std::map<std::string, std::string> entries = nxfile.getEntries();

     // For now, let's just open the first entry
     nxfile.openGroup(entries.begin()->first, "NXentry");
     g_log.debug() << "Using entry '" << entries.begin()->first << "' to determine instrument name." << std::endl;

     nxfile.openGroup("instrument", "NXinstrument");
     try
     {
         nxfile.openData("name");
         instrument = nxfile.getStrData();
     }
     catch (::NeXus::Exception &)
     {
         // TODO: try and get the instrument name from the filename instead.
         instrument = "";
     }

     g_log.debug() << " Instrument name read from NeXus file is " << instrument << std::endl;

     return instrument;
 }

} // namespace Mantid
} // namespace DataHandling
