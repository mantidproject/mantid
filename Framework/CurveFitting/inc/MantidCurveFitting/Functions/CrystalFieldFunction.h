// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/EigenFortranDefs.h"
#include "MantidCurveFitting/Functions/CrystalFieldControl.h"

#include <unordered_map>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Calculates crystal field spectra.
*/
class MANTID_CURVEFITTING_DLL CrystalFieldFunction : public API::IFunction {
public:
  CrystalFieldFunction();
  std::string name() const override { return "CrystalFieldFunction"; }
  const std::string category() const { return "General"; }
  size_t getNumberDomains() const override;
  std::vector<API::IFunction_sptr> createEquivalentFunctions() const override;
  /// Evaluate the function
  void function(const API::FunctionDomain &domain, API::FunctionValues &values) const override;

  //@ Parameters
  //@{
  /// Set i-th parameter
  void setParameter(size_t, const double &value, bool explicitlySet = true) override;
  /// Set i-th parameter description
  void setParameterDescription(size_t, const std::string &description) override;
  /// Get i-th parameter
  double getParameter(size_t i) const override;
  /// Set parameter by name.
  void setParameter(const std::string &name, const double &value, bool explicitlySet = true) override;
  /// Set description of parameter by name.
  void setParameterDescription(const std::string &name, const std::string &description) override;
  /// Get parameter by name.
  double getParameter(const std::string &name) const override;
  /// Check if function has a parameter with this name.
  bool hasParameter(const std::string &name) const override;
  /// Total number of parameters
  size_t nParams() const override;
  /// Returns the index of parameter name
  size_t parameterIndex(const std::string &name) const override;
  /// Returns the name of parameter i
  std::string parameterName(size_t i) const override;
  /// Returns the description of parameter i
  std::string parameterDescription(size_t i) const override;
  /// Checks if a parameter has been set explicitly
  bool isExplicitlySet(size_t i) const override;
  /// Get the fitting error for a parameter
  double getError(size_t i) const override;
  /// Get the fitting error for a parameter by name
  double getError(const std::string &name) const override;
  /// Set the fitting error for a parameter
  void setError(size_t i, double err) override;
  /// Set the fitting error for a parameter by name
  void setError(const std::string &name, double err) override;

  /// Return parameter index from a parameter reference.
  size_t getParameterIndex(const API::ParameterReference &ref) const override;
  /// Set up the function for a fit.
  void setUpForFit() override;
  /// Get the tie for i-th parameter
  API::ParameterTie *getTie(size_t i) const override;
  /// Checks if whether tie should be ignored
  bool ignoreTie(const API::ParameterTie &tie) const override;
  /// Get the i-th constraint
  API::IConstraint *getConstraint(size_t i) const override;
  //@}

  /** @name Attributes */
  //@{
  /// Returns the number of attributes associated with the function
  size_t nAttributes() const override;
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;
  /// Return a value of attribute attName
  Attribute getAttribute(const std::string &name) const override;
  /// Set a value to attribute attName
  void setAttribute(const std::string &name, const Attribute &) override;
  /// Check if attribute attName exists
  bool hasAttribute(const std::string &name) const override;
  //@}

  /// @name Checks
  //@{
  /// Check if the function is set up for a multi-site calculations.
  /// (Multiple ions defined)
  bool isMultiSite() const;
  /// Check if the function is set up for a multi-spectrum calculations
  /// (Multiple temperatures defined)
  bool isMultiSpectrum() const;
  /// Check if the spectra have a background.
  bool hasBackground() const;
  /// Check if there are peaks (there is at least one spectrum).
  bool hasPeaks() const;
  /// Check if there are any phys. properties.
  bool hasPhysProperties() const;
  //@}

  /// Build source function if necessary.
  void checkSourceFunction() const;
  /// Build target function.
  void buildTargetFunction() const;
  /// Get number of the number of spectra (excluding phys prop data).
  size_t nSpectra() const;

protected:
  /// Declare a new parameter
  void declareParameter(const std::string &name, double initValue = 0, const std::string &description = "") override;
  /// Change status of parameter
  void setParameterStatus(size_t i, ParameterStatus status) override;
  /// Get status of parameter
  ParameterStatus getParameterStatus(size_t i) const override;

  /// Build the source function
  void buildSourceFunction() const;
  /// Update the target function
  void updateTargetFunction() const;

private:
  /// Build the target function in a single site case.
  void buildSingleSite() const;
  /// Build the target function in a multi site case.
  void buildMultiSite() const;
  /// Build the target function in a single site - single spectrum case.
  void buildSingleSiteSingleSpectrum() const;
  /// Build the target function in a single site - multi spectrum case.
  void buildSingleSiteMultiSpectrum() const;
  /// Build the target function in a multi site - single spectrum case.
  void buildMultiSiteSingleSpectrum() const;
  /// Build the target function in a multi site - multi spectrum case.
  void buildMultiSiteMultiSpectrum() const;

  /// Update the target function in a single site case.
  void updateSingleSite() const;
  /// Update the target function in a multi site case.
  void updateMultiSite() const;
  /// Update the target function in a single site - single spectrum case.
  void updateSingleSiteSingleSpectrum() const;
  /// Update the target function in a single site - multi spectrum case.
  void updateSingleSiteMultiSpectrum() const;
  /// Update the target function in a multi site - single spectrum case.
  void updateMultiSiteSingleSpectrum() const;
  /// Update the target function in a multi site - multi spectrum case.
  void updateMultiSiteMultiSpectrum() const;

  /// Build a function for a single spectrum.
  API::IFunction_sptr buildSpectrum(int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
                                    double temperature, double fwhm, size_t i, bool addBackground,
                                    double intensityScaling) const;
  /// Update a function for a single spectrum.
  void updateSpectrum(API::IFunction &spectrum, int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
                      double temperature, double fwhm, size_t iSpec, size_t iFirst, double intensityScaling) const;
  /// Calculate excitations at given temperature
  void calcExcitations(int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf, double temperature,
                       API::FunctionValues &values, double intensityScaling) const;
  /// Build a physical property function.
  API::IFunction_sptr buildPhysprop(int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
                                    const ComplexFortranMatrix &ham, const std::string &propName) const;
  /// Update a physical property function.
  void updatePhysprop(int nre, const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
                      const ComplexFortranMatrix &ham, API::IFunction &fun) const;

  /// Set the source function
  void setSource(API::IFunction_sptr source) const;
  /// Update target function if necessary.
  void checkTargetFunction() const;
  /// Get a reference to the source function if it's composite
  API::CompositeFunction &compositeSource() const;

  /// Get a reference to an attribute
  std::pair<API::IFunction *, std::string> getAttributeReference(const std::string &attName) const;
  /// Build and cache the attribute names
  void buildAttributeNames() const;

  /// Make maps between parameter names and indices
  void makeMaps() const;
  void makeMapsSingleSiteSingleSpectrum() const;
  void makeMapsSingleSiteMultiSpectrum() const;
  void makeMapsMultiSiteSingleSpectrum() const;
  void makeMapsMultiSiteMultiSpectrum() const;
  size_t makeMapsForFunction(const IFunction &fun, size_t iFirst, const std::string &prefix) const;
  void cacheSourceParameters() const;
  /// Function that creates the source function.
  mutable CrystalFieldControl m_control;
  /// Function that calculates parameters of the target function.
  mutable API::IFunction_sptr m_source;
  /// Function that actually calculates the output.
  mutable API::CompositeFunction_sptr m_target;
  /// Cached number of parameters in m_control.
  mutable size_t m_nControlParams;
  /// Cached number of parameters in m_control and m_source.
  mutable size_t m_nControlSourceParams;
  /// Flag indicating that updateTargetFunction() is required.
  mutable bool m_dirtyTarget;
  /// Map parameter names to indices
  mutable std::unordered_map<std::string, size_t> m_mapNames2Indices;
  /// Map parameter indices to names
  mutable std::vector<std::string> m_mapIndices2Names;
  /// Attribute names
  mutable std::vector<std::string> m_attributeNames;
  /// Map parameter/attribute prefixes to pointers to phys prop functions
  mutable std::unordered_map<std::string, API::IFunction_sptr> m_mapPrefixes2PhysProps;
  /// Temporary cache for parameter values during source function resetting.
  mutable std::vector<double> m_parameterResetCache;
  mutable std::vector<bool> m_fixResetCache;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
