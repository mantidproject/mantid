#ifndef MANTID_MDEVENTS_DIMENSION_H_
#define MANTID_MDEVENTS_DIMENSION_H_
    
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEvent.h"


namespace Mantid
{
namespace MDEvents
{

  /** Dimension : A simple class describing a dimension in an
   * MDEventWorkspace
   * 
   * @author
   * @date 2011-02-25 13:55:55.698592
   */
  class Dimension
  {
  public:
    Dimension();
    Dimension(CoordType min, CoordType max, std::string name, std::string units);
    ~Dimension();
    CoordType getMax() const;
    CoordType getMin() const;
    std::string getName() const;
    std::string getUnits() const;
    void setMax(CoordType max);
    void setMin(CoordType min);
    void setName(std::string name);
    void setUnits(std::string units);
    
  private:
    CoordType min;
    CoordType max;
    std::string name;
    std::string units;
  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_DIMENSION_H_ */
