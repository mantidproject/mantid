// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"


namespace Mantid {
namespace Algorithms {

namespace CorelliCalibration {
static const std::vector<std::string> calibrationTableColumnNames {"ComponentName", "Xposition", "Yposition",
                                                         "Zposition", "XdirectionCosine", "YdirectionCosine",
                                                         "ZdirectionCosine", "RotationAngle"};
static const std::vector<std::string> calibrationTableColumnTypes {"str", "double", "double",
                                                         "double", "double", "double",
                                                         "double", "double"};
}

/** CorelliPowderCalibrationDatabase: blablabla TODO
 */
class MANTID_ALGORITHMS_DLL CorelliPowderCalibrationDatabase: public API::Algorithm {
public:
  const std::string name() const override { return "CorelliPowderCalibrationDatabase"; };
  int version() const override { return 1; };
  const std::string category() const override {
    return "Diffraction\\Calibration;Events";
  };
  const std::string summary() const override {
    return ".... ....";  // TODO
  };

private:
  std::map<std::string, std::string> validateInputs() override;
  void init() override;
  void exec() override;

  /// Input workspace
  API::MatrixWorkspace_const_sptr inputWS;
  DataObjects::TableWorkspace_const_sptr inputCalibrationTableWS;
  DataObjects::TableWorkspace_sptr outputWS;

};

} // namespace Algorithms
} // namespace Mantid
