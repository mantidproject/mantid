// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionGenerator.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/EigenFortranDefs.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Calculates crystal field spectra.
*/
class MANTID_CURVEFITTING_DLL CrystalFieldMultiSpectrum : public API::FunctionGenerator {
public:
  CrystalFieldMultiSpectrum();

  void init() override;
  std::string name() const override { return "CrystalFieldMultiSpectrum"; }
  size_t getNumberDomains() const override;
  void setAttribute(const std::string &name, const Attribute &) override;
  std::vector<API::IFunction_sptr> createEquivalentFunctions() const override;
  void buildTargetFunction() const override;
  enum PhysicalProperty {
    HeatCapacity = 1,   ///< Specify dataset is magnetic heat capacity Cv(T)
    Susceptibility = 2, ///< Specify dataset is magnetic susceptibility chi(T)
    Magnetisation = 3,  ///< Specify dataset is magnetisation vs field M(H)
    MagneticMoment = 4  ///< Specify dataset is magnetisation vs temp M(T)
  };

protected:
  void updateTargetFunction() const override;

private:
  /// Build a function for a single spectrum.
  API::IFunction_sptr buildSpectrum(int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
                                    double temperature, double fwhm, size_t i) const;
  API::IFunction_sptr buildPhysprop(int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
                                    const ComplexFortranMatrix &ham, double temperature, size_t iSpec) const;
  /// Update a function for a single spectrum.
  void updateSpectrum(API::IFunction &spectrum, int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
                      const ComplexFortranMatrix &ham, double temperature, double fwhm, size_t i) const;
  /// Calculate excitations at given temperature
  void calcExcitations(int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf, double temperature,
                       API::FunctionValues &values, size_t iSpec) const;
  /// Cache number of fitted peaks
  mutable std::vector<size_t> m_nPeaks;
  /// Cache the list of "spectra" corresponding to physical properties
  mutable std::vector<int> m_physprops;
  /// Caches of the width functions
  mutable std::vector<std::vector<double>> m_fwhmX;
  mutable std::vector<std::vector<double>> m_fwhmY;
  /// Cache the temperatures
  mutable std::vector<double> m_temperatures;
  /// Cache the default peak FWHMs
  mutable std::vector<double> m_FWHMs;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
