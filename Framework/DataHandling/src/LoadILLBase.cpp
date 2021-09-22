// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadILLBase.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataHandling/NexusEntryProvider.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyManagerProperty.h"

#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;

namespace Mantid::DataHandling {

/**
 * @brief LoadILLBase::init initializes the common properties all loaders must have
 * This method is final and cannot be overridden.
 * If there are extra properties to declare, override declareExtraProperties
 */
void LoadILLBase::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "The run number of the path of the data file to load.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
  declareProperty(std::make_unique<PropertyManagerProperty>("PatchNexusMetadataEntries", Direction::Input),
                  "JSON formatted key-value pairs to add/override nexus entries.");
  declareProperty(std::make_unique<PropertyManagerProperty>("PatchWorkspaceSampleLogs", Direction::Input),
                  "JSON formatted key-value pairs to add/override sample logs.");
  declareExtraProperties();
}

/**
 * @brief LoadILLBase::setOutputWorkspace sets the output workspace
 */
void LoadILLBase::setOutputWorkspace() { setProperty<API::Workspace_sptr>("OutputWorkspace", m_workspace); }

/**
 * @brief LoadILLBase::getInstrumentDefinitionFilePath returns the fully resolved IDF file path.
 * The instrument is loaded via file and not name, as the variants should not be declared in the facilities xml.
 * @return the fully resolved path of the IDF file
 */
std::string LoadILLBase::getInstrumentDefinitionFilePath() {
  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  Poco::Path file(m_instrument + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

/**
 * @brief LoadILLBase::bootstrap prepares and configures the loader
 * The instrument, acquisition mode are resolved first.
 * Then, the schema of mandatory metadata is validated.
 * The reason for this is that depending on instrument and acquisition mode, there might be some extra entries that are
 * mandatory. Only when this validation passes, it builds the workspace as provided by the overridden pure virtual
 * method. There again, the type, size and the shape of the workspace depends on the mode and the instrument.
 * Once the workspace is instantiated, it resolves the start time and loads the instrument.
 */
void LoadILLBase::bootstrap() {
  const std::string filename = getPropertyValue("Filename");
  PropertyManager_sptr pmp = getProperty("PatchNexusMetadataEntries");
  m_nxroot = std::make_unique<NXRoot>(filename);
  m_nep = std::make_unique<NexusEntryProvider>(filename, *pmp);
  m_helper = std::make_shared<LoadHelper>();
  m_mode = resolveAcqMode();
  m_instrument = resolveInstrument();
  validateMetadata();
  m_workspace = buildWorkspace();
  resolveStartTime();
  loadInstrument();
}

/**
 * @brief LoadILLBase::addSampleLogs adds all the metadata from nexus to the output workspace
 */
void LoadILLBase::addSampleLogs() {
  NXhandle nxHandle;
  NXstatus nxStat = NXopen(getPropertyValue("Filename").c_str(), NXACC_READ, &nxHandle);
  if (nxStat != NX_ERROR) {
    if (isOutputGroup()) {
      WorkspaceGroup_sptr wsg = std::dynamic_pointer_cast<WorkspaceGroup>(m_workspace);
      for (int i = 0; i < wsg->getNumberOfEntries(); ++i) {
        MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(i));
        auto const entryName = std::string("entry" + std::to_string(i));
        m_helper->addNexusFieldsToWsRun(nxHandle, ws->mutableRun(), entryName);
      }
    } else {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(m_workspace);
      m_helper->addNexusFieldsToWsRun(nxHandle, ws->mutableRun());
    }
    NXclose(&nxHandle);
  }
}

/**
 * @brief LoadILLBase::patchSampleLogs adds/overrides sample logs as provided in the dictionary
 * This is useful if one needs to override those metadata, which are just sample logs.
 * In other words, those, that are not used in the loader itself, but are put in the logs for future use later in data
 * reduction. This way there is more flexibility. When overriding the sample logs, the keys are the names of the logs,
 * not to be confused with nexus entries.
 */
void LoadILLBase::patchSampleLogs() {
  const PropertyManager_sptr logsToPatch = getProperty("PatchWorkspaceSampleLogs");
  const auto properties = logsToPatch->getProperties();
  if (isOutputGroup()) {
    WorkspaceGroup_sptr wsg = std::dynamic_pointer_cast<WorkspaceGroup>(m_workspace);
    for (int i = 0; i < wsg->getNumberOfEntries(); ++i) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(i));
      for (const auto &prop : properties) {
        ws->mutableRun().addProperty(prop, true);
      }
    }
  } else {
    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(m_workspace);
    for (const auto &prop : properties) {
      ws->mutableRun().addProperty(prop, true);
    }
  }
}

/**
 * @brief LoadILLBase::loadInstrument loads the instrument to the workspace
 */
void LoadILLBase::loadInstrument() {
  const std::string idf = getInstrumentDefinitionFilePath();
  auto loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("Filename", idf);
  loadInst->setProperty("Workspace", m_workspace);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(true));
  loadInst->execute();
}

/**
 * @brief LoadILLBase::resolveStartTime resolves the start time in ISO format
 * It is necessary to add it in the right format to the logs prior to loading the instrument.
 * This way one can benefit from time-resolved values of instrument parameters.
 */
void LoadILLBase::resolveStartTime() {
  const std::string startTime = "start_time";
  m_timestamp = m_helper->dateTimeInIsoFormat(m_nxroot->openFirstEntry().getString(startTime));
  if (isOutputGroup()) {
    WorkspaceGroup_sptr wsg = std::dynamic_pointer_cast<WorkspaceGroup>(m_workspace);
    for (int i = 0; i < wsg->getNumberOfEntries(); ++i) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(i));
      ws->mutableRun().addProperty(startTime, m_timestamp, true);
    }
  } else {
    MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(m_workspace);
    ws->mutableRun().addProperty(startTime, m_timestamp, true);
  }
}

/**
 * @brief LoadILLBase::resolveInstrument resolves the name of the instrument
 * Optionally, appends the variant, if overridden.
 * @return the name of fully resolved instrument as mantid knows it
 */
std::string LoadILLBase::resolveInstrument() {
  NXEntry firstEntry = m_nxroot->openFirstEntry();
  const std::string instrumentPath = m_helper->findInstrumentNexusPath(firstEntry);
  std::string instrumentName = m_helper->getStringFromNexusPath(firstEntry, instrumentPath + "/name");
  boost::to_upper(instrumentName);
  return instrumentName + resolveVariant();
}

/**
 * @brief LoadILLBase::exec executes the core logic.
 * This method is final and cannot be overridden.
 * The sequence of what it does is not commutative.
 */
void LoadILLBase::exec() {
  bootstrap();
  loadAndFillData();
  configureBeamline();
  addSampleLogs();
  patchSampleLogs();
  setOutputWorkspace();
}

} // namespace Mantid::DataHandling
