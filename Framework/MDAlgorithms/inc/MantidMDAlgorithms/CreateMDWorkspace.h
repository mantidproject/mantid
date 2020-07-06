// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/BoxControllerSettingsAlgorithm.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

std::vector<std::string>
    MANTID_MDALGORITHMS_DLL parseNames(const std::string &names_string);

/** CreateMDWorkspace :
 *
 * Algorithm to create an empty MDEventWorkspace with a given number of
 *dimensions.
 *
 *
 * @author Janik Zikovsky
 * @date 2011-02-25 11:54:52.003137
 */
class DLLExport CreateMDWorkspace : public API::BoxControllerSettingsAlgorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CreateMDWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates an empty MDEventWorkspace with a given number of "
           "dimensions.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToMD", "CreateMDHistoWorkspace", "FakeMDEventData",
            "CreateMD"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Creation";
  }
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  template <typename MDE, size_t nd>
  void finish(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);
  Mantid::Geometry::MDFrame_uptr createMDFrame(const std::string &frame,
                                               const std::string &unit);
  bool checkIfFrameValid(const std::string &frame,
                         const std::vector<std::string> &targetFrames);
};

} // namespace MDAlgorithms
} // namespace Mantid
