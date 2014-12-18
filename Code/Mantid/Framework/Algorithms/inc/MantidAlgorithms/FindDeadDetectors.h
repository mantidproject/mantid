#ifndef MANTID_ALGORITHM_FINDDEADDETECTORS_H_
#define MANTID_ALGORITHM_FINDDEADDETECTORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**
Takes a workspace as input and identifies all of the spectra that have a
value across all time bins less or equal to than the threshold 'dead' value.
This is then used to mark all 'dead' detectors with a 'dead' marker value,
while all spectra from live detectors are given a value of 'live' marker value.

This is primarily used to ease identification using the instrument visualization
tools.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
</UL>

Optional Properties:
<UL>
<LI> DeadThreshold - The threshold against which to judge if a spectrum belongs
to a dead detector (default 0.0)</LI>
<LI> LiveValue - The value to assign to an integrated spectrum flagged as 'live'
(default 0.0)</LI>
<LI> DeadValue - The value to assign to an integrated spectrum flagged as 'dead'
(default 100.0)</LI>
<LI> StartX - Start the integration at the above bin above the one that this
value is in (default: the start of each histogram)</LI>
<LI> EndX - Stop the integration at the bin before the one that contains this x
value (default: the end of each histogram)</LI>
<LI> OutputFile - (Optional) A filename to which to write the list of dead
detector UDETs </LI>
</UL>

@author Nick Draper, Tessella Support Services plc
@date 02/10/2008

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
class DLLExport FindDeadDetectors : public API::Algorithm {
public:
  /// Default constructor
  FindDeadDetectors() : API::Algorithm(){};
  /// Destructor
  virtual ~FindDeadDetectors(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FindDeadDetectors"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Identifies and flags empty spectra caused by 'dead' detectors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diagnostics"; }

private:
  // Overridden Algorithm methods
  void init();
  void exec();

  API::MatrixWorkspace_sptr integrateWorkspace();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FINDDEADDETECTORS_H_*/
