// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/DllConfig.h"
#include "MantidNexus/NexusDescriptor.h"

namespace Mantid::API {

class MANTID_API_DLL NexusFileLoader : public API::IFileLoader<Mantid::Nexus::NexusDescriptor> {
public:
  void exec() override final;    // makes sure the NexusDescriptor is initialized
  virtual void execLoader() = 0; // what would normally be called exec
  // the name of the property that the NexusDescriptor should be created
  // against

  virtual std::string getFilenamePropertyName() const;
  std::shared_ptr<Algorithm> createChildAlgorithm(const std::string &name, const double startProgress = -1.,
                                                  const double endProgress = -1., const bool enableLogging = true,
                                                  const int &version = -1) override;
  virtual void setFileInfo(std::shared_ptr<Mantid::Nexus::NexusDescriptor> fileInfo);

  virtual int confidence(Nexus::NexusDescriptor &descriptor) const override = 0;

private:
  /// Required to pass m_fileInfo to static functions
  /// Keeping it shared_ptr to match setFileInfo signature (although passing
  /// ownership is not the main goal).
  /// AVOID USING THIS.  Use methods in Nexus::File instead
  virtual const std::shared_ptr<Mantid::Nexus::NexusDescriptor> getFileInfo() const noexcept { return m_fileInfo; };

  std::shared_ptr<Mantid::Nexus::NexusDescriptor> m_fileInfo;
};
} // namespace Mantid::API
