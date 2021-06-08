// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

// Forward declarations
namespace mu {
class Parser;
} // namespace mu

namespace Mantid {

namespace HistogramData {
class Histogram;
class HistogramE;
class HistogramY;
class Points;
} // namespace HistogramData

namespace Algorithms {

/** DetectorEfficiencyCorUser :

 This algorithm will calculate the detector efficiency according to the ILL INX
 program for time-of-flight
 data reduction.

 Formula_eff must be defined in the instrument parameters file.
 */
class MANTID_ALGORITHMS_DLL DetectorEfficiencyCorUser : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Corrects for detector efficiency. The correction factor is "
           "calculated using an instrument specific formula as a function "
           "of the final neutron energy E_f=E_i-E. Note that the formula "
           "is implemented only for a limited number of TOF instruments.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"DetectorEfficiencyCor"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void retrieveProperties();
  void correctHistogram(const size_t index, const double eff0, double &e, mu::Parser &parser);

  double evaluate(const mu::Parser &parser) const;

  mu::Parser generateParser(const std::string &formula, double *e) const;

  std::string retrieveFormula(const size_t workspaceIndex);

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;
  /// stores the user selected value for incidient energy of the neutrons
  double m_Ei = 0.0;
};

} // namespace Algorithms
} // namespace Mantid
