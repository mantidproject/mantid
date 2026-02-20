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
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/**
This is a parent algorithm that uses several different child algorithms to
perform it's task.
Takes a workspace as input and the filename of a grouping file of a suitable
format.

The input workspace is
1) Converted to d-spacing units
2) Rebinnned to a common set of bins
3) The spectra are grouped according to the grouping file.

    Required Properties:
<UL>
<LI> InputWorkspace - The name of the 2D Workspace to take as input </LI>
<LI> GroupingFileName - The path to a grouping file</LI>
<LI> OutputWorkspace - The name of the 2D workspace in which to store the result
</LI>
</UL>

The structure of the grouping file is as follows:
# Format: number  UDET offset  select  group
0        611  0.0000000  1    0
1        612  0.0000000  1    0
2        601  0.0000000  0    0
3        602  0.0000000  0    0
4        621  0.0000000  1    0


@author Nick Draper, Tessella
@date 11/07/2008
*/
class MANTID_ALGORITHMS_DLL DiffractionFocussing final : public API::Algorithm, public API::DeprecatedAlgorithm {
public:
  /// Constructor
  DiffractionFocussing();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "DiffractionFocussing"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Algorithm to focus powder diffraction data into a number of "
           "histograms according to a grouping scheme defined in a CalFile.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\Focussing"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  API::MatrixWorkspace_sptr convertUnitsToDSpacing(const API::MatrixWorkspace_sptr &workspace);
  void RebinWorkspace(API::MatrixWorkspace_sptr &workspace);
  void calculateRebinParams(const API::MatrixWorkspace_const_sptr &workspace, double &min, double &max, double &step);
  std::multimap<int64_t, int64_t> readGroupingFile(const std::string &groupingFileName);
};

} // namespace Algorithms
} // namespace Mantid
