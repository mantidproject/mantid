// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadILLMock.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/PropertyManager.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid::DataHandling {

// For manual testing, uncomment this to register the algorithm as a loader
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLMock)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadILLMock::name() const { return "LoadILLMock"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLMock::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLMock::category() const { return "Test"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLMock::summary() const { return "This is a fake algorithm for testing purposes ONLY."; }

/// Algorithms confidence identification. @see Algorithm::confidence
int LoadILLMock::confidence(Kernel::NexusDescriptor &descriptor) const {
  // Modify here if you want to test also the confidence
  UNUSED_ARG(descriptor)
  return 0;
}

std::vector<std::string> LoadILLMock::mandatoryKeys() { return std::vector<std::string>({"/entry0/monitor1/monrate"}); }

Workspace_sptr LoadILLMock::buildWorkspace() {
  return std::dynamic_pointer_cast<Workspace>(WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1));
}

void LoadILLMock::loadAndFillData() {
  float monrate = getScalarMetadata<float>("/entry0/monitor1/monrate");
  // PropertyManager_sptr pm = getProperty("PatchNexusMetadataEntries");
  // float a = pm->getProperty("/entry0/monitor1/monrate");
  g_log.warning() << monrate << std::endl;
};

void LoadILLMock::configureBeamline(){};

} // namespace Mantid::DataHandling
