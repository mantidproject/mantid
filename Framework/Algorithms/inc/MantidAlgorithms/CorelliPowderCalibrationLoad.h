// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CorelliPowderCalibrationDatabase.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
 * @brief Load calibration table for CORELLI from given path
 *
 */
class MANTID_ALGORITHMS_DLL CorelliPowderCalibrationLoad
    : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override {
    return "CorelliPowderCalibrationLoad";
  };

  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Load corresponding Corelli calibration table from given path";
  };

  /// Algorithm's version, overriding a virtual method
  int version() const override { return 1; };

  /// Algorithm's category, overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\Calibration";
  };

  /// Extra help info
  const std::vector<std::string> seeAlso() const override {
    return {"CorelliPowderCalibrationGenerate "
            "& CorelliPowderCalibrationDatabase "
            "& CorelliPowderCalibrationApply"};
  };

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// Private validator for inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Get calibration filename based on given ws
  std::string deduce_calibration_filename(API::MatrixWorkspace_sptr ws);

  /// Members
  API::MatrixWorkspace_sptr ws;
};

} // namespace Algorithms

} // namespace Mantid