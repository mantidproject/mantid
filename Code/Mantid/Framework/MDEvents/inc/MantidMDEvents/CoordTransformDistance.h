#ifndef MANTID_MDEVENTS_COORDTRANSFORMDISTANCE_H_
#define MANTID_MDEVENTS_COORDTRANSFORMDISTANCE_H_
    
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidMDEvents/CoordTransform.h"


namespace Mantid
{
namespace MDEvents
{

  /** A non-linear coordinate transform that takes
   * a point from nd dimensions and converts it to a
   * single dimension: the SQUARE of the distance between the MDEvent
   * and a given point in up to nd dimensions.
   * 
   * The number of output dimensions is 1 (the square of the distance to the point).
   *
   * The square is used instead of the plain distance since square root is a slow calculation.
   *
   * @author Janik Zikovsky
   * @date 2011-04-25 14:48:33.517020
   */
  class DLLExport CoordTransformDistance : public CoordTransform
  {
  public:
    CoordTransformDistance(const size_t inD, const CoordType * center, const bool * dimensionsUsed);
    virtual ~CoordTransformDistance();

    virtual void apply(const CoordType * inputVector, CoordType * outVector);

    /// Return the center coordinate array
    const CoordType * getCenter() { return m_center; }

    /// Return the dimensions used bool array
    const bool * getDimensionsUsed() { return m_dimensionsUsed; }

  protected:
    /// array of size[inD], with the coordinates at the center
    CoordType * m_center;

    /// bool array of size[inD] where True is set for those dimensions that are considered when calculating distance
    bool * m_dimensionsUsed;
  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_COORDTRANSFORMDISTANCE_H_ */
