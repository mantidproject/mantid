// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CorelliCalibrationDatabase.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace Algorithms {
/**
 * @brief Apply calibration table for Corelli Powder Diffraction
 *
 */
class MANTID_ALGORITHMS_DLL CorelliCalibrationApply : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CorelliCalibrationApply"; };

  /// Summary of algorithm's purpose
  const std::string summary() const override { return "Apply Corelli calibration results onto input workspace"; };

  /// Algorithm's version, overriding a virtual method
  int version() const override { return 1; };

  /// Algorithm's category, overriding a virtual method
  const std::string category() const override { return "Diffraction\\Calibration"; };

  /// Extra help info
  const std::vector<std::string> seeAlso() const override {
    return {"CorelliPowderCalibrationGenerate", "CorelliCalibrationDatabase", "CorelliPowderCalibrationLoad"};
  };

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// Private validator for inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Members
  API::MatrixWorkspace_sptr ws;
  DataObjects::TableWorkspace_sptr calTable;
};

} // namespace Algorithms

} // namespace Mantid
