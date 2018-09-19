#ifndef MANTID_CURVEFITTING_VESUVIORESOLUTION_H_
#define MANTID_CURVEFITTING_VESUVIORESOLUTION_H_

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {
//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
struct DetectorParams;
} // namespace Algorithms
namespace Functions {

//---------------------------------------------------------------------------
/// Simple data structure to store resolution parameter values
/// It avoids some functions taking a huge number of arguments
struct ResolutionParams {
  double dl1;        ///< spread in source-sample distance (m)
  double dl2;        ///< spread in sample-detector distance (m)
  double dtof;       ///< spread in tof measurement (us)
  double dthe;       ///< spread in scattering angle (radians)
  double dEnLorentz; ///< lorentz width in energy (meV)
  double dEnGauss;   ///< gaussian width in energy (meV
};

/**

  Calculate the resolution from a workspace of Vesuvio data using the mass &
  instrument definition.

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_CURVEFITTING_DLL VesuvioResolution : public API::ParamFunction,
                                                  public API::IFunction1D {
public:
  /// Creates a POD struct containing the required resolution parameters for
  /// this spectrum
  static ResolutionParams
  getResolutionParameters(const API::MatrixWorkspace_const_sptr &ws,
                          const size_t index);

  /// Default constructor required for factory
  VesuvioResolution();

  /** @name Function evaluation */
  ///@{
  /// A string identifier for this function
  std::string name() const override;
  /// Access total resolution width
  inline double resolutionFWHM() const { return m_resolutionSigma; }
  /// Access lorentz FWHM
  inline double lorentzFWHM() const { return m_lorentzFWHM; }
  /// Calculate the function
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  /// Ensure the object is ready to be fitted
  void setUpForFit() override;
  /// Cache a copy of the workspace pointer and pull out the parameters
  void
  setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,
                     size_t wsIndex, double startX, double endX) override;
  /// Pre-calculate the resolution components values
  void cacheResolutionComponents(const Algorithms::DetectorParams &detpar,
                                 const ResolutionParams &respar);
  /// Turn off logger
  void disableLogging() { m_log.setEnabled(false); }
  /// Compute Voigt function
  void voigtApprox(std::vector<double> &voigt,
                   const std::vector<double> &xValues, const double lorentzPos,
                   const double lorentzAmp, const double lorentzWidth,
                   const double gaussWidth) const;
  /// Compute Voigt function with cached values
  void voigtApprox(std::vector<double> &voigt,
                   const std::vector<double> &xValues, const double lorentzPos,
                   const double lorentzAmp) const;

  ///@}

private:
  /// Declare parameters that will never participate in the fit
  void declareAttributes() override;
  /// Set an attribute value (and possibly cache its value)
  void setAttribute(const std::string &name, const Attribute &value) override;

  /// Logger
  mutable Kernel::Logger m_log;
  /// Current workspace index, required to access instrument parameters
  size_t m_wsIndex;
  /// Store the mass values
  double m_mass;
  /// Voigt function
  boost::shared_ptr<API::IPeakFunction> m_voigt;
  /// Total resolution width
  double m_resolutionSigma;
  /// Lorentz FWHM
  double m_lorentzFWHM;
  ///@}
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_VESUVIORESOLUTION_H_ */
