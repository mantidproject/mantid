// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/NexusFileLoader.h"

namespace Mantid::API {
void NexusFileLoader::exec() {
  // make sure the descriptor is initialized
  if (!m_fileInfo) {
    const std::string filename = this->getPropertyValue(this->getFilenamePropertyName());
    m_fileInfo = std::make_shared<Mantid::Kernel::NexusDescriptor>(filename);
  }

  // execute the algorithm as normal
  execLoader();
}
std::shared_ptr<Mantid::API::Algorithm>
NexusFileLoader::createChildAlgorithm(const std::string &name, const double startProgress, const double endProgress,
                                      const bool enableLogging, const int &version) {
  auto child = IFileLoader::createChildAlgorithm(name, startProgress, endProgress, enableLogging, version);

  // set the NexusDescriptor on the child algorithm
  auto nfl = std::dynamic_pointer_cast<NexusFileLoader>(child);
  if (nfl) {
    nfl->setFileInfo(m_fileInfo);
  }
  return child;
}
void NexusFileLoader::setFileInfo(std::shared_ptr<Mantid::Kernel::NexusDescriptor> fileInfo) {
  m_fileInfo = std::move(fileInfo);
}

const std::shared_ptr<Mantid::Kernel::NexusDescriptor> NexusFileLoader::getFileInfo() const noexcept {
  return m_fileInfo;
}

std::string NexusFileLoader::getFilenamePropertyName() const { return "Filename"; }

} // namespace Mantid::API
