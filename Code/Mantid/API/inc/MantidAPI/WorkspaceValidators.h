#ifndef MANTID_API_WORKSPACEVALIDATORS_H_
#define MANTID_API_WORKSPACEVALIDATORS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IValidator.h"
#include "MantidAPI/Workspace.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <numeric>

namespace Mantid
{
namespace API
{

/** A validator for workspaces which can contain a number of individual validators,
    all of which must pass for the overall validator to do so.

    @author Russell Taylor, Tessella Support Services plc
    @date 16/09/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
template <typename TYPE = Workspace>
class DLLExport CompositeValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  CompositeValidator() {}

  virtual ~CompositeValidator()
  {
    for (unsigned int i=0; i < m_children.size(); ++i)
    {
      delete m_children[i];
    }
    m_children.clear();
  }

  // IValidator methods
  const std::string getType() const { return "composite"; }

  /** Checks the value of all child validators. Fails if any one of them does.
   *  @param value The workspace to test
   */
  const bool isValid( const boost::shared_ptr<TYPE>& value ) const
  {
    for (unsigned int i=0; i < m_children.size(); ++i)
    {
      // Return false if any one child validator fails
      if (! m_children[i]->isValid(value) ) return false;
    }
    // All are OK if we get to here
    return true;
  }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone()
  {
    CompositeValidator<TYPE>* copy = new CompositeValidator<TYPE>();
    for (unsigned int i=0; i < m_children.size(); ++i)
    {
      copy->add( m_children[i]->clone() );
    }
    return copy;
  }

  /** Adds a validator to the group of validators to check
   *  @param child A pointer to the validator to add
   */
  void add(Kernel::IValidator<boost::shared_ptr<TYPE> >* child)
  {
    m_children.push_back(child);
  }

private:
  /// Private Copy constructor: NO DIRECT COPY ALLOWED
  CompositeValidator(const CompositeValidator&);

  /// A container for the child validators
  std::vector<Kernel::IValidator<boost::shared_ptr<TYPE> >*> m_children;
};


/** A validator which checks that the unit of the workspace referred to
 *  by a WorkspaceProperty is the expected one.
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
template <typename TYPE = Workspace>
class DLLExport WorkspaceUnitValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  /** Constructor
   *  @param unitID The name of the unit that the workspace must have. If left empty,
   *                the validator will simply check that the workspace is not unitless.
   */
  explicit WorkspaceUnitValidator(const std::string& unitID = "") : m_unitID(unitID) {}

  virtual ~WorkspaceUnitValidator() {}

  const std::string getType() const { return "workspaceunit"; }

  /** Checks the workspace based on the validator's rules
   *  @param value The workspace to test
   */
  const bool isValid( const boost::shared_ptr<TYPE>& value ) const
  {
    Kernel::Unit_const_sptr unit = value->getAxis(0)->unit();
    // If no unit has been given to the validator, just check that the workspace has a unit...
    if ( m_unitID.empty() )
    {
      return ( unit ? true : false );
    }
    // ... otherwise check that the unit is the correct one
    else
    {
      if (!unit) return false;
      return !( unit->unitID().compare(m_unitID) );
    }
  }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new WorkspaceUnitValidator(*this); }

private:
  /// The name of the required unit
  const std::string m_unitID;
};


/** A validator which checks that a workspace contains histogram data (the default)
 *  or point data as required.
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
template <typename TYPE = Workspace>
class DLLExport HistogramValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  /** Constructor
   *  @param mustBeHistogram Flag indicating whether the check is that a workspace should
   *                         contain histogram data (true, default) or shouldn't (false).
   */
  explicit HistogramValidator(const bool& mustBeHistogram = true) :
    m_mustBeHistogram(mustBeHistogram) {}

  virtual ~HistogramValidator() {}

  const std::string getType() const { return "histogram"; }

  /** Checks the workspace based on the validator's rules
   *  @param value The workspace to test
   */
  const bool isValid( const boost::shared_ptr<TYPE>& value ) const
  {
    return ( value->isHistogramData() == m_mustBeHistogram );
  }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new HistogramValidator(*this); }

private:
  /// A flag indicating whether this validator requires that the workspace be a histogram (true) or not
  const bool m_mustBeHistogram;
};

/** A validator which checks that a workspace contains raw counts in its bins
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 16/09/2008
 */
template <typename TYPE = Workspace>
class DLLExport RawCountValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  /** Constructor
   *  @param mustNotBeDistribution Flag indicating whether the check is that a workspace should
   *                               not be a distribution (true, default) or should be (false).
   */
  RawCountValidator(const bool& mustNotBeDistribution = true) :
    m_mustNotBeDistribution(mustNotBeDistribution) {}

  virtual ~RawCountValidator() {}

  const std::string getType() const { return "rawcount"; }

  /** Checks the workspace based on the validator's rules
   *  @param value The workspace to test
   */
  const bool isValid( const boost::shared_ptr<TYPE>& value ) const
  {
    return ( value->isDistribution() != m_mustNotBeDistribution );
  }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new RawCountValidator(*this); }

private:
  /// A flag indicating whether this validator requires that the workspace must be a distribution (false) or not (true, the default)
  const bool m_mustNotBeDistribution;
};

/** A validator which provides a <I>TENTATIVE</I> check that a workspace contains
 *  common bins in each spectrum.
 *  For efficiency reasons, it only checks that the first and last spectra have
 *  common bins, so it is important to carry out a full check within the algorithm
 *  itself.
 *
 *  @author Russell Taylor, Tessella Support Services plc
 *  @date 18/09/2008
 */
template <typename TYPE = Workspace>
class DLLExport CommonBinsValidator : public Kernel::IValidator<boost::shared_ptr<TYPE> >
{
public:
  CommonBinsValidator() {}

  virtual ~CommonBinsValidator() {}

  const std::string getType() const { return "commonbins"; }

  /** Checks the workspace based on the validator's rules
   *  @param value The workspace to test
   */
  const bool isValid( const boost::shared_ptr<TYPE>& value ) const
  {
    if ( !value->blocksize() || value->getNumberHistograms() < 2) return true;
    const int lastSpec = value->getNumberHistograms() - 1;
    // Quickest check is to see if they are actually the same vector
    if ( &(value->readX(0)[0]) == &(value->readX(lastSpec)[0]) ) return true;
    // Now check numerically
    const double first = std::accumulate(value->readX(0).begin(),value->readX(0).end(),0.);
    const double last = std::accumulate(value->readX(lastSpec).begin(),value->readX(lastSpec).end(),0.);
    if ( std::abs(first-last) > 1.0E-9 ) return false;
    return true;
  }

  Kernel::IValidator<boost::shared_ptr<TYPE> >* clone() { return new CommonBinsValidator(*this); }
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACEVALIDATORS_H_ */
