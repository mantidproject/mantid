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

void LoadILLBase::setOutputWorkspace() { setProperty<API::Workspace_sptr>("OutputWorkspace", m_workspace); }

std::string LoadILLBase::getInstrumentDefinitionFilePath() {
  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  Poco::Path file(m_instrument + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

void LoadILLBase::bootstrap() {
  const std::string filename = getPropertyValue("Filename");
  PropertyManager_sptr pmp = getProperty("PatchNexusMetadataEntries");
  m_nxroot = std::make_unique<NXRoot>(filename);
  m_nep = std::make_unique<NexusEntryProvider>(filename, *pmp);
  m_helper = std::make_unique<LoadHelper>();
  m_mode = resolveAcqMode();
  m_instrument = resolveInstrument();
  validateMetadata();
  m_workspace = buildWorkspace();
  resolveStartTime();
  loadInstrument();
}

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

void LoadILLBase::loadInstrument() {
  const std::string idf = getInstrumentDefinitionFilePath();
  auto loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("Filename", idf);
  loadInst->setProperty("Workspace", m_workspace);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(true));
  loadInst->execute();
}

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

std::string LoadILLBase::resolveInstrument() {
  NXEntry firstEntry = m_nxroot->openFirstEntry();
  const std::string instrumentPath = m_helper->findInstrumentNexusPath(firstEntry);
  std::string instrumentName = m_helper->getStringFromNexusPath(firstEntry, instrumentPath + "/name");
  boost::to_upper(instrumentName);
  return instrumentName + resolveVariant();
}

void LoadILLBase::exec() {
  bootstrap();
  configureBeamline();
  loadAndFillData();
  addSampleLogs();
  patchSampleLogs();
  setOutputWorkspace();
}

} // namespace Mantid::DataHandling
