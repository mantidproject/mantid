// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/NexusHDF5Descriptor.h"
#include "MantidKernel/System.h"

namespace Mantid::DataHandling {

class DLLExport NexusFileLoader
    : public API::IFileLoader<Mantid::Kernel::NexusHDF5Descriptor> {
public:
  void
  exec() override final; // makes sure the NexusHDF5Descriptor is initialized
  virtual void execLoader() = 0; // what would normally be called exec
  // the name of the property that the NexusHDF5Descriptor should be created against
  virtual std::string getFilenamePropertyName() const = 0;
  boost::shared_ptr<Algorithm> createChildAlgorithm(
      const std::string &name, const double startProgress = -1.,
      const double endProgress = -1., const bool enableLogging = true,
      const int &version = -1) override;
  virtual void
  setFileInfo(std::shared_ptr<Mantid::Kernel::NexusHDF5Descriptor> fileInfo);

private:
  std::shared_ptr<Mantid::Kernel::NexusHDF5Descriptor> m_fileInfo;
};
} // namespace Mantid::DataHandling
