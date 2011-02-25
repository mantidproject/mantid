#include "MantidMDEvents/Dimension.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace MDEvents
{


//----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Dimension::Dimension()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param min :: min extent
   * @param max :: max extent
   * @param name :: dimension name
   * @param units :: units for this dimension
   */
  Dimension::Dimension(CoordType min, CoordType max, std::string name, std::string units)
  : min(min),max(max),name(name),units(units)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Dimension::~Dimension()
  {
  }

  /** Get the max extent of this dimension */
  CoordType Dimension::getMax() const
  {
      return max;
  }

  /** Get the min extent of this dimension */
  CoordType Dimension::getMin() const
  {
      return min;
  }

  /** Get the name of this dimension */
  std::string Dimension::getName() const
  {
      return name;
  }

  /** Get the units of this dimension */
  std::string Dimension::getUnits() const
  {
      return units;
  }

  /** Set the maximum extent of this dimension
   * @param max :: value */
  void Dimension::setMax(CoordType max)
  {
      this->max = max;
  }

  /** Set the minimum extent of this dimension
   * @param min :: value */
  void Dimension::setMin(CoordType min)
  {
      this->min = min;
  }

  /** Set the name of this dimension
   * @param name :: name string */
  void Dimension::setName(std::string name)
  {
      this->name = name;
  }

  /** Set the units of this dimension
   * @param units :: unit string */
  void Dimension::setUnits(std::string units)
  {
      this->units = units;
  }



} // namespace Mantid
} // namespace MDEvents

