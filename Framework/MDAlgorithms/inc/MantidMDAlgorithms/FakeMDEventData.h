// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

/** FakeMDEventData : Algorithm to create fake multi-dimensional event
  data that gets added to MDEventWorkspace, for use in testing.
 *
 */
class MANTID_MDALGORITHMS_DLL FakeMDEventData : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "FakeMDEventData"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Adds fake multi-dimensional event data to an existing "
           "MDEventWorkspace, for use in testing.\nYou can create a blank "
           "MDEventWorkspace with CreateMDWorkspace.";
  }
  /// Algorithm's verion for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CreateMDWorkspace", "EvaluateMDFunction"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Creation"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid
