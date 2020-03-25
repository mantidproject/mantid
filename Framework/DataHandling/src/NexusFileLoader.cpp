// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/NexusFileLoader.h"

namespace Mantid::DataHandling {
void NexusFileLoader::exec() { execLoader(); }
boost::shared_ptr<Mantid::API::Algorithm> NexusFileLoader::createChildAlgorithm(
    const std::string &name, const double startProgress,
    const double endProgress, const bool enableLogging, const int &version) {
  auto child = API::IFileLoader<
      Mantid::Kernel::NexusHDF5Descriptor>::createChildAlgorithm(name,
                                                                 startProgress,
                                                                 endProgress,
                                                                 enableLogging,
                                                                 version);
  auto nfl = boost::dynamic_pointer_cast<NexusFileLoader>(child);
  if (nfl) {
    nfl->setFileInfo(m_fileInfo);
  }
  return child;
}
void NexusFileLoader::setFileInfo(
    std::shared_ptr<Mantid::Kernel::NexusHDF5Descriptor> fileInfo) {
  m_fileInfo = std::move(fileInfo);
}
} // namespace Mantid::DataHandling
