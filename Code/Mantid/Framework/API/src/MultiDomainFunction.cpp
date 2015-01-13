//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/Expression.h"

#include <boost/lexical_cast.hpp>
#include <set>

namespace Mantid {
namespace API {

DECLARE_FUNCTION(MultiDomainFunction)

/**
 * Associate a member function and a domain. The function will only be applied
 * to this domain.
 * @param funIndex :: Index of a member function.
 * @param domainIndex :: Index of a domain to be associated with the function.
 */
void MultiDomainFunction::setDomainIndex(size_t funIndex, size_t domainIndex) {
  m_domains[funIndex] = std::vector<size_t>(1, domainIndex);
  countNumberOfDomains();
}

/**
 * Associate a member function and a list of domains. The function will only be
 * applied
 * to the listed domains.
 * @param funIndex :: Index of a member function.
 * @param domainIndices :: A vector with indices of domains to be associated
 * with the function.
 */
void MultiDomainFunction::setDomainIndices(
    size_t funIndex, const std::vector<size_t> &domainIndices) {
  m_domains[funIndex] = domainIndices;
  countNumberOfDomains();
}

/**
 * Counts number of the domains.
 */
void MultiDomainFunction::countNumberOfDomains() {
  std::set<size_t> dSet;
  for (auto it = m_domains.begin(); it != m_domains.end(); ++it) {
    if (it->second.size()) {
      dSet.insert(it->second.begin(), it->second.end());
    }
  }
  m_nDomains = dSet.size();
  m_maxIndex = dSet.empty() ? 0 : *dSet.rbegin();
}

/**
 * Count value offsets for each member domain in a CompositeDomain.
 */
void
MultiDomainFunction::countValueOffsets(const CompositeDomain &domain) const {
  m_valueOffsets.clear();
  m_valueOffsets.push_back(0);
  for (size_t i = 0; i < domain.getNParts(); ++i) {
    const FunctionDomain &d = domain.getDomain(i);
    m_valueOffsets.push_back(m_valueOffsets.back() + d.size());
  }
}

/// Clear all domain indices
void MultiDomainFunction::clearDomainIndices() {
  m_domains.clear();
  countNumberOfDomains();
}

/**
 * Populates a vector with domain indices assigned to function i.
 * @param i :: Index of a function to get the domain info about.
 * @param nDomains :: Maximum number of domains.
 * @param domains :: (Output) vector to collect domain indixes.
 */
void MultiDomainFunction::getDomainIndices(size_t i, size_t nDomains,
                                           std::vector<size_t> &domains) const {
  auto it = m_domains.find(i);
  if (it == m_domains.end()) { // apply to all domains
    domains.resize(nDomains);
    for (size_t i = 0; i < domains.size(); ++i) {
      domains[i] = i;
    }
  } else { // apply to selected domains
    domains.assign(it->second.begin(), it->second.end());
  }
}

/// Function you want to fit to.
/// @param domain :: The buffer for writing the calculated values. Must be big
/// enough to accept dataSize() values
void MultiDomainFunction::function(const FunctionDomain &domain,
                                   FunctionValues &values) const {
  // works only on CompositeDomain
  if (!dynamic_cast<const CompositeDomain *>(&domain)) {
    throw std::invalid_argument(
        "Non-CompositeDomain passed to MultiDomainFunction.");
  }
  const CompositeDomain &cd = dynamic_cast<const CompositeDomain &>(domain);
  // domain must not have less parts than m_maxIndex
  if (cd.getNParts() <= m_maxIndex) {
    throw std::invalid_argument(
        "CompositeDomain has too few parts (" +
        boost::lexical_cast<std::string>(cd.getNParts()) +
        ") for MultiDomainFunction (max index " +
        boost::lexical_cast<std::string>(m_maxIndex) + ").");
  }
  // domain and values must be consistent
  if (cd.size() != values.size()) {
    throw std::invalid_argument(
        "MultiDomainFunction: domain and values have different sizes.");
  }

  countValueOffsets(cd);
  // evaluate member functions
  values.zeroCalculated();
  for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
    // find the domains member function must be applied to
    std::vector<size_t> domains;
    getDomainIndices(iFun, cd.getNParts(), domains);

    for (auto i = domains.begin(); i != domains.end(); ++i) {
      const FunctionDomain &d = cd.getDomain(*i);
      FunctionValues tmp(d);
      getFunction(iFun)->function(d, tmp);
      values.addToCalculated(m_valueOffsets[*i], tmp);
    }
  }
}

/// Derivatives of function with respect to active parameters
void MultiDomainFunction::functionDeriv(const FunctionDomain &domain,
                                        Jacobian &jacobian) {
  // works only on CompositeDomain
  if (!dynamic_cast<const CompositeDomain *>(&domain)) {
    throw std::invalid_argument(
        "Non-CompositeDomain passed to MultiDomainFunction.");
  }
  const CompositeDomain &cd = dynamic_cast<const CompositeDomain &>(domain);
  // domain must not have less parts than m_maxIndex
  if (cd.getNParts() < m_maxIndex) {
    throw std::invalid_argument(
        "CompositeDomain has too few parts (" +
        boost::lexical_cast<std::string>(cd.getNParts()) +
        ") for MultiDomainFunction (max index " +
        boost::lexical_cast<std::string>(m_maxIndex) + ").");
  }

  countValueOffsets(cd);
  // evaluate member functions derivatives
  for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
    // find the domains member function must be applied to
    std::vector<size_t> domains;
    getDomainIndices(iFun, cd.getNParts(), domains);

    for (auto i = domains.begin(); i != domains.end(); ++i) {
      const FunctionDomain &d = cd.getDomain(*i);
      PartialJacobian J(&jacobian, m_valueOffsets[*i], paramOffset(iFun));
      getFunction(iFun)->functionDeriv(d, J);
    }
  }
}

/**
 * Called at the start of each iteration. Call iterationStarting() of the
 * members.
 */
void MultiDomainFunction::iterationStarting() {
  for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
    getFunction(iFun)->iterationStarting();
  }
}

/**
 * Called at the end of an iteration. Call iterationFinished() of the members.
 */
void MultiDomainFunction::iterationFinished() {
  for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
    getFunction(iFun)->iterationFinished();
  }
}

/// Return a value of attribute attName
IFunction::Attribute
MultiDomainFunction::getLocalAttribute(size_t i,
                                       const std::string &attName) const {
  if (attName != "domains") {
    throw std::invalid_argument("MultiDomainFunction does not have attribute " +
                                attName);
  }
  if (i >= nFunctions()) {
    throw std::out_of_range("Function index is out of range.");
  }
  auto it = m_domains.find(i);
  if (it == m_domains.end()) {
    return IFunction::Attribute("All");
  } else if (it->second.size() == 1 && it->second.front() == i) {
    return IFunction::Attribute("i");
  } else if (!it->second.empty()) {
    std::string out(boost::lexical_cast<std::string>(it->second.front()));
    for (auto i = it->second.begin() + 1; i != it->second.end(); ++it) {
      out += "," + boost::lexical_cast<std::string>(*i);
    }
    return IFunction::Attribute(out);
  }
  return IFunction::Attribute("");
}

/**
 * Set a value to a "local" attribute, ie an attribute related to a member
 *function.
 *
 * The only attribute that can be set here is "domains" which defines the
 * indices of domains a particular function is applied to. Possible values are
 *(strings):
 *
 *     1) "All" : the function is applied to all domains defined for this
 *MultiDomainFunction.
 *     2) "i"   : the function is applied to a single domain which index is
 *equal to the
 *                function's index in this MultiDomainFunction.
 *     3) "non-negative integer" : a domain index.
 *     4) "a,b,c,..." : a list of domain indices (a,b,c,.. are non-negative
 *integers).
 *     5) "a - b" : a range of domain indices (a,b are non-negative integers a
 *<= b).
 *
 * To be used with Fit algorithm at least one of the member functions must have
 *"domains" value
 * of type 2), 3), 4) or 5) because these values can tell Fit how many domains
 *need to be created.
 *
 * @param i :: Index of a function for which the attribute is being set.
 * @param attName :: Name of an attribute.
 * @param att :: Value of the attribute to set.
 */
void MultiDomainFunction::setLocalAttribute(size_t i,
                                            const std::string &attName,
                                            const IFunction::Attribute &att) {
  if (attName != "domains") {
    throw std::invalid_argument("MultiDomainFunction does not have attribute " +
                                attName);
  }
  if (i >= nFunctions()) {
    throw std::out_of_range("Function index is out of range.");
  }
  std::string value = att.asString();
  auto it = m_domains.find(i);

  if (value == "All") { // fit to all domains
    if (it != m_domains.end()) {
      m_domains.erase(it);
    }
    return;
  } else if (value ==
             "i") { // fit to domain with the same index as the function
    setDomainIndex(i, i);
    return;
  } else if (value.empty()) { // do not fit to any domain
    setDomainIndices(i, std::vector<size_t>());
    return;
  }

  // fit to a selection of domains
  std::vector<size_t> indx;
  Expression list;
  list.parse(value);
  if (list.name() == "+") {
    if (list.size() != 2 || list.terms()[1].operator_name() != "-") {
      throw std::runtime_error("MultiDomainFunction: attribute \"domains\" "
                               "expects two integers separated by a \"-\"");
    }
    // value looks like "a - b". a and b must be ints and define a range of
    // domain indices
    size_t start = boost::lexical_cast<size_t>(list.terms()[0].str());
    size_t end = boost::lexical_cast<size_t>(list.terms()[1].str()) + 1;
    if (start >= end) {
      throw std::runtime_error(
          "MultiDomainFunction: attribute \"domains\": wrong range limits.");
    }
    indx.resize(end - start);
    for (size_t i = start; i < end; ++i) {
      indx[i - start] = i;
    }
  } else {
    // value must be either an int or a list of ints: "a,b,c,..."
    list.toList();
    for (size_t k = 0; k < list.size(); ++k) {
      indx.push_back(boost::lexical_cast<size_t>(list[k].name()));
    }
  }
  setDomainIndices(i, indx);
}

/**
 * Split this function into independent functions. The number of functions in
 * the
 * returned vector must be equal to the number
 * of domains. The result of evaluation of the i-th function on the i-th domain
 * must be
 * the same as if this MultiDomainFunction was evaluated.
 */
std::vector<IFunction_sptr>
MultiDomainFunction::createEquivalentFunctions() const {
  size_t nDomains = m_maxIndex + 1;
  std::vector<CompositeFunction_sptr> compositeFunctions(nDomains);
  for (size_t iFun = 0; iFun < nFunctions(); ++iFun) {
    // find the domains member function must be applied to
    std::vector<size_t> domains;
    getDomainIndices(iFun, nDomains, domains);

    for (auto i = domains.begin(); i != domains.end(); ++i) {
      size_t j = *i;
      CompositeFunction_sptr cf = compositeFunctions[j];
      if (!cf) {
        // create a composite function for each domain
        cf = CompositeFunction_sptr(new CompositeFunction());
        compositeFunctions[j] = cf;
      }
      // add copies of all functions applied to j-th domain to a single
      // compositefunction
      cf->addFunction(FunctionFactory::Instance().createInitialized(
          getFunction(iFun)->asString()));
    }
  }
  std::vector<IFunction_sptr> outFunctions(nDomains);
  // fill in the output vector
  // check functions containing a single member and take it out of the composite
  for (size_t i = 0; i < compositeFunctions.size(); ++i) {
    auto fun = compositeFunctions[i];
    if (!fun || fun->nFunctions() == 0) {
      throw std::runtime_error("There is no function for domain " +
                               boost::lexical_cast<std::string>(i));
    }
    if (fun->nFunctions() > 1) {
      outFunctions[i] = fun;
    } else {
      outFunctions[i] = fun->getFunction(0);
    }
  }
  return outFunctions;
}

} // namespace API
} // namespace Mantid
