#ifndef MANTIDGEOMETRY_BOUNDINGBOX_H_
#define MANTIDGEOMETRY_BOUNDINGBOX_H_

#include "MantidKernel/Exception.h"
#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/V3D.h"
#include <boost/shared_ptr.hpp>
#include <sstream>

namespace Mantid
{
  namespace Geometry
  {

    /** 
    A simple structure that defines an axis-aligned cuboid shaped bounding box for a geometrical object. 
    It is a thin structure containing the 6 points that define the corners of the cuboid.

    @author Martyn Gigg
    @date 01/10/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport BoundingBox
    {
    public:
      /// Default constructor constructs a zero-sized box
      BoundingBox() : m_minPoint(), m_maxPoint()
      {
      }

      /** Constructor taking six points. If inconsistent points are defined, i.e. xmin > xmax, then an error is thrown
      * @param xmax Value of maximum in X. It must be greater than xmin.
      * @param ymax Value of maximum in Y. It must be greater than ymin.
      * @param zmax Value of maximum in Z. It must be greater than zmin.
      * @param xmin Value of minimum in X. It must be less than xmax.
      * @param ymin Value of minimum in Y. It must be less than ymax.
      * @param zmin Value of minimum in Z. It must be less than zmax.
      */
      BoundingBox(double xmax, double ymax, double zmax, double xmin, double ymin, double zmin)
        : m_minPoint(xmin,ymin,zmin), m_maxPoint(xmax, ymax, zmax)
      {
        // Sanity check
        if( xmax < xmin || ymax < ymin || zmax < zmin )
        {
          std::ostringstream error;
          error << "Error creating bounding box, inconsistent values given:\n\t"
            << "xmin=" << xmin << ", xmax=" << xmax << "\n"
            << "ymin=" << ymin << ", ymax=" << ymax << "\n"
            << "zmin=" << zmin << ", zmax=" << zmax << "\n";
          throw std::invalid_argument(error.str());
        }
      }
      /** @name Point access */
      //@{
      /// Return the minimum value of X
      inline const double & xMin() const { return m_minPoint.X(); }
      /// Return the maximum value of X
      inline const double & xMax() const { return m_maxPoint.X(); }
      /// Return the minimum value of Y
      inline const double & yMin() const { return m_minPoint.Y(); }
      /// Return the maximum value of Y
      inline const double & yMax() const { return m_maxPoint.Y(); }
      /// Return the minimum value of Z
      inline const double & zMin() const { return m_minPoint.Z(); }
      /// Return the maximum value of Z
      inline const double & zMax() const { return m_maxPoint.Z(); }
      /// Returns the min point of the box
      inline const V3D & minPoint() const { return m_minPoint; }
      /// Returns the min point of the box
      inline const V3D & maxPoint() const { return m_maxPoint; }
      /// Returns the centre of the bounding box
      inline V3D centrePoint() const { return Geometry::V3D(0.5*(xMax() + xMin()), 0.5*(yMax() + yMin()), 0.5*(zMax() + zMin())); }
      //@}
      
      /** @name Querying */
      //@{
      /// Is the given point within the bounding box?
      bool isPointInside(const V3D & point) const;
      /// Does a line intersect the bounding box
      bool doesLineIntersect(const V3D & startPoint, const V3D & lineDir) const;
      /// Calculate the angular half width from the given point
      double angularWidth(const V3D & observer) const;
      //@}

      /** @name Box mutation functions*/
      //@{
      /// Return the minimum value of X (non-const)
      inline double & xMin() { return m_minPoint[0]; }
      /// Return the maximum value of X  (non-const)
      inline double & xMax() { return m_maxPoint[0]; }
      /// Return the minimum value of Y  (non-const)
      inline double & yMin() { return m_minPoint[1]; }
      /// Return the maximum value of Y  (non-const)
      inline double & yMax() { return m_maxPoint[1]; }
      /// Return the minimum value of Z  (non-const)
      inline double & zMin() { return m_minPoint[2]; }
      /// Return the maximum value of Z  (non-const)
      inline double & zMax() { return m_maxPoint[2]; }
      /// Grow the bounding box so that it also encompasses the given box
      void grow(const BoundingBox & other);
      //@}

    private:
      /// The minimum point of the axis-aligned box
      V3D m_minPoint;
      /// The maximum point of the axis-aligned box
      V3D m_maxPoint;
    };

    /// A shared pointer to a BoundingBox
    typedef boost::shared_ptr<BoundingBox> BoundingBox_sptr;
    /// A shared pointer to a const BoundingBox
    typedef boost::shared_ptr<const BoundingBox> BoundingBox_const_sptr;



    /*! Print out the bounding box values to a stream.
     */
    std::ostream& operator<<(std::ostream& os, const BoundingBox& box);


  }
}


#endif //MANTIDGEOMETRY_BOUNDINGBOX_H_
