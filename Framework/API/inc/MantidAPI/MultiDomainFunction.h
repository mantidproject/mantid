// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"

#include <map>

namespace Mantid {
namespace API {
class CompositeDomain;
/** A composite function defined on a CompositeDomain. Member functions can be
   applied to
    one or more member domains of the CompositeDomain. If two functions applied
   to the same domain
    the results are added (+).

    @author Roman Tolchenov, Tessella plc
    @date 13/03/2012
*/
class MANTID_API_DLL MultiDomainFunction : public CompositeFunction {
public:
  /// Constructor
  MultiDomainFunction();

  /// Returns the function's name
  std::string name() const override { return "MultiDomainFunction"; }
  /// Function you want to fit to.
  /// @param domain :: The input domain over which the function is to be
  /// calculated
  /// @param values :: A storage object for the calculated values
  void function(const FunctionDomain &domain, FunctionValues &values) const override;
  /// Derivatives of function with respect to active parameters
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) override;
  /// Called at the start of each iteration
  void iterationStarting() override;
  /// Called at the end of an iteration
  void iterationFinished() override;

  /// Associate a function and a domain
  void setDomainIndex(size_t funIndex, size_t domainIndex);
  /// Associate a function and a list of domains
  void setDomainIndices(size_t funIndex, const std::vector<size_t> &domainIndices);
  /// Clear all domain indices
  void clearDomainIndices();
  /// Get the largest domain index
  size_t getMaxIndex() const { return m_maxIndex; }
  /// Get domain indices for a member function
  void getDomainIndices(size_t i, size_t nDomains, std::vector<size_t> &domains) const;
  /// Get number of domains required by this function
  size_t getNumberDomains() const override;
  /// Create a list of equivalent functions
  std::vector<IFunction_sptr> createEquivalentFunctions() const override;

  /// Returns the number of "local" attributes associated with the function.
  /// Local attributes are attributes of MultiDomainFunction but describe
  /// properties
  /// of individual member functions.
  size_t nLocalAttributes() const override { return 1; }
  /// Returns a list of attribute names
  std::vector<std::string> getLocalAttributeNames() const override { return std::vector<std::string>(1, "domains"); }
  /// Return a value of attribute attName
  Attribute getLocalAttribute(size_t i, const std::string &attName) const override;
  /// Set a value to attribute attName
  void setLocalAttribute(size_t i, const std::string &attName, const Attribute &) override;
  /// Check if attribute attName exists
  bool hasLocalAttribute(const std::string &attName) const override { return attName == "domains"; }

protected:
  /// Counts number of the domains
  void countNumberOfDomains();
  void countValueOffsets(const CompositeDomain &domain) const;

  /// Domain index map: finction -> domain
  std::map<size_t, std::vector<size_t>> m_domains;
  /// Number of domains this MultiDomainFunction operates on. == number of
  /// different values in m_domains
  size_t m_nDomains;
  /// Maximum domain index
  size_t m_maxIndex;
  mutable std::vector<size_t> m_valueOffsets;
};

} // namespace API
} // namespace Mantid
