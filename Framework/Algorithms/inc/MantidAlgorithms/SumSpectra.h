// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include <set>

namespace Mantid {

namespace API { // forward declare
class ISpectrum;
} // namespace API

namespace Algorithms {

/** Takes a workspace as input and sums all of the spectra within it maintaining
   the existing bin structure and units.
    The result is stored as a new workspace containing a single spectra.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> StartSpectrum - Workspace index number to integrate from (default
   0)</LI>
    <LI> EndSpectrum - Workspace index number to integrate to (default max)</LI>
    <LI> IncludeMonitors - Whether to include monitor spectra in the sum
   (default yes)
    </UL>

    @author Nick Draper, Tessella Support Services plc
    @date 22/01/2009
 */
class MANTID_ALGORITHMS_DLL SumSpectra : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SumSpectra"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The SumSpectra algorithm adds the data values in each time bin "
           "across a range of spectra; the output workspace has a single "
           "spectrum. If the input is an EventWorkspace, the output is also an "
           "EventWorkspace; otherwise it will be a Workspace2D.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SumNeighbours"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Grouping"; }
  /// Cross-input validation
  std::map<std::string, std::string> validateInputs() override;

private:
  struct WorkspaceLikeVector : public std::vector<std::pair<std::vector<double>, std::vector<double>>> {
    WorkspaceLikeVector(size_t yL, size_t numspec)
        : std::vector<std::pair<std::vector<double>, std::vector<double>>>(yL) {
      for (size_t j = 0; j < yL; j++) {
        this->operator[](j).first.reserve(numspec);
        this->operator[](j).second.reserve(numspec);
      }
      m_specNums.reserve(numspec);
    }
    std::vector<size_t> const &specNums() const { return m_specNums; }
    std::vector<double> const &y(size_t j) const { return this->operator[](j).first; }
    std::vector<double> const &e(size_t j) const { return this->operator[](j).second; }
    void insertSpecNum(size_t specNum) { m_specNums.push_back(specNum); }
    void insertY(size_t j, double const y) { this->operator[](j).first.push_back(y); }
    void insertE(size_t j, double const e) { this->operator[](j).second.push_back(e); }

  private:
    std::vector<size_t> m_specNums;
  };

  /// Handle logic for RebinnedOutput workspaces
  void doFractionalSum(API::MatrixWorkspace_sptr const &, WorkspaceLikeVector const &, API::Progress &, size_t &);
  void doFractionalWeightedSum(API::MatrixWorkspace_sptr const &, WorkspaceLikeVector const &, API::Progress &,
                               size_t &);
  /// Handle logic for summing standard workspaces
  void doSimpleSum(API::ISpectrum &, WorkspaceLikeVector const &, API::Progress &, size_t &);
  void doSimpleWeightedSum(API::ISpectrum &, WorkspaceLikeVector const &, API::Progress &, size_t &);

  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void execEvent(const API::MatrixWorkspace_sptr &outputWorkspace, API::Progress &progress, size_t &numSpectra,
                 size_t &numMasked, size_t &numZeros);
  specnum_t getOutputSpecNo(const API::MatrixWorkspace_const_sptr &localworkspace);

  API::MatrixWorkspace_sptr replaceSpecialValues();
  void determineIndices(const size_t numberOfSpectra);

  /// The output spectrum number
  specnum_t m_outSpecNum{0};
  /// Set true to keep monitors
  bool m_keepMonitors{false};
  /// Set true to remove special values before processing
  bool m_replaceSpecialValues{false};
  /// numberOfSpectra in the input
  size_t m_numberOfSpectra{0};
  /// Blocksize of the input workspace
  size_t m_yLength{0};
  /// Set of indices to sum
  std::set<size_t> m_indices;

  // if calculating additional workspace with specially weighted averages is
  // necessary
  bool m_calculateWeightedSum{false};
  bool m_multiplyByNumSpec{true};
};

} // namespace Algorithms
} // namespace Mantid
