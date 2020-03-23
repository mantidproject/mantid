// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/NexusFileLoader.h"

namespace Mantid::DataHandling {
  void NexusFileLoader::exec() {}
  void createChildAlgorithm(const std::string &name,
                            const double startProgress,
                            const double endProgress,
                            const bool enableLogging,
                            const int &version) {}
  void setFileInfo(std::shared_ptr<Mantid::Nexus::HDF5Descriptor> fileInfo)
	 {}
}
