// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

#include <boost/tuple/tuple.hpp>

namespace Mantid {
namespace Algorithms {

/** Stitch1D : Stitches two Matrix Workspaces together into a single output.
 */
class MANTID_ALGORITHMS_DLL Stitch1D final : public API::Algorithm {
public:
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override { return "Stitch1D"; }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 3; }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Reflectometry"; }
  /// Summary of algorithm's purpose
  const std::string summary() const override { return "Stitches single histogram matrix workspaces together"; }
  const std::vector<std::string> seeAlso() const override { return {"Rebin", "Stitch1DMany"}; }
  /// Does the x-axis have non-zero errors
  bool hasNonzeroErrors(Mantid::API::MatrixWorkspace_sptr &ws);
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

  /// Helper typedef for storing special-value indexes per spectrum.
  using SpecialTypeIndexes = std::vector<std::vector<size_t>>;
  /// Indexes of Y and E values that were NaN or infinite before processing.
  struct SpecialValueIndexes {
    SpecialTypeIndexes nanE;
    SpecialTypeIndexes nanY;
    SpecialTypeIndexes infE;
    SpecialTypeIndexes infY;

    void resize(size_t histogramCount) {
      nanE.resize(histogramCount);
      nanY.resize(histogramCount);
      infE.resize(histogramCount);
      infY.resize(histogramCount);
    }
  };

  struct overlapBounds {
    int a1 = -1;
    int a2 = -1;
  };

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method.
  void exec() override;
  /// Get the start overlap
  double getStartOverlap(const double intesectionMin, const double intesectionMax) const;
  /// Get the end overlap
  double getEndOverlap(const double intesectionMin, const double intesectionMax) const;

  /// Get the rebin parameters
  std::vector<double> getRebinParams(Mantid::API::MatrixWorkspace_const_sptr &lhsWS,
                                     Mantid::API::MatrixWorkspace_const_sptr &rhsWS, const bool scaleRHS) const;
  /// Perform rebin and record special values that are zeroed
  Mantid::API::MatrixWorkspace_sptr rebin(Mantid::API::MatrixWorkspace_sptr &input, const std::vector<double> &params,
                                          SpecialValueIndexes &specialValues);
  /// Perform integration
  Mantid::API::MatrixWorkspace_sptr integration(Mantid::API::MatrixWorkspace_sptr const &input, const double start,
                                                const double stop);
  Mantid::API::MatrixWorkspace_sptr singleValueWS(const double val);
  /// Calculate the weighted mean
  Mantid::API::MatrixWorkspace_sptr weightedMean(Mantid::API::MatrixWorkspace_sptr const &inOne,
                                                 Mantid::API::MatrixWorkspace_sptr const &inTwo);
  /// Conjoin x axis
  Mantid::API::MatrixWorkspace_sptr conjoinXAxis(Mantid::API::MatrixWorkspace_sptr const &inOne,
                                                 Mantid::API::MatrixWorkspace_sptr const &inTwo);
  /// Find the start and end indexes
  boost::tuple<int, int> findStartEndIndexes(double startOverlap, double endOverlap,
                                             Mantid::API::MatrixWorkspace_sptr &workspace);
  /// Mask out everything but the data in the ranges
  Mantid::API::MatrixWorkspace_sptr maskAllBut(int a1, int a2, Mantid::API::MatrixWorkspace_sptr &source);
  /// Mask out everything but the data in the ranges, but do it inplace.
  void maskInPlace(int a1, int a2, Mantid::API::MatrixWorkspace_sptr &source);
  /// Add back special values only where no valid data contributes to the output
  void reinsertSpecialValues(const Mantid::API::MatrixWorkspace_sptr &ws, const SpecialValueIndexes &lhsSpecialValues,
                             const SpecialValueIndexes &rhsSpecialValues, overlapBounds bounds);
  /// Use valid overlap values where exactly one workspace has an invalid signal value
  void useValidOverlapData(const Mantid::API::MatrixWorkspace_sptr &overlap,
                           const Mantid::API::MatrixWorkspace_sptr &lhs, const Mantid::API::MatrixWorkspace_sptr &rhs,
                           const SpecialValueIndexes &lhsSpecialValues, const SpecialValueIndexes &rhsSpecialValues,
                           const overlapBounds bounds);
  /// Range tolerance
  static const double range_tolerance;
  /// Scaling factors
  double m_scaleFactor;
  double m_errorScaleFactor;
  bool m_useValidDataOnly;
  /// Scale workspace (left hand side or right hand side)
  void scaleWorkspace(API::MatrixWorkspace_sptr &ws, API::MatrixWorkspace_sptr &scaleFactorWS,
                      API::MatrixWorkspace_const_sptr &dxWS);
  /// Index per workspace spectra of special values
  SpecialValueIndexes m_lhsSpecialValues;
  SpecialValueIndexes m_rhsSpecialValues;
};

} // namespace Algorithms
} // namespace Mantid
