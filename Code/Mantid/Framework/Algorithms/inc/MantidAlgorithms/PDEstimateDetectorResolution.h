#ifndef MANTID_ALGORITHMS_PDESTIMATEDETECTORRESOLUTION_H_
#define MANTID_ALGORITHMS_PDESTIMATEDETECTORRESOLUTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Algorithms {
/** PDEstimateDetectorResolution : TODO: DESCRIPTION

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
class DLLExport PDEstimateDetectorResolution : public API::Algorithm {
public:
  PDEstimateDetectorResolution();
  virtual ~PDEstimateDetectorResolution();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const;

  /// function to return any aliases to the algorithm
  virtual const std::string alias() const;

  /// Summary of algorithms purpose
  virtual const std::string summary() const;

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const;

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const;

private:
  /// Implement abstract Algorithm methods
  void init();
  /// Implement abstract Algorithm methods
  void exec();

  /// Returns the wavelength from either the property or the input workspace
  double getWavelength();

  /// Process input properties for algorithm
  void processAlgProperties();

  ///
  void retrieveInstrumentParameters();

  /// Create output workspace
  void createOutputWorkspace();

  /// Calculate detector resolution
  void estimateDetectorResolution();

  //------------------------------------------------------

  /// Input workspace
  API::MatrixWorkspace_sptr m_inputWS;

  /// Output workspace
  API::MatrixWorkspace_sptr m_outputWS;

  /// Centre neutron velocity
  double m_centreVelocity;

  /// L1, source to sample
  double m_L1;

  /// Delta T
  double m_deltaT;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_PDESTIMATEDETECTORRESOLUTION_H_ */
