// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/NexusFileLoader.h"

namespace Mantid::DataHandling {
  void NexusFileLoader::exec() {}
  boost::shared_ptr<Mantid::API::Algorithm>
  NexusFileLoader::createChildAlgorithm(const std::string &name,
                                        const double startProgress,
                                        const double endProgress,
                                        const bool enableLogging,
                                        const int &version) {
    return nullptr;
  }
  void NexusFileLoader::setFileInfo(
      std::shared_ptr<Mantid::NeXus::NexusHDF5Descriptor> fileInfo) {
    m_fileInfo = std::move(fileInfo);
  }
}
