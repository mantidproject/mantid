// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  A function that controls creation of the source of CrystalFieldFunction.
*/
class MANTID_CURVEFITTING_DLL CrystalFieldControl : public API::CompositeFunction {
public:
  CrystalFieldControl();
  /// Set a value to attribute
  void setAttribute(const std::string &name, const Attribute &) override;
  /// Build control functions for individual spectra.
  void buildControls();
  /// Build the source function.
  API::IFunction_sptr buildSource();
  /// Build the source function in a single site case.
  API::IFunction_sptr buildSingleSite();
  /// Build the source function in a multi site case.
  API::IFunction_sptr buildMultiSite();
  /// Build the source function in a single site - single spectrum case.
  API::IFunction_sptr buildSingleSiteSingleSpectrum();
  /// Build the source function in a single site - multi spectrum case.
  API::IFunction_sptr buildSingleSiteMultiSpectrum();
  /// Build the source function in a multi site - single spectrum case.
  API::IFunction_sptr buildMultiSiteSingleSpectrum();
  /// Build the source function in a multi site - multi spectrum case.
  API::IFunction_sptr buildMultiSiteMultiSpectrum();
  /// Are there multiple ions?
  bool isMultiSite() const;
  /// Is it a multi-spectrum case?
  bool isMultiSpectrum() const;
  /// Any peaks defined?
  bool hasPeaks() const;
  /// Check if there are any phys. properties.
  bool hasPhysProperties() const;
  const std::vector<double> &temperatures() const;
  const std::vector<double> &FWHMs() const;
  const std::vector<std::string> &physProps() const;

private:
  /// Build control functions for phys properties.
  void buildPhysPropControls();
  /// Cache the attributes
  void cacheAttributes();
  /// Check that everything is consistent
  void checkConsistent();
  /// Parse a comma-separated list attribute
  void parseStringListAttribute(const std::string &attName, const std::string &value, std::vector<std::string> &cache);
  ///// @name Attribute caches
  ////@{
  /// The ion names
  std::vector<std::string> m_ions;
  /// The symmetries
  std::vector<std::string> m_symmetries;
  /// The temperatures
  std::vector<double> m_temperatures;
  /// Cache the default peak FWHMs
  std::vector<double> m_FWHMs;
  /// Caches of the width functions
  std::vector<std::vector<double>> m_fwhmX;
  std::vector<std::vector<double>> m_fwhmY;
  /// The physical properties
  std::vector<std::string> m_physProps;
  //@}
};

class MANTID_CURVEFITTING_DLL CrystalFieldSpectrumControl : public API::ParamFunction {
public:
  CrystalFieldSpectrumControl();
  std::string name() const override;
  void function(const API::FunctionDomain &, API::FunctionValues &) const override;
};

class MANTID_CURVEFITTING_DLL CrystalFieldPhysPropControl : public API::ParamFunction {
public:
  CrystalFieldPhysPropControl();
  std::string name() const override;
  void function(const API::FunctionDomain &, API::FunctionValues &) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
