#ifndef MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_
#define MANTID_SINQ_POLDISPECTRUMDOMAINFUNCTION_H_

#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/FunctionDomain1D.h"
#include <string>

#include "MantidAPI/IPeakFunction.h"
#include "MantidSINQ/PoldiUtilities/IPoldiFunction1D.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"
#include "MantidSINQ/PoldiUtilities/PoldiTimeTransformer.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"

namespace Mantid {
namespace Poldi {

struct MANTID_SINQ_DLL Poldi2DHelper {
  void setChopperSlitOffsets(double distance, double sinTheta, double deltaD,
                             const std::vector<double> &offsets) {
    dFractionalOffsets.clear();
    dOffsets.clear();

    for (auto it = offsets.begin(); it != offsets.end(); ++it) {
      double dEquivalent = Conversions::TOFtoD(*it, distance, sinTheta);
      double rounded = floor(dEquivalent / deltaD + 0.5);
      dOffsets.push_back(static_cast<int>(rounded));
      dFractionalOffsets.push_back(dEquivalent - rounded * deltaD);
    }
  }

  void setDomain(double dMin, double dMax, double deltaD) {
    int dMinN = static_cast<int>(dMin / deltaD);
    int dMaxN = static_cast<int>(dMax / deltaD);

    std::vector<double> current;
    current.reserve(dMaxN - dMinN);

    for (int i = dMinN; i <= dMaxN; ++i) {
      current.push_back(static_cast<double>(i + 0.5) * deltaD);
    }

    domain = boost::make_shared<API::FunctionDomain1DVector>(current);
  }

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
  std::vector<double> factors;

  double deltaD;
  int minTOFN;
};

typedef boost::shared_ptr<Poldi2DHelper> Poldi2DHelper_sptr;

class LocalJacobian : public API::Jacobian {
public:
  /// Constructor
  LocalJacobian(size_t nValues, size_t nParams)
      : Jacobian(), m_nValues(nValues), m_nParams(nParams),
        m_jacobian(nValues * nParams) {}

  /// Destructor
  ~LocalJacobian() {}

  /// Implementation of API::Jacobian::set. Throws std::out_of_range on invalid
  /// index.
  void set(size_t iY, size_t iP, double value) {
    m_jacobian[safeIndex(iY, iP)] = value;
  }

  /// Implementation of API::Jacobian::get. Throws std::out_of_range on invalid
  /// index.
  double get(size_t iY, size_t iP) { return m_jacobian[safeIndex(iY, iP)]; }

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

/** PoldiSpectrumDomainFunction : TODO: DESCRIPTION

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

class MANTID_SINQ_DLL PoldiSpectrumDomainFunction
    : virtual public API::ParamFunction,
      virtual public API::IFunction1DSpectrum,
      public IPoldiFunction1D {
public:
  PoldiSpectrumDomainFunction();
  virtual ~PoldiSpectrumDomainFunction() {}

  virtual std::string name() const { return "PoldiSpectrumDomainFunction"; }

  virtual void setWorkspace(boost::shared_ptr<const API::Workspace> ws);
  virtual void function1DSpectrum(const API::FunctionDomain1DSpectrum &domain,
                                  API::FunctionValues &values) const;

  virtual void
  functionDeriv1DSpectrum(const API::FunctionDomain1DSpectrum &domain,
                          API::Jacobian &jacobian);

  void poldiFunction1D(const std::vector<int> &indices,
                       const API::FunctionDomain1D &domain,
                       API::FunctionValues &values) const;

  virtual void setActiveParameter(size_t i, double value);
  virtual double activeParameter(size_t i) const;

  virtual void setParameter(size_t i, const double &value,
                            bool explicitlySet = true);
  virtual void setParameter(const std::string &name, const double &value,
                            bool explicitlySet = true);
  virtual double getParameter(size_t i) const;
  virtual double getParameter(const std::string &name) const;

  virtual void setAttribute(const std::string &attName,
                            const API::IFunction::Attribute &attValue);

  API::IPeakFunction_sptr getProfileFunction() const;

protected:
  virtual void init();

  void initializeParametersFromWorkspace(
      const DataObjects::Workspace2D_const_sptr &workspace2D);
  void initializeInstrumentParameters(
      const PoldiInstrumentAdapter_sptr &poldiInstrument);

  void setProfileFunction(const std::string &profileFunctionName);

  void exposeFunctionParameters(const API::IFunction_sptr &function);

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
