// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadLLB.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace Mantid::DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;
using namespace HistogramData;

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadLLB)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadLLB::LoadLLB()
    : m_supportedInstruments{"MIBEMOL"}, m_numberOfTubes{0}, m_numberOfPixelsPerTube{0}, m_numberOfChannels{0},
      m_numberOfHistograms{0}, m_wavelength{0.0}, m_channelWidth{0.0} {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadLLB::name() const { return "LoadLLB"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadLLB::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadLLB::category() const { return "DataHandling\\Nexus"; }

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadLLB::confidence(Kernel::NexusDescriptor &descriptor) const {
  // fields existent only at the LLB
  if (descriptor.pathExists("/nxentry/program_name") && descriptor.pathExists("/nxentry/subrun_number") &&
      descriptor.pathExists("/nxentry/total_subruns")) {
    return 80;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadLLB::init() {
  const std::vector<std::string> exts{".nxs", ".hdf"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to load");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadLLB::exec() {

  std::string filename = getPropertyValue("Filename");
  NXRoot root(filename);
  NXEntry entry = root.openFirstEntry();
  setInstrumentName(entry);

  initWorkSpace(entry);
  runLoadInstrument(); // just to get IDF
  loadTimeDetails(entry);
  loadDataIntoTheWorkSpace(entry);

  loadRunDetails(entry);
  loadExperimentDetails(entry);

  runLoadInstrument();

  setProperty("OutputWorkspace", m_localWorkspace);
}

void LoadLLB::setInstrumentName(const NeXus::NXEntry &entry) {

  m_instrumentPath = "nxinstrument";
  m_instrumentName = LoadHelper::getStringFromNexusPath(entry, m_instrumentPath + "/name");

  if (m_instrumentName.empty()) {
    throw std::runtime_error("Cannot read the instrument name from the Nexus file!");
  }
  g_log.debug() << "Instrument Name: " << m_instrumentName << " in NxPath: " << m_instrumentPath << '\n';
}

void LoadLLB::initWorkSpace(const NeXus::NXEntry &entry) {

  // read in the data
  NXData dataGroup = entry.openNXData("nxdata");
  NXInt data = dataGroup.openIntData();

  m_numberOfTubes = static_cast<size_t>(data.dim0());
  m_numberOfPixelsPerTube = 1;
  m_numberOfChannels = static_cast<size_t>(data.dim1());

  // dim0 * m_numberOfPixelsPerTube is the total number of detectors
  m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;

  g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << '\n';
  g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << '\n';
  g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << '\n';

  // Now create the output workspace
  // Might need to get this value from the number of monitors in the Nexus file
  // params:
  // workspace type,
  // total number of spectra + (number of monitors = 0),
  // bin boundaries = m_numberOfChannels + 1
  // Z/time dimension
  m_localWorkspace = WorkspaceFactory::Instance().create("Workspace2D", m_numberOfHistograms, m_numberOfChannels + 1,
                                                         m_numberOfChannels);
  m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  m_localWorkspace->setYUnitLabel("Counts");
}

void LoadLLB::loadTimeDetails(const NeXus::NXEntry &entry) {

  m_wavelength = entry.getFloat("nxbeam/incident_wavelength");
  // Apparently this is in the wrong units
  // http://iramis.cea.fr/Phocea/file.php?class=page&reload=1227895533&file=21/How_to_install_and_use_the_Fitmib_suite_v28112008.pdf
  m_channelWidth = entry.getInt("nxmonitor/channel_width") * 0.1;

  g_log.debug("Nexus Data:");
  g_log.debug() << " ChannelWidth: " << m_channelWidth << '\n';
  g_log.debug() << " Wavelength: " << m_wavelength << '\n';
}

void LoadLLB::loadDataIntoTheWorkSpace(const NeXus::NXEntry &entry) {

  // read in the data
  NXData dataGroup = entry.openNXData("nxdata");
  NXFloat data = dataGroup.openFloatData();
  data.load();

  // EPP
  int calculatedDetectorElasticPeakPosition = getDetectorElasticPeakPosition(data);

  // Assign time bin to first X entry
  setTimeBinning(m_localWorkspace->mutableX(0), calculatedDetectorElasticPeakPosition, m_channelWidth);

  Progress progress(this, 0.0, 1.0, m_numberOfTubes * m_numberOfPixelsPerTube);
  size_t spec = 0;
  for (size_t i = 0; i < m_numberOfTubes; ++i) {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
      float *data_p = &data(static_cast<int>(i), static_cast<int>(j));
      m_localWorkspace->setHistogram(spec++, m_localWorkspace->binEdges(0),
                                     Counts(data_p, data_p + m_numberOfChannels));
      progress.report();
    }
  }

  g_log.debug() << "Data loading inti WS done....\n";
}

int LoadLLB::getDetectorElasticPeakPosition(const NeXus::NXFloat &data) {

  std::vector<int> cumulatedSumOfSpectras(m_numberOfChannels, 0);
  for (size_t i = 0; i < m_numberOfTubes; i++) {
    const float *data_p = &data(static_cast<int>(i), 0);
    float currentSpec = 0;

    for (size_t j = 0; j < m_numberOfChannels; ++j)
      currentSpec += data_p[j];

    if (i > 0) {
      cumulatedSumOfSpectras[i] = cumulatedSumOfSpectras[i - 1] + static_cast<int>(currentSpec);
    } else {
      cumulatedSumOfSpectras[i] = static_cast<int>(currentSpec);
    }
  }
  auto it = std::max_element(cumulatedSumOfSpectras.begin(), cumulatedSumOfSpectras.end());

  int calculatedDetectorElasticPeakPosition;
  if (it == cumulatedSumOfSpectras.end()) {
    throw std::runtime_error("No Elastic peak position found while analyzing the data!");
  } else {
    // calculatedDetectorElasticPeakPosition = *it;
    calculatedDetectorElasticPeakPosition = static_cast<int>(std::distance(cumulatedSumOfSpectras.begin(), it));

    if (calculatedDetectorElasticPeakPosition == 0) {
      throw std::runtime_error("No Elastic peak position found while analyzing "
                               "the data. Elastic peak position is ZERO!");
    } else {
      g_log.debug() << "Calculated Detector EPP: " << calculatedDetectorElasticPeakPosition << '\n';
    }
  }
  return calculatedDetectorElasticPeakPosition;
}

void LoadLLB::setTimeBinning(HistogramX &histX, int elasticPeakPosition, double channelWidth) {

  double l1 = m_localWorkspace->spectrumInfo().l1();
  double l2 = m_localWorkspace->spectrumInfo().l2(1);

  double theoreticalElasticTOF =
      (LoadHelper::calculateTOF(l1, m_wavelength) + LoadHelper::calculateTOF(l2, m_wavelength)) * 1e6; // microsecs

  g_log.debug() << "elasticPeakPosition : " << static_cast<float>(elasticPeakPosition) << '\n';
  g_log.debug() << "l1 : " << l1 << '\n';
  g_log.debug() << "l2 : " << l2 << '\n';
  g_log.debug() << "theoreticalElasticTOF : " << theoreticalElasticTOF << '\n';

  for (size_t i = 0; i < m_numberOfChannels + 1; ++i) {
    histX[i] = theoreticalElasticTOF + channelWidth * static_cast<double>(static_cast<int>(i) - elasticPeakPosition) -
               channelWidth / 2; // to make sure the bin is in the middle of the elastic peak
  }
}

void LoadLLB::loadRunDetails(const NXEntry &entry) {

  API::Run &runDetails = m_localWorkspace->mutableRun();

  //	int runNum = entry.getInt("run_number");
  //	std::string run_num = boost::lexical_cast<std::string>(runNum);
  //	runDetails.addProperty("run_number", run_num);

  std::string start_time = entry.getString("start_time");
  // start_time = getDateTimeInIsoFormat(start_time);
  runDetails.addProperty("run_start", start_time);

  std::string end_time = entry.getString("end_time");
  // end_time = getDateTimeInIsoFormat(end_time);
  runDetails.addProperty("run_end", end_time);

  double wavelength = entry.getFloat("nxbeam/incident_wavelength");
  runDetails.addProperty<double>("wavelength", wavelength);

  double energy = LoadHelper::calculateEnergy(wavelength);
  runDetails.addProperty<double>("Ei", energy, true); // overwrite

  std::string title = entry.getString("title");
  runDetails.addProperty("title", title);
  m_localWorkspace->setTitle(title);
}

/*
 * Load data about the Experiment.
 *
 * TODO: This is very incomplete. In ISIS they much more info in the nexus file
 *than ILL.
 *
 * @param entry :: The Nexus entry
 */
void LoadLLB::loadExperimentDetails(const NXEntry &entry) {

  // TODO: Do the rest
  // Pick out the geometry information

  (void)entry;

  //	std::string description = boost::lexical_cast<std::string>(
  //			entry.getFloat("sample/description"));
  //
  //	m_localWorkspace->mutableSample().setName(description);

  //	m_localWorkspace->mutableSample().setThickness(static_cast<double>
  //(isis_raw->spb.e_thick));
  //	m_localWorkspace->mutableSample().setHeight(static_cast<double>
  //(isis_raw->spb.e_height));
  //	m_localWorkspace->mutableSample().setWidth(static_cast<double>
  //(isis_raw->spb.e_width));
}

/**
 * Run the Child Algorithm LoadInstrument.
 */
void LoadLLB::runLoadInstrument() {

  auto loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {

    // TODO: depending on the m_numberOfPixelsPerTube we might need to load a
    // different IDF

    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loadInst->execute();
  } catch (...) {
    g_log.information("Cannot load the instrument definition.");
  }
}

} // namespace Mantid::DataHandling
