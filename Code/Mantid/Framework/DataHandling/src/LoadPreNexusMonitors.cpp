#include "MantidDataHandling/LoadPreNexusMonitors.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/BinaryFile.h"

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/SAX/InputSource.h>

#include <boost/lexical_cast.hpp>
#include <boost/shared_array.hpp>

#include <fstream>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <iterator>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadPreNexusMonitors)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// Some constants for property names
static const std::string RUNINFO_FILENAME("RunInfoFilename");
static const std::string WORKSPACE_OUT("OutputWorkspace");

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void LoadPreNexusMonitors::init() {
  // Filename for the runinfo file.
  declareProperty(new FileProperty(RUNINFO_FILENAME, "", FileProperty::Load,
                                   "_runinfo.xml"),
                  "The filename of the runinfo file for a particular run. "
                  "Allowed Values are: _runinfo.xml");

  // The output workspace
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(WORKSPACE_OUT, "",
                                                         Direction::Output),
                  "The workspace to load the monitors into.");

  // Make sure things are initialised.
  nMonitors = 0;
}

void LoadPreNexusMonitors::exec() {
  // time of flight channel parameters
  double tmin = 0.0;
  double tstep = 0.0;
  int tchannels = 0;
  std::string instrumentName;

  // Vectors to store monitor parameters
  std::vector<std::string> monitorFilenames;
  std::vector<int> monitorIDs;

  // Get the Runinfo filename from the property
  std::string runinfo_filename = this->getPropertyValue(RUNINFO_FILENAME);

  // TODO: Extract the directory that the runinfo file is in.
  // Create a Poco Path object for runinfo filename
  Poco::Path runinfoPath(runinfo_filename, Poco::Path::PATH_GUESS);
  // Now lets get the directory
  Poco::Path dirPath(runinfoPath.parent());

  this->g_log.information("Monitor File Dir: " + dirPath.toString());

  // Some XML parsing magic...
  std::ifstream in(runinfo_filename.c_str());
  Poco::XML::InputSource src(in);

  Poco::XML::DOMParser parser;
  Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&src);

  Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();
  while (pNode) {

    // Get the beamline name.
    if (pNode->nodeName() == "RunInfo") {
      Poco::XML::Element *pRunInfoElement =
          static_cast<Poco::XML::Element *>(pNode);
      instrumentName = pRunInfoElement->getAttribute("instrument");
    }

    // Look for the section that contains the binning parameters.
    // This will end up with using the binning for the last monitor that is
    // found
    // TODO: Modify to store (and use) the TCBs for each monitor separately.
    if (pNode->nodeName() == "BeamMonitorInfo") {
      // Increment the number of monitors we've found
      ++nMonitors;

      Poco::XML::Element *pE = static_cast<Poco::XML::Element *>(pNode);
      g_log.debug() << "Beam Monitor " << pE->getAttribute("id") << std::endl;
      g_log.debug() << "\tname: " << pE->getAttribute("name") << std::endl;
      g_log.debug() << "\tdescription: " << pE->getAttribute("description")
                    << std::endl;

      // Now lets get the tof binning settings
      Poco::XML::Element *pTimeChannels =
          pE->getChildElement("NumTimeChannels");
      tmin =
          boost::lexical_cast<double>(pTimeChannels->getAttribute("startbin"));
      tstep = boost::lexical_cast<double>(pTimeChannels->getAttribute("width"));
    }

    // Look for the 'DataList' node to get the monitor dims.
    // TODO: Again we will only use the mast monitor value.
    if (pNode->nodeName() == "DataList") {
      // Get a list of the child elements
      Poco::AutoPtr<Poco::XML::NodeList> pDataListChildren =
          pNode->childNodes();
      for (unsigned long i = 0; i < pDataListChildren->length(); ++i) {
        // We only care about monitors
        if (pDataListChildren->item(i)->nodeName() == "monitor") {
          Poco::XML::Element *element =
              static_cast<Poco::XML::Element *>(pDataListChildren->item(i));
          monitorIDs.push_back(
              boost::lexical_cast<int>(element->getAttribute("id")));
          monitorFilenames.push_back(element->getAttribute("name"));
        }
      }
    }

    // Get the size of the files
    if (pNode->nodeName() == "FileFormats") {
      // Get a list of the child elements
      Poco::AutoPtr<Poco::XML::NodeList> pDataListChildren =
          pNode->childNodes();
      for (unsigned long i = 0; i < pDataListChildren->length(); ++i) {
        // We only care about monitors
        if (pDataListChildren->item(i)->nodeName() == "monitor") {
          std::string dims =
              static_cast<Poco::XML::Element *>(pDataListChildren->item(i))
                  ->getAttribute("dims");
          tchannels = boost::lexical_cast<int>(dims);
        }
      }
    }

    pNode = it.nextNode();
  }

  g_log.information() << "Found " << nMonitors << " beam monitors."
                      << std::endl;

  g_log.information() << "Number of Time Channels = " << tchannels << std::endl;

  // Now lets create the time of flight array.
  const int numberTimeBins = tchannels + 1;
  MantidVec time_bins(numberTimeBins);
  for (int i = 0; i < numberTimeBins; ++i) {
    time_bins[i] = tmin + (i)*tstep;
  }

  // Create the new workspace
  MatrixWorkspace_sptr localWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", nMonitors, numberTimeBins, tchannels);

  // temp buffer for file reading
  std::vector<uint32_t> buffer;

  for (int i = 0; i < nMonitors; i++) {
    // Now lets actually read the monitor files..
    Poco::Path pMonitorFilename(dirPath, monitorFilenames[i]);

    g_log.debug() << "Loading monitor file :" << pMonitorFilename.toString()
                  << std::endl;

    Kernel::BinaryFile<uint32_t> monitorFile(pMonitorFilename.toString());
    monitorFile.loadAllInto(buffer);

    MantidVec intensity(buffer.begin(), buffer.end());
    // Copy the same data into the error array
    MantidVec error(buffer.begin(), buffer.end());
    // Now take the sqrt()
    std::transform(error.begin(), error.end(), error.begin(),
                   (double (*)(double))sqrt);

    localWorkspace->dataX(i) = time_bins;
    localWorkspace->dataY(i) = intensity;
    localWorkspace->dataE(i) = error;
    // Just have spectrum number be the same as the monitor number but -ve.
    ISpectrum *spectrum = localWorkspace->getSpectrum(i);
    spectrum->setSpectrumNo(monitorIDs[i]);
    spectrum->setDetectorID(-monitorIDs[i]);
  }

  g_log.debug() << "Setting axis zero to TOF" << std::endl;

  // Set the units
  localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  localWorkspace->setYUnit("Counts");
  // TODO localWorkspace->setTitle(title);

  // Actually load the instrument
  this->runLoadInstrument(instrumentName, localWorkspace);

  // Set the property
  setProperty("OutputWorkspace", localWorkspace);
}

//-----------------------------------------------------------------------------
/** Load the instrument geometry File
 *  @param instrument :: instrument name.
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument
 * geometry
 */
void
LoadPreNexusMonitors::runLoadInstrument(const std::string &instrument,
                                        MatrixWorkspace_sptr localWorkspace) {

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try {
    loadInst->setPropertyValue("InstrumentName", instrument);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
    loadInst->setProperty("RewriteSpectraMap",
                          false); // We have a custom mapping
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around
    // a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument &e) {
    g_log.information()
        << "Invalid argument to LoadInstrument Child Algorithm : " << e.what()
        << std::endl;
    executionSuccessful = false;
  } catch (std::runtime_error &e) {
    g_log.information()
        << "Unable to successfully run LoadInstrument Child Algorithm : "
        << e.what() << std::endl;
    executionSuccessful = false;
  }

  // If loading instrument definition file fails
  if (!executionSuccessful) {
    g_log.error() << "Error loading Instrument definition file\n";
  } else {
    this->instrument_loaded_correctly = true;
  }
}

} // namespace DataHandling
} // namespace Mantid
