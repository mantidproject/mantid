// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/NexusFileLoader.h"
#include "MantidDataHandling/Load.h"

namespace Mantid::DataHandling {
void NexusFileLoader::exec() {
  // make sure the descriptor is initialized
  if (!m_fileInfo) {
    const std::string filePropName = Load::findFilenamePropertyName(this);
    const std::string filename = this->getPropertyValue(filePropName);
    m_fileInfo =
        std::make_shared<Mantid::Kernel::NexusHDF5Descriptor>(filename);
  }

  // execute the algorithm as normal
  execLoader();
}
boost::shared_ptr<Mantid::API::Algorithm> NexusFileLoader::createChildAlgorithm(
    const std::string &name, const double startProgress,
    const double endProgress, const bool enableLogging, const int &version) {
  auto child = IFileLoader::createChildAlgorithm(
      name, startProgress, endProgress, enableLogging, version);

  // set the NexusHDF5Descriptor on the child algorithm
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
