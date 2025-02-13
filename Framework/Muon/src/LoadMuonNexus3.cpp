// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/LoadMuonNexus3.h"

#include "MantidAPI/NexusFileLoader.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataHandling/LoadMuonNexusV2.h"
#include "MantidKernel/Logger.h"
#include "MantidMuon/LoadMuonNexus1.h"
#include "MantidMuon/LoadMuonNexus2.h"
#include "MantidNexus/NexusClasses.h"
#include <H5Cpp.h>
#include <string>

namespace {
const int CONFIDENCE_THRESHOLD{80};

int calculateConfidenceHDF5(const std::string &filePath, const std::shared_ptr<Mantid::API::Algorithm> &alg) {
  auto nexusLoader = std::dynamic_pointer_cast<Mantid::API::NexusFileLoader>(alg);
  int confidence{0};
  if (H5::H5File::isHdf5(filePath)) {
    try {
      Mantid::Kernel::NexusHDF5Descriptor descriptorHDF5(filePath);
      confidence = nexusLoader->confidence(descriptorHDF5);
    } catch (std::exception const &e) {
      Mantid::Kernel::Logger("LoadMuonNexus3").debug()
          << "Error in calculating confidence for: " << nexusLoader->name() << " " << e.what() << '\n';
    }
  }
  return (confidence >= CONFIDENCE_THRESHOLD) ? confidence : 0;
}

int calculateConfidence(const std::string &filePath, const std::shared_ptr<Mantid::API::Algorithm> &alg) {
  auto fileLoader = std::dynamic_pointer_cast<Mantid::API::IFileLoader<Mantid::Kernel::NexusDescriptor>>(alg);
  Mantid::Kernel::NexusDescriptor descriptor(filePath);
  const int confidence = fileLoader->confidence(descriptor);
  return (confidence >= CONFIDENCE_THRESHOLD) ? confidence : 0;
}
} // namespace

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMuonNexus3)

LoadMuonNexus3::LoadMuonNexus3() : LoadMuonNexus() {}

/** Executes the right version of the Muon nexus loader
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */

void LoadMuonNexus3::exec() {
  std::string filePath = getPropertyValue("Filename");
  m_loadAlgs.emplace(std::make_shared<Mantid::DataHandling::LoadMuonNexusV2>(), &calculateConfidenceHDF5);
  m_loadAlgs.emplace(std::make_shared<LoadMuonNexus1>(), &calculateConfidence);
  m_loadAlgs.emplace(std::make_shared<LoadMuonNexus2>(), &calculateConfidence);

  int maxConfidence{0};
  int confidence{0};
  int version{0};
  std::string algName;
  for (const auto &alg : m_loadAlgs) {
    confidence = alg.second(filePath, alg.first);
    if (confidence > maxConfidence) {
      maxConfidence = confidence;
      algName = alg.first->name();
      version = alg.first->version();
    }
  }

  if (!maxConfidence) {
    throw Kernel::Exception::FileError("Cannot open the file ", filePath);
  }

  runSelectedAlg(algName, version);
}

void LoadMuonNexus3::runSelectedAlg(const std::string &algName, const int version) {
  auto childAlg = createChildAlgorithm(algName, 0, 1, true, version);
  auto loader = std::dynamic_pointer_cast<API::Algorithm>(childAlg);
  loader->copyPropertiesFrom(*this);
  loader->executeAsChildAlg();
  this->copyPropertiesFrom(*loader);
  API::Workspace_sptr outWS = loader->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outWS);
}
} // namespace Mantid::Algorithms
