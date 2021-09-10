// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** This algorithm is the equivalent of the COLETTE "DISPLAY H/V" command.
    It firsts integrates the input workspace, which must contain all the spectra
   from
    the detector of interest - no more and no less (so 128x128 or 192x192),
    between the X values given. Then each row or column is summed between the
   HOverVMin/Max
    values, if given, and the result is a single spectrum of row or column
   number against
    total counts.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    <LI> Orientation     - Whether to sum rows (D_H) or columns (D_V) </LI>
    </UL>

    Optional properties:
    <UL>
    <LI> XMin - The starting X value for each spectrum to include in the
   summation (default min). </LI>
    <LI> XMax - The ending X value for each spectrum to include in the summation
   (default max). </LI>
    <LI> HOverVMin - The first row to include when summing by columns, or vice
   versa (default all). </LI>
    <LI> HOverVmax - The last row to include when summing by columns, or vice
   versa (default all). </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 22/06/2009
*/
class MANTID_ALGORITHMS_DLL SumRowColumn : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "SumRowColumn"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "SANS-specific algorithm which gives a single spectrum containing "
           "the total counts in either each row or each column of pixels in a "
           "square LOQ or SANS2D detector bank.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SumSpectra", "SumNeighbours"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS;Transforms\\Grouping"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  API::MatrixWorkspace_sptr integrateWorkspace();
};

} // namespace Algorithms
} // namespace Mantid
