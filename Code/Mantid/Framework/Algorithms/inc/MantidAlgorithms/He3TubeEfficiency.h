#ifndef MANTID_ALGORITHM_HE3TUBEEFFICIENCY_H_
#define MANTID_ALGORITHM_HE3TUBEEFFICIENCY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/V3D.h"
#include <map>
#include <vector>

namespace Mantid {
namespace Algorithms {
/**
    Corrects the input workspace for helium3 tube efficiency based on an
    exponential parameterization. The algorithm expects the input workspace
    units to be wavelength. The formula for the efficiency is given here.

    \f[
    \epsilon = \frac{A}{1-e^{\frac{-\alpha P (L - 2W) \lambda}{T sin(\theta)}}}
    \f]

    where \f$A\f$ is a dimensionless scaling factor, \f$\alpha\f$ is a constant
    with units \f$(Kelvin / (metres\: \mbox{\AA}\: atm))\f$, \f$P\f$ is pressure
   in
    units of \f$atm\f$, \f$L\f$ is the tube diameter in units of \f$metres\f$,
    \f$W\f$ is the tube thickness in units of \f$metres\f$, \f$T\f$ is the
    temperature in units of \f$Kelvin\f$, \f$sin(\theta)\f$ is the angle of
    the neutron trajectory with respect to the long axis of the He3 tube and
    \f$\lambda\f$ is in units of \f$\mbox{\AA}\f$.

    @author Michael Reuter
    @date 30/09/2010

    Copyright &copy; 2008-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport He3TubeEfficiency : public API::Algorithm {
public:
  /// Default constructor
  He3TubeEfficiency();
  /// Virtual destructor
  virtual ~He3TubeEfficiency();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "He3TubeEfficiency"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "He3 tube efficiency correction.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "CorrectionFunctions\\EfficiencyCorrections";
  }

private:
  // Implement abstract Algorithm methods
  void init();
  void exec();
  void execEvent();

  /// Correct the given spectra index for efficiency
  void correctForEfficiency(std::size_t spectraIndex);
  /// Sets the detector geometry cache if necessary
  void getDetectorGeometry(boost::shared_ptr<const Geometry::IDetector> det,
                           double &detRadius, Kernel::V3D &detAxis);
  /// Computes the distance to the given shape from a starting point
  double distToSurface(const Kernel::V3D start,
                       const Geometry::Object *shape) const;
  /// Calculate the detector efficiency
  double detectorEfficiency(const double alpha,
                            const double scale_fac = 1.0) const;
  /// Log any errors with spectra that occurred
  void logErrors() const;
  /// Retrieve the detector parameters from workspace or detector properties
  double getParameter(std::string wsPropName, std::size_t currentIndex,
                      std::string detPropName,
                      boost::shared_ptr<const Geometry::IDetector> idet);
  /// Helper for event handling
  template <class T> void eventHelper(std::vector<T> &events, double expval);
  /// Function to calculate exponential contribution
  double
  calculateExponential(std::size_t spectraIndex,
                       boost::shared_ptr<const Geometry::IDetector> idet);

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr outputWS;
  /// Map that stores additional properties for detectors
  const Geometry::ParameterMap *paraMap;
  /// A lookup of previously seen shape objects used to save calculation time as
  /// most detectors have the same shape
  std::map<const Geometry::Object *, std::pair<double, Kernel::V3D>> shapeCache;
  /// Sample position
  Kernel::V3D samplePos;
  /// The spectra numbers that were skipped
  std::vector<specid_t> spectraSkipped;
  /// Algorithm progress keeper
  API::Progress *progress;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHM_HE3TUBEEFFICIENCY_H_ */
