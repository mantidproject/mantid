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
    const std::string filename =
        this->getPropertyValue(this->getFilenamePropertyName());
    m_fileInfo =
        std::make_shared<Mantid::Kernel::NexusHDF5Descriptor>(filename);
  }

  // execute the algorithm as normal
  execLoader();
}
std::shared_ptr<Mantid::API::Algorithm> NexusFileLoader::createChildAlgorithm(
    const std::string &name, const double startProgress,
    const double endProgress, const bool enableLogging, const int &version) {
  auto child = IFileLoader::createChildAlgorithm(
      name, startProgress, endProgress, enableLogging, version);

  // set the NexusHDF5Descriptor on the child algorithm
  auto nfl = std::dynamic_pointer_cast<NexusFileLoader>(child);
  if (nfl) {
    nfl->setFileInfo(m_fileInfo);
  }
  return child;
}
void NexusFileLoader::setFileInfo(
    std::shared_ptr<Mantid::Kernel::NexusHDF5Descriptor> fileInfo) {
  m_fileInfo = std::move(fileInfo);
}

std::string NexusFileLoader::getFilenamePropertyName() const {
  return "Filename";
}

//----------------------------------------------------------------------------------------------
/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int NexusFileLoader::confidence(Kernel::NexusHDF5Descriptor &descriptor) const {

  int confidence = 0;
  const std::map<std::string, std::set<std::string>> &allEntries =
      descriptor.getAllEntries();
  if (allEntries.count("NXevent_data") == 1) {
    if (descriptor.isClassEntry("NXentry", "/entry") ||
        descriptor.isClassEntry("NXentry", "/raw_data_1")) {
      confidence = 80;
    }
  }

  return confidence;
}

} // namespace Mantid::API
