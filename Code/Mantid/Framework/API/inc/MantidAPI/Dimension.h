#ifndef MANTID_API_DIMENSION_H_
#define MANTID_API_DIMENSION_H_
    
#include "MantidKernel/System.h"


namespace Mantid
{
namespace MDEvents
{

  /** Typedef for the data type to use for coordinate axes.
   * This could be a float or a double, depending on requirements.
   * We can change this in order to compare
   * performance/memory/accuracy requirements.
   */
  typedef double CoordType;
}
}


namespace Mantid
{
namespace API
{

  /** Dimension : A simple class describing a dimension in an
   * MDEventWorkspace
   * 
   * @author Janik Zikovsky
   * @date 2011-02-25 13:55:55.698592
   */
  class DLLExport Dimension
  {
  public:
    Dimension();
    Dimension(MDEvents::CoordType min, MDEvents::CoordType max, std::string name, std::string units);
    ~Dimension();
    MDEvents::CoordType getMax() const;
    MDEvents::CoordType getMin() const;
    std::string getName() const;
    std::string getUnits() const;
    void setMax(MDEvents::CoordType max);
    void setMin(MDEvents::CoordType min);
    void setName(std::string name);
    void setUnits(std::string units);
    
  private:
    MDEvents::CoordType min;
    MDEvents::CoordType max;
    std::string name;
    std::string units;
  };


} // namespace Mantid
} // namespace API

#endif  /* MANTID_API_DIMENSION_H_ */
