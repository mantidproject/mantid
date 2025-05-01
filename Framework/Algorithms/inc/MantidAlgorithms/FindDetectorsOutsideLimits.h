// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/DetectorDiagnostic.h"

namespace Mantid {
namespace Algorithms {
/**
   Takes a workspace as input and identifies all spectra where the sum of the
   counts in all bins is outside a range. Those that are outside the range, i.e.
   fail
   the tests, have their spectra masked on the output workspace and those
   passing
   the tests are left with a postive value in the bin

   Required Properties:
   <UL>
   <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
   <LI> OutputWorkspace - The name of the workspace in which to store the
   results </LI>
   <LI> HighThreshold - Spectra whose total number of counts are above or equal
   to this
   value will be marked dead</LI>
   </UL>

   Optional Properties:
   <UL>
   <LI> LowThreshold - Spectra whose total number of counts are below or equal
   to this
   value will be marked dead (default 0)</LI>
   <LI> StartX - Start the integration at the above bin above the one that this
   value is
   in (default: the start of each histogram)</LI>
   <LI> EndX - Stop the integration at the bin before the one that contains this
   x value
   (default: the end of each histogram)</LI>
   </UL>

   @author Steve D Williams, ISIS Facility Rutherford Appleton Laboratory
   @date 07/07/2009
*/
class MANTID_ALGORITHMS_DLL FindDetectorsOutsideLimits : public DetectorDiagnostic {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FindDetectorsOutsideLimits"; }
  const std::string category() const override;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"FindDeadDetectors", "DetectorDiagnostic"}; }

private:
  /// Overridden init
  void init() override;
  /// Overridden exec
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
