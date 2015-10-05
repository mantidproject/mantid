#ifndef MANTID_ALGORITHMS_FILTERBADPULSES_H_
#define MANTID_ALGORITHMS_FILTERBADPULSES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {

namespace Algorithms {

/** Filters out events associated with pulses that happen when proton charge is
   lower than a given percentage of the average.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace whose detectors are to be
   aligned </LI>
    <LI> OutputWorkspace - The name of the Workspace in which the result of the
   algorithm will be stored </LI>
    <LI> LowerCutoff - The percentage of the average to use as the lower bound
   </LI>
    </UL>

    @author Peter Peterson, ORNL
    @date 21/12/10

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FilterBadPulses : public API::Algorithm {
public:
  FilterBadPulses();
  virtual ~FilterBadPulses();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Filters out events associated with pulses that happen when proton "
           "charge is lower than a given percentage of the average.";
  }

  virtual int version() const;

  virtual const std::string category() const;

private:
  // Implement abstract Algorithm methods
  void init();
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FILTERBADPULSES_H_ */
