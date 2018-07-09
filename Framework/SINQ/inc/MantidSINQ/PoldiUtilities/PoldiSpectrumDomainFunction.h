#ifndef MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_
#define MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/IPoldiFunction1D.h"
#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiTimeTransformer.h"

#include <string>

namespace Mantid {
namespace Poldi {

/**
 * @brief Helper struct for POLDI 2D spectrum calculation
 *
 * This helper struct collects some quantities that are required
 * for calculating POLDI 2D spectra with arbitrary profile functions.
 *
 * Bragg peak-parameters are stored in terms of d and their profiles
 * are also calculated d-dependent. The helper struct helps transforming
 * the d-spectrum to the arrival time spectrum, for example by translating
 * the chopper slit timing offsets into fractions of a d-bin. Also, it
 * holds the d-bins for a given 2theta, because the d-resolution at
 * each angle is different at the current POLDI setup.
 *
 * Also it provides intensity multiplication factors for each 2theta value.
 *
 * Having these things available removes the need of transforming parameters
 * on each call to PoldiSpectrumDomainFunction::function() and thus makes
 * PoldiSpectrumDomainFunction independent of knowing anything about the profile
 * function that is used.
 */
struct MANTID_SINQ_DLL Poldi2DHelper {
  /// Default constructor
  Poldi2DHelper()
      : dFractionalOffsets(), dOffsets(), domain(), values(), factors(),
        deltaD(), minTOFN() {}

  /// Transforms the chopper slit offsets for a given 2theta/distance pair.
  void setChopperSlitOffsets(double distance, double sinTheta, double deltaD,
                             const std::vector<double> &offsets) {
    dFractionalOffsets.clear();
    dOffsets.clear();

    for (const double offset : offsets) {
      double dEquivalent = Conversions::TOFtoD(offset, distance, sinTheta);
      double rounded = floor(dEquivalent / deltaD + 0.5);
      dOffsets.push_back(static_cast<int>(rounded));
      dFractionalOffsets.push_back(dEquivalent - rounded * deltaD);
    }
  }

  /// Generates the d-domain with the given parameters.
  void setDomain(double dMin, double dMax, double deltaD) {
    int dMinN = static_cast<int>(dMin / deltaD);
    int dMaxN = static_cast<int>(dMax / deltaD);

    std::vector<double> current;
    current.reserve(dMaxN - dMinN);

    for (int i = dMinN; i <= dMaxN; ++i) {
      current.push_back(static_cast<double>(i + 0.5) * deltaD);
    }

    domain = boost::make_shared<API::FunctionDomain1DVector>(current);
    values.reset(*domain);
  }

  /// Calculates intensity factors for each point in the spectrum domain.
  void setFactors(const PoldiTimeTransformer_sptr &timeTransformer,
                  size_t index) {
    factors.clear();
    if (domain && timeTransformer) {
      factors.reserve(domain->size());
      for (size_t i = 0; i < domain->size(); ++i) {
        factors.push_back(
            timeTransformer->detectorElementIntensity((*domain)[i], index));
      }
    }
  }

  std::vector<double> dFractionalOffsets;
  std::vector<int> dOffsets;

  API::FunctionDomain1D_sptr domain;
  API::FunctionValues values;
  std::vector<double> factors;

  double deltaD;
  int minTOFN;
};

using Poldi2DHelper_sptr = boost::shared_ptr<Poldi2DHelper>;

class WrapAroundJacobian : public API::Jacobian {
public:
  WrapAroundJacobian(API::Jacobian &jacobian, size_t offset,
                     const std::vector<double> &factors, size_t factorOffset,
                     size_t domainSize)
      : m_jacobian(jacobian), m_offset(offset), m_factors(factors),
        m_factorOffset(factorOffset), m_domainSize(domainSize) {}

  double get(size_t iY, size_t iP) override { return m_jacobian.get(iY, iP); }

  void set(size_t iY, size_t iP, double value) override {
    size_t realY = (m_offset + iY) % m_domainSize;
    m_jacobian.set(realY, iP,
                   m_jacobian.get(realY, iP) +
                       value * m_factors[m_factorOffset + iY]);
  }

  void zero() override { m_jacobian.zero(); }

protected:
  API::Jacobian &m_jacobian;
  size_t m_offset;
  const std::vector<double> &m_factors;
  size_t m_factorOffset;
  size_t m_domainSize;
};

/**
 * @brief The LocalJacobian class
 *
 * Transformation of the d-based profile into the arrival time based profile
 * is just a matter of multiplication by a constant factor. This means that
 * the derivatives of the profile function can be obtained for a given
 * range, using a small "local" jacobian matrix. The values from this matrix
 * are then copied to the right place in the actual Jacobian. This makes
 * PoldiSpectrumDomainFunction independent from knowing anything about
 * how the derivatives of the profile function are calculated.
 */
class LocalJacobian : public API::Jacobian {
public:
  /// Constructor
  LocalJacobian(size_t nValues, size_t nParams)
      : Jacobian(), m_nValues(nValues), m_nParams(nParams),
        m_jacobian(nValues * nParams) {}

  /// Implementation of API::Jacobian::set. Throws std::out_of_range on invalid
  /// index.
  void set(size_t iY, size_t iP, double value) override {
    m_jacobian[safeIndex(iY, iP)] = value;
  }

  /// Implementation of API::Jacobian::get. Throws std::out_of_range on invalid
  /// index.
  double get(size_t iY, size_t iP) override {
    return m_jacobian[safeIndex(iY, iP)];
  }

  /// Implements API::Jacobian::zero
  void zero() override { m_jacobian.assign(m_jacobian.size(), 0.0); }

  /// Provides raw pointer access to the underlying std::vector. Required for
  /// adept-interface.
  double *rawValues() { return &m_jacobian[0]; }

  /// Assign values to jacobian
  void copyValuesToJacobian(Jacobian &jacobian, size_t yOffset) {
    for (size_t y = 0; y < m_nValues; ++y) {
      for (size_t p = 0; p < m_nParams; ++p) {
        jacobian.set(y + yOffset, p, getRaw(y, p));
      }
    }
  }

  /// Get-method without checks.
  inline double getRaw(size_t iY, size_t iP) const {
    return m_jacobian[index(iY, iP)];
  }

protected:
  /// Index-calculation for vector access.
  inline size_t index(size_t iY, size_t iP) const {
    return iY + iP * m_nValues;
  }

  /// Get index and check for validity. Throws std::out_of_range on invalid
  /// index.
  size_t safeIndex(size_t iY, size_t iP) const {
    size_t i = index(iY, iP);

    if (i >= m_jacobian.size()) {
      throw std::out_of_range("Index is not valid for this Jacobian.");
    }

    return i;
  }

  size_t m_nValues;
  size_t m_nParams;
  std::vector<double> m_jacobian;
};

/** PoldiSpectrumDomainFunction

    PoldiSpectrumDomainFunction wraps an IPeakFunction. The parameters
    of the wrapped profile function are given in terms of d, whereas
    PoldiSpectrumDomainFunction operates in terms of "arrival time", which
    is specific to the POLDI experiment.

    The wrapped profile function is controlled by the "ProfileFunction"
    attribute. Setting the attribute creates the function in question and
    exposes its parameters as the parameters of PoldiSpectrumDomainFunction
    which can then be used for fitting.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 16/05/2014

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

class MANTID_SINQ_DLL PoldiSpectrumDomainFunction
    : public API::FunctionParameterDecorator,
      public API::IFunction1DSpectrum,
      public IPoldiFunction1D {
public:
  PoldiSpectrumDomainFunction();

  std::string name() const override { return "PoldiSpectrumDomainFunction"; }

  void setWorkspace(boost::shared_ptr<const API::Workspace> ws) override;
  void function1DSpectrum(const API::FunctionDomain1DSpectrum &domain,
                          API::FunctionValues &values) const override;

  void functionDeriv1DSpectrum(const API::FunctionDomain1DSpectrum &domain,
                               API::Jacobian &jacobian) override;

  void poldiFunction1D(const std::vector<int> &indices,
                       const API::FunctionDomain1D &domain,
                       API::FunctionValues &values) const override;

  API::IPeakFunction_sptr getProfileFunction() const;

protected:
  void init() override;

  void initializeParametersFromWorkspace(
      const DataObjects::Workspace2D_const_sptr &workspace2D);
  void initializeInstrumentParameters(
      const PoldiInstrumentAdapter_sptr &poldiInstrument);

  void beforeDecoratedFunctionSet(const API::IFunction_sptr &fn) override;

  std::vector<double>
  getChopperSlitOffsets(const PoldiAbstractChopper_sptr &chopper);

  std::vector<double> m_chopperSlitOffsets;
  double m_deltaT;
  PoldiTimeTransformer_sptr m_timeTransformer;
  std::vector<Poldi2DHelper_sptr> m_2dHelpers;
  API::IPeakFunction_sptr m_profileFunction;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_ */
