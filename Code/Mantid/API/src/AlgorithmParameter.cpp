//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmParameter.h"
#include "MantidAPI/WorkspaceProperty.h"


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

    /** Prints a text representation of itself
    * @param os The ouput stream to write to
	* @param indent an indentation value to make pretty printing of object and sub-objects
    */
    void AlgorithmParameter::printSelf(std::ostream& os, const int indent) const
    {
      os << std::string(indent,' ') << "Name : " << m_name << std::endl;
      os << std::string(indent,' ') << "Value: " << m_value << std::endl;
      os << std::string(indent,' ') << "Type: " << m_type << std::endl;
      os << std::string(indent,' ') << "isDefault: "<< m_isDefault << std::endl; 
      os << std::string(indent,' ') << "Direction :" << getdirectiontext() << std::endl;
    }
    const std::string AlgorithmParameter::getdirectiontext()const
    { 
      if(m_direction == Mantid::Kernel::Direction::Input) return "Input";
      if(m_direction == Mantid::Kernel::Direction::Output) return "Output";
      if(m_direction == Mantid::Kernel::Direction::InOut) return "Inout";
      if(m_direction == Mantid::Kernel::Direction::None) return "None";
      throw std::invalid_argument("Unknown Mantid::Kernel::Direction defined");
    }

    /** Prints a text representation
    * @param os The ouput stream to write to
    * @param AP The AlgorithmParameter to output
    * @returns The ouput stream
    */
    std::ostream& operator<<(std::ostream& os, const AlgorithmParameter& AP)
    {
      AP.printSelf(os);
      return os;
    }
  
  } // namespace API
} // namespace Mantid
