//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"
#include <Poco/StringTokenizer.h>


namespace Mantid
{
namespace Geometry
{

  /**
    Get parameter value. The default parameter 'at' is ignored expect if
    the value of the parameter is determined from a look up table etc.
    @param at number to return the value at
  */
  double FitParameter::getValue(const double& at) const 
  { 
    if ( m_lookUpTable.containData() )
    {
      return  m_lookUpTable.value(at);
    }

    return m_value;
  }

  /**
    Get parameter value.
  */
  double FitParameter::getValue() const 
  { 
    return m_value;
  }

  /**
    Prints object to stream
    @param os the Stream to output to
  */
  void FitParameter::printSelf(std::ostream& os) const
  {
    os << m_value;
    return;
  }

  /**
    Prints the value of parameter
    @param os the Stream to output to
    @param f the FitParameter to output
    @return the output stream
    */
  std::ostream& operator<<(std::ostream& os, const FitParameter& f)
  {
    f.printSelf(os);
    return os;
  }

  /**
    Reads in parameter value
    @param in Input Stream
    @param f FitParameter to write to
    @return Current state of stream
  */
  std::istream& operator>>(std::istream& in, FitParameter& f)
  {

    typedef Poco::StringTokenizer tokenizer;
    std::string str;
    getline(in, str);
    tokenizer values(str, ",", tokenizer::TOK_TRIM);

    try
    {
      f.setValue() = atof(values[0].c_str());
    }
    catch (...)
    {
      f.setValue() = 0.0;
    }

    f.setFunction() = values[1];

    f.setConstraint() = values[2];

    f.setTie() = values[3];

    if ( values.count() > 4 )
    {
      std::stringstream str(values[4]);
      str >> f.setLookUpTable();
    }

    return in;
  }

} // namespace Geometry
} // namespace Mantid
