#ifndef MANTID_CURVEFITTING_CRYSTALFIELDCONTROL_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDCONTROL_H_

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  A function that controls creation of the source of CrystalFieldFunction.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_CURVEFITTING_DLL CrystalFieldControl
    : public API::CompositeFunction {
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
  void parseStringListAttribute(const std::string &attName,
                                const std::string &value,
                                std::vector<std::string> &cache);
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

class MANTID_CURVEFITTING_DLL CrystalFieldSpectrumControl
    : public API::ParamFunction {
public:
  CrystalFieldSpectrumControl();
  std::string name() const override;
  void function(const API::FunctionDomain &,
                API::FunctionValues &) const override;
};

class MANTID_CURVEFITTING_DLL CrystalFieldPhysPropControl
    : public API::ParamFunction {
public:
  CrystalFieldPhysPropControl();
  std::string name() const override;
  void function(const API::FunctionDomain &,
                API::FunctionValues &) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CRYSTALFIELDCONTROL_H_ */
