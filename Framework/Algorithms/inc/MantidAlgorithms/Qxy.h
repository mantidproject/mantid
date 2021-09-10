// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** This algorithm rebins a 2D workspace in units of wavelength into 2D Q.
    The result is stored in a 2D workspace with units of Q on both axes.
    @todo Doesn't (yet) calculate the errors.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The corrected data in units of wavelength. </LI>
    <LI> OutputWorkspace - The workspace in which to store data as x & y
   components of Q. </LI>
    <LI> MaxQxy          - The upper limit of the Qx-Qy grid (goes from -MaxQxy
   to +MaxQxy). </LI>
    <LI> DeltaQ          - The dimension of a Qx-Qy cell. </LI>
    <LI> AccountForGravity - If true, account for gravity. </LI>
    <LI> SolidAngleWeighting - If true, pixels will be weighted by their solid
   angle. </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 09/04/2009
*/
class MANTID_ALGORITHMS_DLL Qxy : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "Qxy"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs the final part of a SANS (LOQ/SANS2D) two dimensional (in "
           "Q) data reduction.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"Q1D"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  std::vector<double> logBinning(double min, double max, int num);
  API::MatrixWorkspace_sptr setUpOutputWorkspace(const API::MatrixWorkspace_const_sptr &inputWorkspace);
  double getQminFromWs(const API::MatrixWorkspace &inputWorkspace);
};

} // namespace Algorithms
} // namespace Mantid
