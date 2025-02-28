// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IPawleyFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ParamFunction.h"

#include "MantidCurveFitting/DllConfig.h"

#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** @class PawleyParameterFunction

  This function is used internally by PawleyFunction to hold the unit cell
  parameters as well as the ZeroShift parameter. The function and functionDeriv-
  methods have been implemented to do nothing, the calculation of the spectrum
  that results from the unit cell is calculated in PawleyFunction.

  Additionally it stores the crystal system and the name of the profile function
  that is used to model the Bragg peaks as attributes.
*/
class MANTID_CURVEFITTING_DLL PawleyParameterFunction : virtual public API::IFunction,
                                                        virtual public API::ParamFunction {
public:
  PawleyParameterFunction();

  /// Returns the function name
  std::string name() const override { return "PawleyParameterFunction"; }

  void setAttribute(const std::string &attName, const Attribute &attValue) override;

  Geometry::PointGroup::LatticeSystem getLatticeSystem() const;
  Geometry::UnitCell getUnitCellFromParameters() const;
  void setParametersFromUnitCell(const Geometry::UnitCell &cell);

  /// Returns the stored profile function name
  std::string getProfileFunctionName() const { return getAttribute("ProfileFunction").asString(); }

  /// Returns the name of the stored function's center parameter
  const std::string &getProfileFunctionCenterParameterName() const { return m_profileFunctionCenterParameterName; }

  void function(const API::FunctionDomain &domain, API::FunctionValues &values) const override;
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override;

protected:
  void init() override;

  void setProfileFunction(const std::string &profileFunction);
  void setLatticeSystem(const std::string &latticeSystem);

  void createLatticeSystemParameters(Geometry::PointGroup::LatticeSystem latticeSystem);

  void addLengthConstraint(const std::string &parameterName);
  void addAngleConstraint(const std::string &parameterName);

  void setCenterParameterNameFromFunction(const API::IPeakFunction_sptr &profileFunction);

  Geometry::PointGroup::LatticeSystem m_latticeSystem;
  std::string m_profileFunctionCenterParameterName;
};

using PawleyParameterFunction_sptr = std::shared_ptr<PawleyParameterFunction>;

/** @class PawleyFunction

  The Pawley approach to obtain lattice parameters from a powder diffractogram
  works by placing peak profiles at d-values (which result from the lattice
  parameters and the Miller indices of each peak) and fitting the total profile
  to the recorded diffractogram.

  Depending on the chosen crystal system, this function exposes the appropriate
  lattice parameters as parameters, as well as profile parameters of the
  individual peak functions, except the peak locations, which are a direct
  result of their HKLs in combination with the unit cell.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 11/03/2015
*/
class MANTID_CURVEFITTING_DLL PawleyFunction : public API::IPawleyFunction {
public:
  PawleyFunction();

  /// Returns the name of the function.
  std::string name() const override { return "PawleyFunction"; }

  void setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                          double endX) override;

  void setLatticeSystem(const std::string &latticeSystem) override;
  void setProfileFunction(const std::string &profileFunction) override;
  void setUnitCell(const std::string &unitCellString) override;

  void function(const API::FunctionDomain &domain, API::FunctionValues &values) const override;

  /// Derivates are calculated numerically.
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override {
    calNumericalDeriv(domain, jacobian);
  }

  void setPeaks(const std::vector<Kernel::V3D> &hkls, double fwhm, double height) override;
  void clearPeaks() override;

  void addPeak(const Kernel::V3D &hkl, double fwhm, double height) override;
  size_t getPeakCount() const override;
  API::IPeakFunction_sptr getPeakFunction(size_t i) const override;
  Kernel::V3D getPeakHKL(size_t i) const override;

  PawleyParameterFunction_sptr getPawleyParameterFunction() const;

protected:
  void setPeakPositions(const std::string &centreName, double zeroShift, const Geometry::UnitCell &cell) const;

  size_t calculateFunctionValues(const API::IPeakFunction_sptr &peak, const API::FunctionDomain1D &domain,
                                 API::FunctionValues &localValues) const;

  double getTransformedCenter(double d) const;

  void init() override;
  void beforeDecoratedFunctionSet(const API::IFunction_sptr &fn) override;

  API::CompositeFunction_sptr m_compositeFunction;
  PawleyParameterFunction_sptr m_pawleyParameterFunction;
  API::CompositeFunction_sptr m_peakProfileComposite;

  std::vector<Kernel::V3D> m_hkls;

  Kernel::Unit_sptr m_dUnit;
  Kernel::Unit_sptr m_wsUnit;

  int m_peakRadius;
};

using PawleyFunction_sptr = std::shared_ptr<PawleyFunction>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
