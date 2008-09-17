//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid
{
namespace API
{

//----------------------------------------------------------------------
// CompositeValidator
//----------------------------------------------------------------------

CompositeValidator::CompositeValidator() {}

CompositeValidator::~CompositeValidator()
{
  std::vector<Kernel::IValidator<boost::shared_ptr<Workspace> >*>::iterator it;
  for (it = m_children.begin(); it != m_children.end(); ++it)
  {
    delete *it;
  }
  m_children.clear();
}

/** Checks the value of all child validators. Fails if any one of them does.
 *  @param value The workspace to test
 */
const bool CompositeValidator::isValid( const boost::shared_ptr<Workspace> &value ) const
{
  std::vector<Kernel::IValidator<boost::shared_ptr<Workspace> >*>::const_iterator it;
  for (it = m_children.begin(); it != m_children.end(); ++it)
  {
    // Return false if any one child validator fails
    if (! (*it)->isValid(value) ) return false;
  }
  // All are OK if we get to here
  return true;
}

Kernel::IValidator<boost::shared_ptr<Workspace> >* CompositeValidator::clone()
{
  CompositeValidator* copy = new CompositeValidator();
  std::vector<Kernel::IValidator<boost::shared_ptr<Workspace> >*>::const_iterator it;
  for (it = m_children.begin(); it != m_children.end(); ++it)
  {
    copy->add( (*it)->clone() );
  }
  return copy;
}

/** Adds a validator to the group of validators to check
 *  @param child A pointer to the validator to add
 */
void CompositeValidator::add(Kernel::IValidator<boost::shared_ptr<Workspace> >* child)
{
  m_children.push_back(child);
}

//----------------------------------------------------------------------
// WorkspaceUnitValidator
//----------------------------------------------------------------------
/** Constructor
 *  @param unitID The name of the unit that the workspace must have. If left empty,
 *                the validator will simply check that the workspace is not unitless.
 */
WorkspaceUnitValidator::WorkspaceUnitValidator(const std::string& unitID) : m_unitID(unitID)
{}

/** Checks the workspace based on the validator's rules
 *  @param value The workspace to test
 */
const bool WorkspaceUnitValidator::isValid( const boost::shared_ptr<Workspace> &value ) const
{
  boost::shared_ptr<Kernel::Unit> unit = value->getAxis(0)->unit();
  // If no unit has been given to the validator, just check that the workspace has a unit...
  if ( m_unitID.empty() )
  {
    return ( unit ? true : false );
  }
  // ... otherwise check that the unit is the correct one
  else
  {
    return !( unit->unitID().compare(m_unitID) );
  }
}

//----------------------------------------------------------------------
// HistogramValidator
//----------------------------------------------------------------------
/** Constructor
 *  @param mustBeHistogram Flag indicating whether the check is that a workspace should
 *                         contain workspace data (true, default) or shouldn't (false).
 */
HistogramValidator::HistogramValidator(const bool& mustBeHistogram) :
  m_mustBeHistogram(mustBeHistogram)
{}

/** Checks the workspace based on the validator's rules
 *  @param value The workspace to test
 */
const bool HistogramValidator::isValid( const boost::shared_ptr<Workspace> &value ) const
{
  if ( value->dataX(0).size() == value->dataY(0).size() )
  {
    return ( m_mustBeHistogram ? false : true );
  }
  else
  {
    return ( m_mustBeHistogram ? true : false );
  }
}

//----------------------------------------------------------------------
// RawCountValidator
//----------------------------------------------------------------------
/** Checks the workspace based on the validator's rules
 *  @param value The workspace to test
 */
const bool RawCountValidator::isValid( const boost::shared_ptr<Workspace> &value ) const
{
  return !( value->isDistribution() );
}


} // namespace API
} // namespace Mantid
