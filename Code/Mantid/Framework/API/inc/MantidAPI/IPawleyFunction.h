#ifndef MANTID_API_IPAWLEYFUNCTION_H_
#define MANTID_API_IPAWLEYFUNCTION_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace API {

/** IPawleyFunction

  This abstract class defines the interface of a PawleyFunction. An
  implementation can be found in CurveFitting/PawleyFunction. This interface
  exists so that the function can be used in modules outside CurveFitting.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 11/03/2015

  Copyright © 2015 PSI-NXMM

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
class MANTID_API_DLL IPawleyFunction : public FunctionParameterDecorator {
public:
  IPawleyFunction();
  /// Virtual destructor.
  virtual ~IPawleyFunction() {}

  /// A string that names the crystal system.
  virtual void setCrystalSystem(const std::string &crystalSystem) = 0;

  /// Sets the name of the profile function used for the reflections.
  virtual void setProfileFunction(const std::string &profileFunction) = 0;

  /// Set the function parameters according to the supplied unit cell.
  virtual void setUnitCell(const std::string &unitCellString) = 0;

  /// Assign several peaks with the same fwhm/height parameters.
  virtual void setPeaks(const std::vector<Kernel::V3D> &hkls, double fwhm,
                        double height) = 0;

  /// Removes all peaks from the function.
  virtual void clearPeaks() = 0;

  /// Add a peak with the given parameters.
  virtual void addPeak(const Kernel::V3D &hkl, double fwhm, double height) = 0;

  /// Returns the number of peaks in the function
  virtual size_t getPeakCount() const = 0;

  /// Returns the profile function stored for the i-th peak.
  virtual IPeakFunction_sptr getPeakFunction(size_t i) const = 0;

  /// Returns the Miller indices stored for the i-th peak.
  virtual Kernel::V3D getPeakHKL(size_t i) const = 0;
};

typedef boost::shared_ptr<IPawleyFunction> IPawleyFunction_sptr;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_IPAWLEYFUNCTION_H_ */
