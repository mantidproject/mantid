// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/LoadMuonNexus3.h"

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/NexusFileLoader.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/Logger.h"
#include "MantidNexus/NexusClasses.h"
#include <H5Cpp.h>
#include <string>

namespace {
const int CONFIDENCE_THRESHOLD{80};

int calculateConfidenceHDF5(const std::string &filePath, const std::shared_ptr<Mantid::API::Algorithm> &alg) {
  const auto nexusLoader = std::dynamic_pointer_cast<Mantid::API::NexusFileLoader>(alg);
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
  const auto fileLoader = std::dynamic_pointer_cast<Mantid::API::IFileLoader<Mantid::Kernel::NexusDescriptor>>(alg);
  Mantid::Kernel::NexusDescriptor descriptor(filePath);
  const int confidence = fileLoader->confidence(descriptor);
  return (confidence >= CONFIDENCE_THRESHOLD) ? confidence : 0;
}
} // namespace

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMuonNexus3)

/** Executes the right version of the Muon nexus loader
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */
LoadMuonNexus3::LoadMuonNexus3() : LoadMuonNexus() {
  addAlgToVec("LoadMuonNexusV2", 1, &calculateConfidenceHDF5);
  addAlgToVec("LoadMuonNexus", 1, &calculateConfidence);
  addAlgToVec("LoadMuonNexus", 2, &calculateConfidence);
}

void LoadMuonNexus3::exec() {
  const std::string filePath = getPropertyValue("Filename");

  int maxConfidenceRes{0};
  for (int i = 0; i < m_loadAlgs.size(); i++) {
    const int confidenceRes = m_loadAlgs[i].m_confFunc(filePath, m_loadAlgs[i].m_alg);
    if (confidenceRes > maxConfidenceRes) {
      maxConfidenceRes = confidenceRes;
      m_selectedIndex = i;
    }
  }

  if (!maxConfidenceRes) {
    throw Kernel::Exception::FileError("Cannot open the file ", filePath);
  }

  runSelectedAlg();
}

void LoadMuonNexus3::runSelectedAlg() {
  const auto &alg = m_loadAlgs[m_selectedIndex].m_alg;
  this->setupAsChildAlgorithm(alg, 0, 1, true);
  alg->copyPropertiesFrom(*this);
  alg->executeAsChildAlg();
  this->copyPropertiesFrom(*alg);
}

void LoadMuonNexus3::addAlgToVec(const std::string &name, const int version, const ConfFuncPtr &loader) {
  const auto &factory = API::AlgorithmFactory::Instance();
  const auto alg = factory.create(name, version);
  m_loadAlgs.push_back(AlgDetail(name, version, loader, alg));
}
} // namespace Mantid::Algorithms
