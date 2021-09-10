// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/VesuvioResolution.h"
#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {
//---------------------------------------------------------------------------
// Forward declarations
//---------------------------------------------------------------------------
struct DetectorParams;
} // namespace Algorithms

namespace Functions {
/**
  This class serves as a base-class for ComptonProfile type functions. @see
  GaussianComptonProfile, GramCharlierComptonProfile
*/
class MANTID_CURVEFITTING_DLL ComptonProfile : public API::ParamFunction, public API::IFunction1D {
public:
  /// Default constructor required for factory
  ComptonProfile();

  /** @name Function evaluation */
  ///@{
  /// Calculate the function
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  /// Ensure the object is ready to be fitted
  void setUpForFit() override;
  /// Cache a copy of the workspace pointer and pull out the parameters
  void setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wsIndex, double startX,
                          double endX) override;

  /// Initiate a Y-space value chache rebuild when workspace of mass are
  /// changed.
  void buildCaches();

  /// Pre-calculate the Y-space values with specified resolution parameters
  void cacheYSpaceValues(const HistogramData::Points &tseconds, const Algorithms::DetectorParams &detpar,
                         const ResolutionParams &respar);

  /// Pre-calculate the Y-space values
  virtual void cacheYSpaceValues(const HistogramData::Points &tseconds, const Algorithms::DetectorParams &detpar);
  /// Turn off logger
  void disableLogging() { m_log.setEnabled(false); }
  ///@}

  /// Override to calculate the value of the profile for this mass and store in
  /// the given array
  virtual void massProfile(double *result, const size_t nData) const = 0;
  /// Returns the indices of the intensity parameters
  virtual std::vector<size_t> intensityParameterIndices() const = 0;
  /// Fill the appropriate columns of the given matrix with the values
  /// of the mass profile
  virtual size_t fillConstraintMatrix(Kernel::DblMatrix &cmatrix, const size_t start,
                                      const HistogramData::HistogramE &errors) const = 0;

  void setParameter(size_t i, const double &value, bool explicitlySet = true) override;

  void setParameter(const std::string &name, const double &value, bool explicitlySet = true) override {
    ParamFunction::setParameter(name, value, explicitlySet);
  }

protected:
  /// Declare parameters that will never participate in the fit
  void declareParameters() override;

  /// Access y-values cache
  inline const std::vector<double> &ySpace() const { return m_yspace; }
  /// Access Q values cache
  inline const std::vector<double> &modQ() const { return m_modQ; }
  /// Access e0 values
  inline const std::vector<double> &e0() const { return m_e0; }
  /// Access the mass
  inline double mass() const { return m_mass; }

  /// Compute Voigt function interpolated around the given values
  void voigtApproxDiff(std::vector<double> &voigtDiff, const std::vector<double> &yspace, const double lorentzPos,
                       const double lorentzAmp, const double lorentzWidth, const double gaussWidth) const;
  /// Compute Voigt function
  void voigtApprox(std::vector<double> &voigt, const std::vector<double> &yspace, const double lorentzPos,
                   const double lorentzAmp, const double lorentzWidth, const double gaussWidth) const;

protected:
  /// Logger
  mutable Kernel::Logger m_log;

  /// Current workspace
  std::shared_ptr<const API::MatrixWorkspace> m_workspace;
  /// Current workspace index, required to access instrument parameters
  size_t m_wsIndex;

  double m_startX;
  double m_endX;

  /// Voigt function
  std::shared_ptr<API::IPeakFunction> m_voigt;
  /// Vesuvio resolution function
  std::shared_ptr<VesuvioResolution> m_resolutionFunction;

  /** @name Caches for commonly used values*/
  ///@{
  /// Y-values
  std::vector<double> m_yspace;
  /// Q-values
  std::vector<double> m_modQ;
  /// Incident energies
  std::vector<double> m_e0;
  /// Atomic mass
  double m_mass;
  ///@}
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
