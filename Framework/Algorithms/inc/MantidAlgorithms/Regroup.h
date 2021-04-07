// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace HistogramData {
class HistogramX;
class HistogramY;
class HistogramE;
} // namespace HistogramData
namespace Algorithms {
/** Takes a 2D workspace as input and regroups the data according to the input
   regroup parameters.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> RegroupParameters -  </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the
   result </LI>
    </UL>

    @author Roman Tolchenov
    @date 16/07/2008
 */
class MANTID_ALGORITHMS_DLL Regroup : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "Regroup"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Regroups data with new bin boundaries."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Rebin"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Rebin"; }

  int newAxis(const std::vector<double> &params, const std::vector<double> &xold, std::vector<double> &xnew,
              std::vector<int> &xoldIndex);

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void rebin(const HistogramData::HistogramX &xold, const HistogramData::HistogramY &yold,
             const HistogramData::HistogramE &eold, std::vector<int> &xoldIndex, HistogramData::HistogramY &ynew,
             HistogramData::HistogramE &enew, bool distribution);
};

} // namespace Algorithms
} // namespace Mantid
