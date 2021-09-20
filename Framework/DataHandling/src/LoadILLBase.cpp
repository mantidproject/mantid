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
#include "MantidDataHandling/NexusEntryProvider.h"
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
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
  declareProperty(std::make_unique<PropertyManagerProperty>("PatchNexusMetadataEntries", Direction::Input),
                  "JSON formatted key-value pairs to add/override nexus entries.");
  declareProperty(std::make_unique<PropertyManagerProperty>("PatchWorkspaceSampleLogs", Direction::Input),
                  "JSON formatted key-value pairs to add/override sample logs.");
  declareExtraProperties();
}

std::string LoadILLBase::getInstrumentDefinitionFilePath() {
  Poco::Path directory(ConfigService::Instance().getInstrumentDirectory());
  Poco::Path file(m_instrumentName + "_Definition.xml");
  Poco::Path fullPath(directory, file);
  return fullPath.toString();
}

void LoadILLBase::bootstrap() {
  const std::string filename = getPropertyValue("Filename");
  m_nxroot = std::make_unique<NXRoot>(filename);
  PropertyManager_sptr pmp = getProperty("PatchNexusMetadataEntries");
  m_nep = std::make_unique<NexusEntryProvider>(filename, *pmp);
  m_helper = std::make_unique<LoadHelper>();
  m_acqMode = resolveAcqMode();
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
  loadInst->setProperty("RewriteSpectraMap", true);
  loadInst->execute();
}

void LoadILLBase::resolveStartTime() {}

void LoadILLBase::resolveInstrument() {
  NXEntry firstEntry = m_nxroot->openFirstEntry();
  const std::string instrumentPath = m_helper->findInstrumentNexusPath(firstEntry);
  m_instrumentName = m_helper->getStringFromNexusPath(firstEntry, instrumentPath + "/name");
  boost::to_upper(m_instrumentName);
  m_instrumentName += resolveVariant();
}

void LoadILLBase::exec() {
  bootstrap();
  validateMetadata();
  resolveInstrument();
  buildWorkspace();
  resolveStartTime();
  loadInstrument();
  configureBeamline();
  loadAndFillData();
  addSampleLogs();
  patchSampleLogs();
  setOutputWorkspace();
}

} // namespace Mantid::DataHandling
