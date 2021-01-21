// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/
class MANTID_API_DLL IPawleyFunction : public FunctionParameterDecorator {
public:
  IPawleyFunction();

  /// A string that names the crystal system.
  virtual void setLatticeSystem(const std::string &crystalSystem) = 0;

  /// Sets the name of the profile function used for the reflections.
  virtual void setProfileFunction(const std::string &profileFunction) = 0;

  /// Set the function parameters according to the supplied unit cell.
  virtual void setUnitCell(const std::string &unitCellString) = 0;

  /// Assign several peaks with the same fwhm/height parameters.
  virtual void setPeaks(const std::vector<Kernel::V3D> &hkls, double fwhm, double height) = 0;

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

using IPawleyFunction_sptr = std::shared_ptr<IPawleyFunction>;

} // namespace API
} // namespace Mantid
