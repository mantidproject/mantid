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

#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace ::NeXus;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  // TODO: Uncomment this when we want it visible in Mantid.
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

      declareProperty(new PropertyWithValue<bool>("MakeCopy", true, Direction::Input),
                      "Copy the NeXus file first before appending (optional, default True).");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void AppendGeometryToSNSNexus::exec()
  {          

      // Retrieve filename from the properties
      m_filename = getPropertyValue("Filename");

      // Are we going to make a copy of the file ?
      m_makeNexusCopy = getProperty("MakeCopy");

      if (m_makeNexusCopy)
      {
          Poco::File originalFile(m_filename);
          Poco::Path originalPath(m_filename);

          if (originalFile.exists())
          {
              Poco::File duplicateFile(Poco::Path(Poco::Path::temp(), originalPath.getFileName()));
              originalFile.copyTo(duplicateFile.path());
              g_log.notice() << "Copied " << m_filename << " to " << duplicateFile.path() << "." << std::endl ;
              m_filename = duplicateFile.path();
          }
          else
          {
              g_log.error() << "Cannot copy a file that doesn't exist! (" << originalFile.path() << ")." << std::endl;
          }

      }

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


      // Get the number of detectors (just for progress reporting)
      // Get the number of histograms/detectors
      const size_t numDetectors = ws->getInstrument()->getDetectorIDs().size();

      this->progress = new API::Progress(this, 0.0, 1.0, numDetectors);


      // Get the instrument
      Geometry::Instrument_const_sptr instrument = ws->getInstrument();

      // Get the sample (needed to calculate distances)
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      // Get the source (moderator)
      Geometry::IObjComponent_const_sptr source = instrument->getSource();


      // Get a list of the NXdetectors in the NeXus file
      std::vector<std::string> nxdetectors;

      // Open the NeXus file
      ::NeXus::File nxfile(m_filename, NXACC_RDWR);

      //typedef std::map<std::string,std::string> string_map_t;
      std::map<std::string,std::string>::const_iterator root_iter;
      std::map<std::string, std::string> entries = nxfile.getEntries();

      for (root_iter = entries.begin(); root_iter != entries.end(); ++root_iter)
      {
          // Open all NXentry
          if (root_iter->second == "NXentry")
          {
              nxfile.openGroup(root_iter->first, "NXentry");

              // Get a list of items within the entry.
              std::map<std::string, std::string> entry_items = nxfile.getEntries();
              // Create an iterator for this
              std::map<std::string,std::string>::const_iterator entry_iter;


              for (entry_iter = entry_items.begin(); entry_iter != entry_items.end(); ++entry_iter)
              {
                  // Look for an instrument
                  if (entry_iter->second == "NXinstrument")
                  {
                      // Open the instrument
                      nxfile.openGroup(entry_iter->first, "NXinstrument");
                      std::map<std::string, std::string> instr_items = nxfile.getEntries();
                      std::map<std::string,std::string>::const_iterator instr_iter;
                      for (instr_iter = instr_items.begin(); instr_iter != instr_items.end(); ++instr_iter)
                      {
                          // Look for NXdetectors
                          if (instr_iter->second == "NXdetector")
                          {
                              g_log.debug() << "Detector called '" << instr_iter->first << "' found." << std::endl;
                              std::string bankName = instr_iter->first;
                              std::vector<Geometry::IDetector_const_sptr> dets;
                              ws->getInstrument()->getDetectorsInBank(dets, bankName);

                              if (!dets.empty())
                              {
                                  nxfile.openGroup(bankName, "NXdetector");

                                  // Let's create some vectors for the parameters to write
                                  // Pixel IDs
                                  std::vector<int> pixel_id;
                                  std::vector<double> distance;
                                  std::vector<double> polar_angle;
                                  std::vector<double> azimuthal_angle;

                                  pixel_id.reserve(dets.size());
                                  distance.reserve(dets.size());
                                  polar_angle.reserve(dets.size());
                                  azimuthal_angle.reserve(dets.size());


                                  for (std::size_t i=0; i < dets.size(); i++)
                                  {
                                      pixel_id.push_back(dets[i]->getID());
                                      distance.push_back(dets[i]->getDistance(*sample));
                                      azimuthal_angle.push_back(dets[i]->getPhi());
                                      polar_angle.push_back(ws->detectorTwoTheta(dets[i]));
                                  }

                                  // Write Pixel ID to file
                                  nxfile.writeData("pixel_id_new", pixel_id);

                                  // Write Secondary Flight Path to file
                                  nxfile.writeData("distance_new", distance);
                                  nxfile.openData("distance_new");
                                  nxfile.putAttr("units", "metre");
                                  nxfile.closeData();

                                  // Write Polar Angle (2theta) to file
                                  nxfile.writeData("polar_angle_new", polar_angle);
                                  nxfile.openData("polar_angle_new");
                                  nxfile.putAttr("units", "radian");
                                  nxfile.closeData();

                                  // Write Azimuthal Angle (Phi) to file
                                  nxfile.writeData("azimuthal_angle_new", azimuthal_angle);
                                  nxfile.openData("azimuthal_angle_new");
                                  nxfile.putAttr("units", "radian");
                                  nxfile.closeData();

                                  nxfile.closeGroup(); // close NXdetector

                                  this->progress->report(dets.size());
                              }
                              else
                              {
                                  throw std::runtime_error("Could not find any detectors for the bank named" + bankName +
                                                           " that is listed in the NeXus file."
                                                           "Check that it exists in the Instrument Definition FIle.");
                              }

                          }
                      }

                      nxfile.closeGroup(); // NXinstrument

                  }
                  // Look for monitors
                  else if (entry_iter->second == "NXmonitor")
                  {
                        g_log.debug() << "Monitor called '" << entry_iter->first << "' found." << std::endl;
                        nxfile.openGroup(entry_iter->first, "NXmonitor");

                        Geometry::IComponent_const_sptr monitor = instrument->getComponentByName(entry_iter->first);

                        // Write Pixel ID to file
                        //nxfile.writeData("pixel_id_new", monitor->get);

                        double source_monitor = source->getDistance(*monitor);
                        double source_sample = source->getDistance(*sample);

                        g_log.debug() << "source->monitor=" << source_monitor << std::endl;
                        g_log.debug() << "source->sample=" << source_sample << std::endl;
                        g_log.debug() << "sample->monitor=" << (source_monitor-source_sample) << std::endl;

                        // Distance
                        nxfile.writeData("distance_new", (source_monitor-source_sample));
                        nxfile.openData("distance_new");
                        nxfile.putAttr("units", "metre");
                        nxfile.closeData();


                        nxfile.closeGroup();  // NXmonitor

                  }


              }


          }
          else
          {
              g_log.error() << "There are no NXentry nodes in the specified NeXus file." << std::endl;
          }

      }


      // Clean up the workspace
      //AnalysisDataService::Instance().remove(workspaceName);
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
