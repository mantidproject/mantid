#ifndef MANTID_ALGORITHM_HE3TUBEEFFICIENCY_H_
#define MANTID_ALGORITHM_HE3TUBEEFFICIENCY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/V3D.h"
#include <vector>

namespace Mantid
{
namespace Algorithms
{
/**
    Corrects the input workspace for helium3 tube efficiency based on an
    exponential parameterization. The algorithm expects the input workspace
    units to be wavelength.

    @author Michael Reuter
    @date 30/09/2010

    Copyright &copy; 2008-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport He3TubeEfficiency: public API::Algorithm
{
public:
  /// Default constructor
  He3TubeEfficiency();
  /// Virtual destructor
  virtual ~He3TubeEfficiency();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "He3TubeEfficiency"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const{ return "CorrectionFunctions"; }

private:
  // Implement abstract Algorithm methods
  void init();
  void exec();

  /// Correct the given spectra index for efficiency
  void correctForEfficiency(int spectraIndex);
  /// Sets the detector geometry cache if necessary
  void getDetectorGeometry(boost::shared_ptr<Geometry::IDetector> det,
      double & detRadius, Geometry::V3D & detAxis);
  /// Computes the distance to the given shape from a starting point
  double distToSurface(const Geometry::V3D start,
      const Geometry::Object *shape) const;
  /// Calculate the detector efficiency
  double detectorEfficiency() const;
  /// Log any errors with spectra that occurred
  void logErrors() const;

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr outputWS;
  /// Map that stores additional properties for detectors
  const Geometry::ParameterMap *paraMap;
  /// A lookup of previously seen shape objects used to save calculation time as most detectors have the same shape
  std::map<const Geometry::Object *, std::pair<double, Geometry::V3D> > shapeCache;
  /// Sample position
  Geometry::V3D samplePos;
  /// The spectra numbers that were skipped
  std::vector<int> spectraSkipped;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHM_HE3TUBEEFFICIENCY_H_ */
