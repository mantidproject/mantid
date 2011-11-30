#ifndef MANTID_MDEVENTS_COORDTRANSFORM_H_
#define MANTID_MDEVENTS_COORDTRANSFORM_H_
    
#include "MantidAPI/SingleValueParameter.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace API
{
  /// Unique SingleValueParameter Declaration for InputNDimensions
  DECLARE_SINGLE_VALUE_PARAMETER(InDimParameter, size_t)
  /// Unique SingleValueParaemter Declaration for OutputNDimensions
  DECLARE_SINGLE_VALUE_PARAMETER(OutDimParameter, size_t)

  /** Abstract class for transforming coordinate systems.
   * This will be subclassed by e.g. CoordTransformAffine to perform
   * rotations, etc.
   * 
   * @author Janik Zikovsky
   * @date 2011-04-14
   */
  class DLLExport CoordTransform 
  {
  public:
    CoordTransform(const size_t inD, const size_t outD);
    virtual ~CoordTransform();

    /// Pure abstract methods to be implemented
    virtual std::string toXMLString() const = 0;
    virtual void apply(const coord_t * inputVector, coord_t * outVector) const = 0;

    /// @return the number of input dimensions
    size_t getInD() const
    { return inD; };

    /// @return the number of output dimensions
    size_t getOutD() const
    { return outD; };
//
//    /// Compound the transformation by appending another one after this one.
//    virtual void compound(CoordTransform * other) = 0;

  protected:
    /// Input number of dimensions
    size_t inD;

    /// Output number of dimensions
    size_t outD;
  };


} // namespace Mantid
} // namespace API

#endif  /* MANTID_MDEVENTS_COORDTRANSFORM_H_ */
