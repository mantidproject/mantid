#ifndef MANTID_CURVEFITTING_COMPTONPEAKPROFILE_H_
#define MANTID_CURVEFITTING_COMPTONPEAKPROFILE_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {
namespace CurveFitting {
/**
  This implements a resolution function for fitting a single mass in a compton
  scattering spectrum. The
  function has two domains defined by the VoigtEnergyCutOff attribute:

    - < VoigtEnergyCutOff: Voigt approximation to spectrum is used
    - >= VoigtEnergyCutOff: Gaussian approximation to spectrum is used.

  Both are normalized by the peak area.

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
class MANTID_CURVEFITTING_DLL ComptonPeakProfile
    : public virtual API::ParamFunction,
      public virtual API::IFunction1D {
public:
  /// Default constructor required for factory
  ComptonPeakProfile();

private:
  std::string name() const;

  /** @name Function evaluation */
  ///@{
  /// Calculate the function
  void function1D(double *out, const double *xValues, const size_t nData) const;
  /// Ensure the object is ready to be fitted
  void setUpForFit();
  /// Cache a copy of the workspace pointer and pull out the parameters
  void setWorkspace(boost::shared_ptr<const API::Workspace> ws);
  ///@}

  void declareParameters();
  void declareAttributes();
  void setAttribute(const std::string &name, const Attribute &value);

  /// WorkspaceIndex attribute
  size_t m_wsIndex;
  /// Mass of peak
  double m_mass;
  /// Below this value a Voigt is used for profile approximation
  double m_voigtCutOff;

  /// Gaussian function for lower-energy peaks
  boost::shared_ptr<API::IPeakFunction> m_gauss;
  /// Voigt function for higher-energy peaks
  boost::shared_ptr<API::IPeakFunction> m_voigt;

  /// Final energy of analyser
  double m_efixed;
  /// Calculated value of lorentz width
  double m_hwhmLorentz;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_COMPTONPEAKPROFILE_H_ */
