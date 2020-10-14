// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/BoxControllerSettingsAlgorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace MDAlgorithms {

/** BaseConvertToDiffractionMDWorkspace
 *
 * Base class for common code shared between different versions of the
 * ConvertToDiffractionMDWorkspace algorithm.
 */
class DLLExport BaseConvertToDiffractionMDWorkspace
    : public API::BoxControllerSettingsAlgorithm {

public:
  /// Algorithm's name for identification
  const std::string name() const override {
    return "ConvertToDiffractionMDWorkspace";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, "
           "Qz) for an elastic diffraction experiment.";
  }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Creation";
  }

private:
  void exec() override;

protected: // for testing
  void init() override;

  // the target frame names exposed as the algorithm properties and recognized
  // by old convertToDiffractionWorkspace algorithm.
  std::vector<std::string> frameOptions;

  // method to convert the value of the target frame specified for the
  // ConvertToDiffractionMDWorksapce  into the properties names of the
  // ConvertToMD
  void convertFramePropertyNames(const std::string &TargFrame,
                                 std::string &TargFrameName,
                                 std::string &ScalingName);
  // method to convert the extents specified for the
  // ConvertToDiffractionMDWorksapce  into the min-max properties names of the
  // ConvertToMD
  virtual void convertExtents(const std::vector<double> &Extents,
                              std::vector<double> &minVal,
                              std::vector<double> &maxVal) = 0;
};

} // namespace MDAlgorithms
} // namespace Mantid
