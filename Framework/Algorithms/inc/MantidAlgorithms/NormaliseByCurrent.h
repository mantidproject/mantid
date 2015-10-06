#ifndef MANTID_ALGORITHMS_NORMALISEBYCURRENT_H_
#define MANTID_ALGORITHMS_NORMALISEBYCURRENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <boost/shared_ptr.hpp>
namespace Mantid {
namespace API {
// Forward declare
class MatrixWorkspace;
}
namespace Algorithms {
/** Normalises a workspace according to the good proton charge figure taken from
   the
    raw file (and stored in the Sample object). Every data point is divided by
   that number.
    Note that units are not fully dealt with at the moment - the output will
   have identical
    units to the input workspace (i.e. will not show "/ uA.hour").

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The names of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 25/08/2008

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport NormaliseByCurrent : public API::Algorithm {
public:
  NormaliseByCurrent();
  virtual ~NormaliseByCurrent();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "NormaliseByCurrent"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Normalises a workspace by the proton charge.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "CorrectionFunctions\\NormalisationCorrections";
  }

private:
  // Overridden Algorithm methods
  void init();
  void exec();
  // Extract the charge value from the logs.
  double
  extractCharge(boost::shared_ptr<Mantid::API::MatrixWorkspace> inputWs) const;
};

} // namespace Algorithm
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_NORMALISEBYCURRENT_H_ */
