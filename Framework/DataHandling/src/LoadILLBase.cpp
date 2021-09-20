// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadILLBase.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/NexusEntryProvider.h"
#include "MantidKernel/PropertyManagerProperty.h"

#include <Poco/Path.h>

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
}

void LoadILLBase::addSampleLogs() {}

void LoadILLBase::patchSampleLogs() {}

void LoadILLBase::loadInstrument() {}

void LoadILLBase::resolveStartTime() {}

void LoadILLBase::resolveInstrument() {}

void LoadILLBase::exec() {
  bootstrap();
  validateMetadata();
  resolveInstrument();
  resolveVariant();
  resolveAcqMode();
  buildWorkspace();
  loadAndfillData();
  resolveStartTime();
  loadInstrument();
  placeInstrument();
  addSampleLogs();
  patchSampleLogs();
  setOutputWorkspace();
}

} // namespace Mantid::DataHandling
