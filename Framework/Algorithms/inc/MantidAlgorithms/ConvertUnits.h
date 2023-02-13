// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

/** Converts the units in which a workspace is represented.
    Note that if you are converting to or from units which are not meaningful
    for monitor detectors,
    then you should not expect the resulting spectrum to hold meaningful values.
 */
class MANTID_ALGORITHMS_DLL ConvertUnits : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ConvertUnits"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Performs a unit change on the X values of a workspace"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertAxisByFormula", "ConvertAxesToRealSpace", "ConvertSpectrumAxis", "ConvertToYSpace"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Units"; }

protected:
  /// Reverses the workspace if X values are in descending order
  void reverse(const API::MatrixWorkspace_sptr &WS);

  /// For conversions to energy transfer, removes bins corresponding to
  /// inaccessible values
  API::MatrixWorkspace_sptr removeUnphysicalBins(const API::MatrixWorkspace_const_sptr &workspace);

  const std::string workspaceMethodName() const override { return "convertUnits"; }
  const std::string workspaceMethodInputProperty() const override { return "InputWorkspace"; }

  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void setupMemberVariables(const API::MatrixWorkspace_const_sptr &inputWS);
  virtual void storeEModeOnWorkspace(API::MatrixWorkspace_sptr outputWS);
  API::MatrixWorkspace_sptr setupOutputWorkspace(const API::MatrixWorkspace_const_sptr &inputWS);

  /// Executes the main part of the algorithm that handles the conversion of the
  /// units
  API::MatrixWorkspace_sptr executeUnitConversion(const API::MatrixWorkspace_sptr &inputWS);

  /// Convert the workspace units according to a simple output = a * (input^b)
  /// relationship
  API::MatrixWorkspace_sptr convertQuickly(const API::MatrixWorkspace_const_sptr &inputWS, const double &factor,
                                           const double &power);

  /// Convert the workspace units using TOF as an intermediate step in the
  /// conversion
  virtual API::MatrixWorkspace_sptr convertViaTOF(Kernel::Unit_const_sptr fromUnit,
                                                  API::MatrixWorkspace_const_sptr inputWS);

  // Calls Rebin as a Child Algorithm to align the bins of the output workspace
  API::MatrixWorkspace_sptr alignBins(const API::MatrixWorkspace_sptr &workspace);
  const std::vector<double> calculateRebinParams(const API::MatrixWorkspace_const_sptr &workspace) const;

  void putBackBinWidth(const API::MatrixWorkspace_sptr &outputWS);

  std::size_t m_numberOfSpectra{0}; ///< The number of spectra in the input workspace
  bool m_distribution{false};       ///< Whether input is a distribution. Only applies
  /// to histogram workspaces.
  bool m_inputEvents{false};           ///< Flag indicating whether input workspace is an EventWorkspace
  Kernel::Unit_const_sptr m_inputUnit; ///< The unit of the input workspace
  Kernel::Unit_sptr m_outputUnit;      ///< The unit we're going to
};

} // namespace Algorithms
} // namespace Mantid
