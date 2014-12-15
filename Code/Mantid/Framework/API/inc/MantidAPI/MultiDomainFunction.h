#ifndef MANTID_API_MULTIDOMAINFUNCTION_H_
#define MANTID_API_MULTIDOMAINFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"

#include <map>

namespace Mantid
{
namespace API
{
  class CompositeDomain;
/** A composite function defined on a CompositeDomain. Member functions can be applied to 
    one or more member domains of the CompositeDomain. If two functions applied to the same domain
    the results are added (+).

    @author Roman Tolchenov, Tessella plc
    @date 13/03/2012

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL MultiDomainFunction : public CompositeFunction
{
public:
  /// Constructor
  MultiDomainFunction():m_nDomains(0),m_maxIndex(0){}

  /// Returns the function's name
  virtual std::string name()const {return "MultiDomainFunction";}
  /// Function you want to fit to. 
  /// @param domain :: The input domain over which the function is to be calculated
  /// @param values :: A storage object for the calculated values
  virtual void function(const FunctionDomain& domain, FunctionValues& values)const;
  /// Derivatives of function with respect to active parameters
  virtual void functionDeriv(const FunctionDomain& domain, Jacobian& jacobian);
  /// Called at the start of each iteration
  virtual void iterationStarting();
  /// Called at the end of an iteration
  virtual void iterationFinished();
  /// Create a list of equivalent functions
  virtual std::vector<IFunction_sptr> createEquivalentFunctions() const;

  /// Associate a function and a domain
  void setDomainIndex(size_t funIndex, size_t domainIndex);
  /// Associate a function and a list of domains
  void setDomainIndices(size_t funIndex, const std::vector<size_t>& domainIndices);
  /// Clear all domain indices
  void clearDomainIndices();
  /// Get the largest domain index
  size_t getMaxIndex() const {return m_maxIndex;}
  /// Get domain indices for a member function
  void getDomainIndices(size_t i, size_t nDomains, std::vector<size_t>& domains)const;

  /// Returns the number of "local" attributes associated with the function.
  /// Local attributes are attributes of MultiDomainFunction but describe properties
  /// of individual member functions.
  virtual size_t nLocalAttributes()const {return 1;}
  /// Returns a list of attribute names
  virtual std::vector<std::string> getLocalAttributeNames()const {return std::vector<std::string>(1,"domains");}
  /// Return a value of attribute attName
  virtual Attribute getLocalAttribute(size_t i, const std::string& attName)const;
  /// Set a value to attribute attName
  virtual void setLocalAttribute(size_t i, const std::string& attName,const Attribute& );
  /// Check if attribute attName exists
  virtual bool hasLocalAttribute(const std::string& attName)const {return attName == "domains";}

protected:

  /// Counts number of the domains
  void countNumberOfDomains();
  void countValueOffsets(const CompositeDomain& domain)const;

  /// Domain index map: finction -> domain
  std::map<size_t, std::vector<size_t> > m_domains;
  /// Number of domains this MultiDomainFunction operates on. == number of different values in m_domains
  size_t m_nDomains;
  /// Maximum domain index
  size_t m_maxIndex;
  mutable std::vector<size_t> m_valueOffsets;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_MULTIDOMAINFUNCTION_H_*/
