// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadILLBase.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDHistoWorkspace.h"
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
template <class W> void LoadILLBase<W>::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "The run number of the path of the data file to load.");
  declareProperty(std::make_unique<WorkspaceProperty<W>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
  declareProperty(std::make_unique<PropertyManagerProperty>("PatchNexusMetadataEntries", Direction::Input),
                  "JSON formatted key-value pairs to add/override nexus entries.");
  declareProperty(std::make_unique<PropertyManagerProperty>("PatchWorkspaceSampleLogs", Direction::Input),
                  "JSON formatted key-value pairs to add/override sample logs.");
  declareExtraProperties();
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
template <class W> void LoadILLBase<W>::setup() {
  const std::string filename = getPropertyValue("Filename");
  PropertyManager_sptr pmp = getProperty("PatchNexusMetadataEntries");
  m_nxroot = std::make_unique<NXRoot>(filename);
  m_nep = std::make_unique<NexusEntryProvider>(filename, *pmp);
  m_helper = std::make_unique<LoadHelper>();
  validateMetadata();
}

template <class W> void LoadILLBase<W>::wrapup(std::shared_ptr<W> ws) {
  addSampleLogs(ws);
  patchLogsForPatchedEntries(ws);
  patchSampleLogs(ws);
  setOutputWorkspace(ws);
}

/**
 * @brief LoadILLBase::setOutputWorkspace sets the output workspace
 */
template <class W> void LoadILLBase<W>::setOutputWorkspace(std::shared_ptr<W> ws) {
  setProperty<std::shared_ptr<W>>("OutputWorkspace", ws);
}

/**
 * @brief LoadILLBase::getInstrumentDefinitionFilePath returns the fully resolved IDF file path.
 * The instrument is loaded via file and not name, as the variants should not be declared in the facilities xml.
 * @return the fully resolved path of the IDF file
 */
template <class W> std::string LoadILLBase<W>::getInstrumentDefinitionFilePath(const std::string &iname) {
  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  Poco::Path file(iname + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

/**
 * @brief LoadILLBase::addSampleLogs adds all the metadata from nexus to the output workspace
 */
template <class W> void LoadILLBase<W>::addSampleLogs(std::shared_ptr<W> ws) {
  NXhandle nxHandle;
  NXstatus nxStat = NXopen(getPropertyValue("Filename").c_str(), NXACC_READ, &nxHandle);
  if (nxStat != NX_ERROR) {
    ExperimentInfo_sptr einfo = std::dynamic_pointer_cast<ExperimentInfo>(ws);
    m_helper->addNexusFieldsToWsRun(nxHandle, einfo->mutableRun());
  }
  NXclose(&nxHandle);
}

/**
 * @brief LoadILLBase::patchSampleLogs adds/overrides sample logs as provided in the dictionary
 * This is useful if one needs to override those metadata, which are just sample logs.
 * In other words, those, that are not used in the loader itself, but are put in the logs for future use later in data
 * reduction. This way there is more flexibility. When overriding the sample logs, the keys are the names of the logs,
 * not to be confused with nexus entries.
 */
template <class W> void LoadILLBase<W>::patchSampleLogs(std::shared_ptr<W> ws) {
  const PropertyManager_sptr logsToPatch = getProperty("PatchWorkspaceSampleLogs");
  const auto properties = logsToPatch->getProperties();
  ExperimentInfo_sptr einfo = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  for (const auto &prop : properties) {
    ws->mutableRun().addProperty(prop, true);
  }
}

/**
 * @brief LoadILLBase::patchLogsForPatchedEntries
 */
template <class W> void LoadILLBase<W>::patchLogsForPatchedEntries(std::shared_ptr<W> ws) {
  const PropertyManager_sptr entiresToPatch = getProperty("PatchNexusMetadataEntries");
  const auto properties = entiresToPatch->getProperties();
  ExperimentInfo_sptr einfo = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  for (const auto &prop : properties) {
    std::string logName = prop->name();
    // TODO: build the sample log name just as load helper would do
    ws->mutableRun().addProperty(logName, prop->value(), true);
  }
}

/**
 * @brief LoadILLBase::addStartTime adds the start time in ISO format to enable time-resolved instrument parameters
 */
template <class W> void LoadILLBase<W>::addStartTime(std::shared_ptr<W> ws) {
  ExperimentInfo_sptr einfo = std::dynamic_pointer_cast<ExperimentInfo>(ws);
  einfo->mutableRun().addProperty("start_time", getStartTime(), true);
}

/**
 * @brief LoadILLBase::loadInstrument loads the instrument to the workspace
 */
template <class W> void LoadILLBase<W>::loadInstrument(std::shared_ptr<W> ws) {
  addStartTime(ws);
  auto loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("Filename", getInstrumentDefinitionFilePath());
  loadInst->setProperty("Workspace", ws);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(true));
  loadInst->execute();
}

/**
 * @brief LoadILLBase::resolveStartTime resolves the start time in ISO format
 * It is necessary to add it in the right format to the logs prior to loading the instrument.
 * This way one can benefit from time-resolved values of instrument parameters.
 */
template <class W> std::string LoadILLBase<W>::getStartTime() {
  return m_helper->dateTimeInIsoFormat(m_nxroot->openFirstEntry().getString("start_time"));
}

/**
 * @brief LoadILLBase::resolveInstrument resolves the name of the instrument
 * Optionally, appends the variant, if overridden.
 * @return the name of fully resolved instrument as mantid knows it
 */
template <class W> std::string LoadILLBase<W>::resolveInstrument() {
  NXEntry firstEntry = m_nxroot->openFirstEntry();
  const std::string instrumentPath = m_helper->findInstrumentNexusPath(firstEntry);
  std::string instrumentName = m_helper->getStringFromNexusPath(firstEntry, instrumentPath + "/name");
  boost::to_upper(instrumentName);
  return instrumentName;
}

/**
 * @brief LoadILLBase::exec Entry point, final.
 */
template <class W> void LoadILLBase<W>::exec() {
  setup();
  wrapup(load());
}

} // namespace Mantid::DataHandling
