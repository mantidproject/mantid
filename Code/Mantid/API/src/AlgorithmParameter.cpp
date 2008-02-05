//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmParameter.h"

namespace Mantid
{
namespace API
{
/// default constructor
  AlgorithmParameter::AlgorithmParameter()
    :
  // strings have their own default constructor
  m_isDefault(false)
{}

/// Constructor
  AlgorithmParameter::AlgorithmParameter(const std::string& name, const std::string& value,
    const std::string& type, const bool& isdefault, const unsigned int& direction)
    :
  m_name(name),m_value(value),m_type(type),m_isDefault(isdefault),m_direction(direction)
{
}

/// Destructor
AlgorithmParameter::~AlgorithmParameter()
{
}

/*!
  Standard Copy Constructor
  \param A :: AlgorithmParameter Item to copy
*/

AlgorithmParameter::AlgorithmParameter(const AlgorithmParameter& A)
:
  m_name(A.m_name),m_value(A.m_value),
    m_type(A.m_type),m_isDefault(A.m_isDefault),m_direction(A.m_direction)
{}

/*!
  Standard Assignment operator
  \param A :: AlgorithmParameter Item to assign to 'this'
*/
AlgorithmParameter& AlgorithmParameter::operator=(const AlgorithmParameter& A)
{
  if (this!=&A)
    {
       m_name=A.m_name;
       m_value=A.m_value;
       m_type=A.m_type;
       m_isDefault=A.m_isDefault;
       m_direction=A.m_direction;
    }
   return *this;
}

} // namespace API
} // namespace Mantid
