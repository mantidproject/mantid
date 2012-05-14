#ifndef MANTID_CURVEFITTING_SEQDOMAIN_H_
#define MANTID_CURVEFITTING_SEQDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"

#include "MantidCurveFitting/IDomainCreator.h"

#include <stdexcept>
#include <vector>
#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{
/** An implementation of CompositeDomain.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_CURVEFITTING_DLL SeqDomain: public API::FunctionDomain
{
public:
  SeqDomain():API::FunctionDomain(),m_currentIndex(0){}
  /// Return the number of points in the domain
  virtual size_t size() const;
  /// Return the number of parts in the domain
  virtual size_t getNDomains() const;
  /// Create and return i-th domain and i-th values, (i-1)th domain is released.
  virtual void getDomainAndValues(size_t i, API::FunctionDomain_sptr& domain, API::IFunctionValues_sptr& values) const;
  /// Add new domain creator
  void addCreator( IDomainCreator_sptr creator );
protected:
  /// Current index
  mutable size_t m_currentIndex;
  /// Currently active domain.
  mutable API::FunctionDomain_sptr m_domain;
  /// Currently active values.
  mutable API::IFunctionValues_sptr m_values;
  /// Domain creators.
  std::vector< boost::shared_ptr<IDomainCreator> > m_creators;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_SEQDOMAIN_H_*/
