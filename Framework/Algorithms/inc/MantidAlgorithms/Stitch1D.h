// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_STITCH1D_H_
#define MANTID_ALGORITHMS_STITCH1D_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <boost/tuple/tuple.hpp>
#include <functional>
#include <map>
#include <string>

namespace Mantid {
namespace Algorithms {

/** Stitch1D : Stitches two Matrix Workspaces together into a single output.
 */
class DLLExport Stitch1D : public API::Algorithm {
public:
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override { return "Stitch1D"; }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 3; }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Reflectometry"; }
  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Stitches single histogram matrix workspaces together";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"Rebin", "Stitch1DMany"};
  }
  /// Does the x-axis have non-zero errors
  bool hasNonzeroErrors(Mantid::API::MatrixWorkspace_sptr &ws);
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Helper typedef. For storing indexes of special values per spectra per
  /// workspace.
  using SpecialTypeIndexes = std::vector<std::vector<size_t>>;
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method.
  void exec() override;

  /// Get the rebin parameters
  std::vector<double>
  getRebinParams(Mantid::API::MatrixWorkspace_const_sptr &lhsWS,
                 Mantid::API::MatrixWorkspace_const_sptr &rhsWS,
                 const bool scaleRHS) const;
  void replaceSpecialValues(API::MatrixWorkspace_sptr &input);
  /// Perform rebin
  void rebin(Mantid::API::MatrixWorkspace_sptr &ws,
             const std::vector<double> &params);
  /// Perform integration
  Mantid::API::MatrixWorkspace_sptr
  integration(Mantid::API::MatrixWorkspace_sptr &input, const double start,
              const double stop);
  Mantid::API::MatrixWorkspace_sptr singleValueWS(const double val);
  /// Calculate the weighted mean
  Mantid::API::MatrixWorkspace_sptr
  weightedMean(Mantid::API::MatrixWorkspace_sptr &inOne,
               Mantid::API::MatrixWorkspace_sptr &inTwo);
  /// Conjoin x axis
  Mantid::API::MatrixWorkspace_sptr
  conjoinXAxis(Mantid::API::MatrixWorkspace_sptr &inOne,
               Mantid::API::MatrixWorkspace_sptr &inTwo);
  /// Find the start and end indexes
  boost::tuple<int, int>
  findStartEndIndexes(double startOverlap, double endOverlap,
                      API::MatrixWorkspace_sptr &workspace);
  /// Mask out everything but the data in the ranges
  Mantid::API::MatrixWorkspace_sptr
  maskAllBut(size_t a1, size_t a2, API::MatrixWorkspace_sptr &source);
  /// Mask out everything but the data in the ranges, but do it inplace.
  void maskInPlace(size_t a1, size_t a2,
                   Mantid::API::MatrixWorkspace_sptr &source);
  /// Add back in any special values
  void reinsertSpecialValues(Mantid::API::MatrixWorkspace_sptr ws);
  /// Scaling factors
  double m_scaleFactor;
  double m_errorScaleFactor;
  /// Scale workspace (left hand side or right hand side)
  void scaleWorkspace(API::MatrixWorkspace_sptr &ws,
                      API::MatrixWorkspace_sptr &scaleFactorWS,
                      API::MatrixWorkspace_const_sptr &dxWS);
  /// Index per workspace spectra of Nans
  SpecialTypeIndexes m_nanEIndexes;
  SpecialTypeIndexes m_nanYIndexes;
  SpecialTypeIndexes m_nanDxIndexes;
  /// Index per workspace spectra of Infs
  SpecialTypeIndexes m_infEIndexes;
  SpecialTypeIndexes m_infYIndexes;
  SpecialTypeIndexes m_infDxIndexes;

  /**
   * @brief A template which returns a boundary value
   * @param overlapVal A reference value intersection min or max
   * @param name A property name StartOverlap or EndOverlap
   * @param compare std::less<double> or std::greater<double>
   */
  template <typename Comparator>
  double overlap(const double overlapVal, const std::string &name,
                 Comparator const &compare) const {
    // StartOverlap and EndOverlap will be considered separately
    // For binned data, input validation ensures that final StartOverlap < final
    // EndOverlap. For point data this is not the case
    double interSectionVal = this->getProperty(name);
    if (isDefault(name))
      interSectionVal = overlapVal;
    if (compare(overlapVal, interSectionVal)) {
      g_log.warning(name + " outside range, will be " +
                    std::to_string(overlapVal));
      interSectionVal = overlapVal;
    } else {
      if (name == ("StartOverlap"))
        interSectionVal -= 1.e-9;
      else
        interSectionVal += 1.e-9;
    }
    g_log.information(name + ": " + std::to_string(interSectionVal));
    return interSectionVal;
  }
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_STITCH1D_H_ */
