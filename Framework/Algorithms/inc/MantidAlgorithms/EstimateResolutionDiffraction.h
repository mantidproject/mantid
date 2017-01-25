#ifndef MANTID_ALGORITHMS_ESTIMATERESOLUTIONDIFFRACTION_H_
#define MANTID_ALGORITHMS_ESTIMATERESOLUTIONDIFFRACTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {
/** EstimateResolutionDiffraction : TODO: DESCRIPTION

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport EstimateResolutionDiffraction : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override;

  /// function to return any aliases to the algorithm
  const std::string alias() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override;

  /// Algorithm's version for identification overriding a virtual method
  int version() const override;

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;

  /// Returns the wavelength from either the property or the input workspace
  double getWavelength();

  /// Process input properties for algorithm
  void processAlgProperties();

  ///
  void retrieveInstrumentParameters();

  /// Calculate detector resolution
  void estimateDetectorResolution();

  /// Input workspace
  API::MatrixWorkspace_sptr m_inputWS;

  /// Output workspace
  API::MatrixWorkspace_sptr m_outputWS;

  /// Centre neutron velocity
  double m_centreVelocity = 0.0;

  /// Delta T
  double m_deltaT = 0.0;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ESTIMATERESOLUTIONDIFFRACTION_H_ */
