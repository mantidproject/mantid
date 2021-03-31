// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IPeakFunction.h"
#include "gsl/gsl_integration.h"

namespace Mantid {
namespace API {

/** PeakFunctionIntegrator :
 *
  General integration of peaks (in the form of IPeakFunction) by wrapping the
  corresponding GSL-functions. Integration with infinity limits is supported.

  PeakFunctionIntegrator allocates a GSL integration workspace on construction
  and frees the memory when it's destroyed.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 24/04/2014
*/

struct MANTID_API_DLL IntegrationResult {
  double result;
  double error;
  size_t intervals;

  int errorCode;
  bool success;
};

class MANTID_API_DLL PeakFunctionIntegrator {
public:
  PeakFunctionIntegrator(double requiredRelativePrecision = 1e-8);
  virtual ~PeakFunctionIntegrator();

  void setRequiredRelativePrecision(double newPrecision);
  double requiredRelativePrecision() const;

  IntegrationResult integrateInfinity(const IPeakFunction &peakFunction) const;
  IntegrationResult integratePositiveInfinity(const IPeakFunction &peakFunction, double lowerLimit) const;
  IntegrationResult integrateNegativeInfinity(const IPeakFunction &peakFunction, double upperLimit) const;

  IntegrationResult integrate(const IPeakFunction &peakFunction, double lowerLimit, double upperLimit) const;

protected:
  gsl_function getGSLFunction(const IPeakFunction &peakFunction) const;

  gsl_integration_workspace *m_integrationWorkspace;

  double m_relativePrecision;
};

double MANTID_API_DLL gsl_peak_wrapper(double x, void *parameters);
} // namespace API
} // namespace Mantid
