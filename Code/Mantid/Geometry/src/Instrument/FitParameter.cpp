//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterFactory.h"


namespace Mantid
{
namespace Geometry
{


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
    in >> f.setValue();

    if ( in.fail() == true )
    {
      f.setValue() = 0.0;
      f.setTie() = "";
      return in;
    }

    in >> f.setTie();

    if ( in.fail() == true )
    {
      f.setTie() = "";
    }

    return in;
  }

} // namespace Geometry
} // namespace Mantid
