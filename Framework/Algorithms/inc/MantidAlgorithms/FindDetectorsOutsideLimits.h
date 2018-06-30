#ifndef MANTID_ALGORITHM_FINDDETECTORSOUTSIDELIMITS_H_
#define MANTID_ALGORITHM_FINDDETECTORSOUTSIDELIMITS_H_

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

   Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

   This file is part of Mantid.

   Mantid is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Mantid is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   File change history is stored at: <https://github.com/mantidproject/mantid>
   Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FindDetectorsOutsideLimits : public DetectorDiagnostic {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override {
    return "FindDetectorsOutsideLimits";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Identifies histograms and their detectors that have total numbers "
           "of counts over a user defined maximum or less than the user define "
           "minimum.";
  }

  const std::string category() const override;
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"FindDeadDetectors", "DetectorDiagnostic"};
  }

private:
  /// Overridden init
  void init() override;
  /// Overridden exec
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FINDDETECTORSOUTSIDELIMITS_H_*/
