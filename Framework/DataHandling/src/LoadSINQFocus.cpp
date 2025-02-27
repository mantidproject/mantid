// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadSINQFocus.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Sample.h"
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

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadSINQFocus)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadSINQFocus::LoadSINQFocus()
    : m_supportedInstruments{"FOCUS"}, m_numberOfTubes{0}, m_numberOfPixelsPerTube{0}, m_numberOfChannels{0},
      m_numberOfHistograms{0} {

  this->useAlgorithm("LoadSINQ");
  this->deprecatedDate("2013-10-28");
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadSINQFocus::name() const { return "LoadSINQFocus"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadSINQFocus::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadSINQFocus::category() const { return "DataHandling\\Nexus"; }

//----------------------------------------------------------------------------------------------

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadSINQFocus::confidence(Kernel::NexusDescriptor &descriptor) const {

  // fields existent only at the SINQ (to date Loader only valid for focus)
  if (descriptor.isEntry("/entry1/FOCUS/SINQ")) {
    return 80;
  } else {
    return 0;
  }
}

//-----------------------------------------1-----------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadSINQFocus::init() {
  const std::vector<std::string> exts{".nxs", ".hdf"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to load");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadSINQFocus::exec() {

  std::string filename = getPropertyValue("Filename");
  NXRoot root(filename);
  NXEntry entry = root.openFirstEntry();
  setInstrumentName(entry);

  initWorkSpace(entry);

  loadDataIntoTheWorkSpace(entry);

  loadRunDetails(entry);
  loadExperimentDetails(entry);

  runLoadInstrument();

  setProperty("OutputWorkspace", m_localWorkspace);
}

/*
 * Set global variables:
 * m_instrumentPath
 * m_instrumentName
 * Note that the instrument in the nexus file is of the form "FOCUS at SINQ"
 *
 */
void LoadSINQFocus::setInstrumentName(const NeXus::NXEntry &entry) {

  m_instrumentPath = LoadHelper::findInstrumentNexusPath(entry);

  if (m_instrumentPath.empty()) {
    throw std::runtime_error("Cannot set the instrument name from the Nexus file!");
  }
  m_instrumentName = LoadHelper::getStringFromNexusPath(entry, m_instrumentPath + "/name");
  size_t pos = m_instrumentName.find(' ');
  m_instrumentName = m_instrumentName.substr(0, pos);
}

void LoadSINQFocus::initWorkSpace(const NeXus::NXEntry &entry) {

  // read in the data
  NXData dataGroup = entry.openNXData("merged");
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

void LoadSINQFocus::loadDataIntoTheWorkSpace(NeXus::NXEntry &entry) {

  // read in the data
  NXData dataGroup = entry.openNXData("merged");
  NXInt data = dataGroup.openIntData();
  data.load();

  std::vector<double> timeBinning = LoadHelper::getTimeBinningFromNexusPath(entry, "merged/time_binning");
  auto &x = m_localWorkspace->mutableX(0);
  x.assign(timeBinning.begin(), timeBinning.end());

  Progress progress(this, 0.0, 1.0, m_numberOfTubes * m_numberOfPixelsPerTube);
  size_t spec = 0;
  for (size_t i = 0; i < m_numberOfTubes; ++i) {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j) {
      if (spec > 0) {
        // just copy the time binning axis to every spectra
        m_localWorkspace->setSharedX(spec, m_localWorkspace->sharedX(0));
      }
      // Assign Y
      int *data_p = &data(static_cast<int>(i), static_cast<int>(j));
      m_localWorkspace->mutableY(spec).assign(data_p, data_p + m_numberOfChannels);
      // Assign Error
      auto &E = m_localWorkspace->mutableE(spec);
      std::transform(data_p, data_p + m_numberOfChannels, E.begin(), LoadSINQFocus::calculateError);
      ++spec;
      progress.report();
    }
  }
  g_log.debug() << "Data loading into WS done....\n";
}

void LoadSINQFocus::loadRunDetails(const NXEntry &entry) {

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

  double wavelength = entry.getFloat(m_instrumentPath + "/monochromator/lambda");
  runDetails.addProperty<double>("wavelength", wavelength);

  double energy = entry.getFloat(m_instrumentPath + "/monochromator/energy");
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
void LoadSINQFocus::loadExperimentDetails(const NXEntry &entry) {

  std::string sampleName = boost::lexical_cast<std::string>(entry.getFloat("sample/name"));
  m_localWorkspace->mutableSample().setName(sampleName);
}

/**
 * Run the Child Algorithm LoadInstrument.
 */
void LoadSINQFocus::runLoadInstrument() {

  auto loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {

    // TODO: depending on the m_numberOfPixelsPerTube we might need to load a
    // different IDF

    loadInst->setPropertyValue("InstrumentName", m_instrumentName);
    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
    loadInst->execute();
  } catch (...) {
    g_log.information("Cannot load the instrument definition.");
  }
}

} // namespace Mantid::DataHandling
