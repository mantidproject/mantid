/*WIKI*

 Loads an ILL Ascii / Raw data file into a [[Workspace2D]] with the given name.
 To date this Loader is only compatible with non TOF instruments.

 Supported instruments : ILL D2B

 *WIKI*/

#include "MantidMDAlgorithms/LoadILLAscii.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidMDAlgorithms/LoadILLAsciiHelper.h"

#include <Poco/TemporaryFile.h>
#include <boost/shared_ptr.hpp>

#include <fstream>
#include <iterator> // std::distance

namespace Mantid {
namespace MDAlgorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadILLAscii)

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadILLAscii::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &filePath = descriptor.filename();
  // Avoid some known file types that have different loaders
  int confidence(0);

  if (descriptor.isAscii()) {
    confidence = 10; // Low so that others may try
    ILLParser p(filePath);
    std::string instrumentName = p.getInstrumentName();

    g_log.information() << "Instrument name: " << instrumentName << "\n";

    if (std::find(m_supportedInstruments.begin(), m_supportedInstruments.end(),
                  instrumentName) != m_supportedInstruments.end())
      confidence = 80;
  }

  return confidence;
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLAscii::name() const { return "LoadILLAscii"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLAscii::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLAscii::category() const {
  return "MDAlgorithms\\DataHandling";
}

//----------------------------------------------------------------------------------------------
/// Summary of behaviour
const std::string LoadILLAscii::summary() const {
  return "Loads ILL Raw data in Ascii format.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLAscii::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, ""),
      "Name of the data file to load.");
  declareProperty(make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name to use for the output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLAscii::exec() {

  std::string filename = getPropertyValue("Filename");

  // Parses ascii file and fills the data scructures
  ILLParser illAsciiParser(filename);
  loadInstrumentName(illAsciiParser);
  illAsciiParser.parse();
  loadExperimentDetails(illAsciiParser);

  // get local references to the parsed file
  const std::vector<std::vector<int>> &spectraList =
      illAsciiParser.getSpectraList();
  const std::vector<std::map<std::string, std::string>> &spectraHeaderList =
      illAsciiParser.getSpectraHeaderList();

  // list containing all parsed scans. 1 scan => 1 ws
  std::vector<API::MatrixWorkspace_sptr> workspaceList;
  workspaceList.reserve(spectraList.size());

  // iterate parsed file
  std::vector<std::vector<int>>::const_iterator iSpectra;
  std::vector<std::map<std::string, std::string>>::const_iterator
      iSpectraHeader;

  Progress progress(this, 0.0, 1.0, spectraList.size());
  for (iSpectra = spectraList.begin(),
      iSpectraHeader = spectraHeaderList.begin();
       iSpectra < spectraList.end() && iSpectraHeader < spectraHeaderList.end();
       ++iSpectra, ++iSpectraHeader) {

    size_t spectrumIndex = std::distance(spectraList.begin(), iSpectra);
    g_log.debug() << "Reading Spectrum: " << spectrumIndex << '\n';

    std::vector<int> thisSpectrum = *iSpectra;
    API::MatrixWorkspace_sptr thisWorkspace =
        WorkspaceFactory::Instance().create("Workspace2D", thisSpectrum.size(),
                                            2, 1);

    thisWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create("Wavelength");
    thisWorkspace->setYUnitLabel("Counts");

    // Set this workspace position
    double currentPositionAngle =
        illAsciiParser.getValue<double>("angles*1000", *iSpectraHeader) / 1000;
    setWorkspaceRotationAngle(thisWorkspace, currentPositionAngle);

    //
    loadsDataIntoTheWS(thisWorkspace, thisSpectrum);
    loadIDF(thisWorkspace); // assigns data to the instrument

    workspaceList.push_back(thisWorkspace);

    //		// just to see the list of WS in MantidPlot if needed for
    // debugging
    //		std::stringstream outWorkspaceNameStream;
    //		outWorkspaceNameStream << "test" <<
    // std::distance(spectraList.begin(),
    // iSpectra);
    //		AnalysisDataService::Instance().addOrReplace(outWorkspaceNameStream.str(),
    // thisWorkspace);

    progress.report("Loading scans...");
  }

  // Merge the workspace list into a single WS with a virtual instrument
  IMDEventWorkspace_sptr outWorkspace = mergeWorkspaces(workspaceList);
  setProperty("OutputWorkspace", outWorkspace);
}

/**
 * Sets the workspace position based on the rotation angle
 * See tag logfile in file instrument/D2B_Definition.xml
 */
void LoadILLAscii::setWorkspaceRotationAngle(API::MatrixWorkspace_sptr ws,
                                             double rotationAngle) {

  API::Run &runDetails = ws->mutableRun();
  auto *p = new Mantid::Kernel::TimeSeriesProperty<double>("rotangle");

  p->addValue(DateAndTime::getCurrentTime(), rotationAngle);
  runDetails.addLogData(p);
}

/**
 * Loads instrument details
 */
void LoadILLAscii::loadExperimentDetails(ILLParser &p) {

  m_wavelength = p.getValueFromHeader<double>("wavelength");
  g_log.debug() << "Wavelength: " << m_wavelength << '\n';
}

void LoadILLAscii::loadInstrumentName(ILLParser &p) {

  m_instrumentName = p.getInstrumentName();
  if (m_instrumentName == "") {
    throw std::runtime_error("Cannot read instrument name from the data file.");
  }
  g_log.debug() << "Instrument name set to: " + m_instrumentName << '\n';

  m_wavelength = p.getValueFromHeader<double>("wavelength");
  g_log.debug() << "Wavelength: " << m_wavelength << '\n';
}

/**
 * Run the Child Algorithm LoadInstrument.
 */
void LoadILLAscii::loadIDF(API::MatrixWorkspace_sptr &workspace) {

  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", workspace);
    loadInst->setProperty("RewriteSpectraMap",
                          OptionalBool(OptionalBool::True));
    loadInst->execute();
  } catch (...) {
    g_log.information("Cannot load the instrument definition.");
  }
}

/**
 * Loads the scan into the workspace
 */
void LoadILLAscii::loadsDataIntoTheWS(API::MatrixWorkspace_sptr &thisWorkspace,
                                      const std::vector<int> &thisSpectrum) {

  thisWorkspace->mutableX(0)[0] = m_wavelength - 0.001;
  thisWorkspace->mutableX(0)[1] = m_wavelength + 0.001;

  size_t spec = 0;
  for (auto value : thisSpectrum) {

    if (spec > 0) {
      // just copy the time binning axis to every spectra
      thisWorkspace->setSharedX(spec, thisWorkspace->sharedX(0));
    }
    // Assign Y
    thisWorkspace->mutableY(spec)[0] = value;
    // Assign Error
    thisWorkspace->mutableE(spec)[0] = value * value;

    ++spec;
  }

  loadIDF(thisWorkspace); // assigns data to the instrument
}

/**
 * Merge all workspaces and create a virtual new instrument.
 *
 * To date this is slow as we are passing through a temp file and then
 * it is loaded in the ImportMDEventWorkspace.
 * If this loader is to used at the ILL, the better option is to avoid
 * a MDWS and go ahead with the merge instruments.
 *
 * @return MD Event workspace
 *
 */
IMDEventWorkspace_sptr LoadILLAscii::mergeWorkspaces(
    std::vector<API::MatrixWorkspace_sptr> &workspaceList) {

  Poco::TemporaryFile tmpFile;
  const std::string &tempFileName = tmpFile.path();
  g_log.debug() << "Dumping WSs in a temp file: " << tempFileName << '\n';

  std::ofstream myfile;
  myfile.open(tempFileName.c_str());
  myfile << "DIMENSIONS\n";
  myfile << "x X m 100\n";
  myfile << "y Y m 100\n";
  myfile << "z Z m 100\n";
  myfile << "# Signal, Error, DetectorId, RunId, coord1, coord2, ... to end of "
            "coords\n";
  myfile << "MDEVENTS\n";

  if (!workspaceList.empty()) {
    Progress progress(this, 0.0, 1.0, workspaceList.size());

    for (size_t pos = 0; pos < workspaceList.size(); ++pos) {
      const auto &workspace = workspaceList[pos];

      std::size_t nHist = workspace->getNumberHistograms();
      const auto &specInfo = workspace->spectrumInfo();
      for (std::size_t i = 0; i < nHist; ++i) {
        Geometry::IDetector_const_sptr det = workspace->getDetector(i);
        const auto &signal = workspace->y(i);
        const auto &error = workspace->e(i);
        myfile << signal[0] << " ";
        myfile << error[0] << " ";
        myfile << specInfo.detector(i).getID() << " ";
        myfile << pos << " ";
        const auto &detPos = specInfo.position(i);
        myfile << detPos.X() << " ";
        myfile << detPos.Y() << " ";
        myfile << detPos.Z() << " ";
        myfile << '\n';
      }
      progress.report("Creating MD WS");
    }
    myfile.close();

    IAlgorithm_sptr importMDEWS =
        createChildAlgorithm("ImportMDEventWorkspace");
    // Now execute the Child Algorithm.
    try {
      importMDEWS->setPropertyValue("Filename", tempFileName);
      importMDEWS->setProperty("OutputWorkspace", "Test");
      importMDEWS->executeAsChildAlg();
    } catch (std::exception &exc) {
      throw std::runtime_error(
          std::string("Error running ImportMDEventWorkspace: ") + exc.what());
    }
    IMDEventWorkspace_sptr workspace =
        importMDEWS->getProperty("OutputWorkspace");
    if (!workspace)
      throw(std::runtime_error("Can not retrieve results of child algorithm "
                               "ImportMDEventWorkspace"));

    return workspace;
  } else {
    throw std::runtime_error("Error: No workspaces were found to be merged!");
  }
}

} // namespace MDAlgorithms
} // namespace Mantid
