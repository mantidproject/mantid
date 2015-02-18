#ifndef PEAKFUNCTIONINTEGRATOR_H
#define PEAKFUNCTIONINTEGRATOR_H

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

    Copyright © 2014,2015 PSI-MSS

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

  IntegrationResult
  integrateInfinity(IPeakFunction_const_sptr peakFunction) const;
  IntegrationResult
  integratePositiveInfinity(IPeakFunction_const_sptr peakFunction,
                            double lowerLimit) const;
  IntegrationResult
  integrateNegativeInfinity(IPeakFunction_const_sptr peakFunction,
                            double upperLimit) const;

  IntegrationResult integrate(IPeakFunction_const_sptr peakFunction,
                              double lowerLimit, double upperLimit) const;

protected:
  gsl_function getGSLFunction(IPeakFunction_const_sptr peakFunction) const;
  void throwIfInvalid(IPeakFunction_const_sptr peakFunction) const;

  gsl_integration_workspace *m_integrationWorkspace;

  double m_relativePrecision;
};

double MANTID_API_DLL gsl_peak_wrapper(double x, void *parameters);
}
}

#endif // PEAKFUNCTIONINTEGRATOR_H
