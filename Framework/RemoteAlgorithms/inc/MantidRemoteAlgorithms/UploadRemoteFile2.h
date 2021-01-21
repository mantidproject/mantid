// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
namespace Mantid {
namespace RemoteAlgorithms {

/**
Transer/upload a file to a (remote) compute resource
*/
class DLLExport UploadRemoteFile2 : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "UploadRemoteFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Transfers/uploads a file to the specified compute resource."; }

  /// Algorithm's version
  int version() const override { return (2); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Remote"; }

private:
  void init() override;
  /// Execution code
  void exec() override;
};

} // end namespace RemoteAlgorithms
} // end namespace Mantid
