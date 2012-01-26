#ifndef MANTID_GEOMETRY_QUADRILATERAL_H_
#define MANTID_GEOMETRY_QUADRILATERAL_H_
    
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/Vertex2D.h"

namespace Mantid
{
  namespace Geometry
  {

    /** Quadrilateral

        A ConvexPolygon with only 4 vertices. Better performance as no dynamic allocation
    
        @author Martyn Gigg
        @date 2011-07-22

        Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport Quadrilateral : public ConvexPolygon
    {
    public:
      /// Constructor with the four vertices
      Quadrilateral(const Kernel::V2D & lowerLeft, const Kernel::V2D & lowerRight,
                    const Kernel::V2D & upperRight, const Kernel::V2D & upperLeft);
      /// Special constructor for a rectangle
      Quadrilateral(const double lowerX, const double upperX, 
                    const double lowerY, const double upperY);
      /// Copy constructor
      Quadrilateral(const Quadrilateral & other);
      /// Destructor
      ~Quadrilateral();
      /// Index access.
      virtual const Kernel::V2D& operator[](const size_t index) const;
      /// Is a point inside this polygon
      virtual bool contains(const Kernel::V2D & point) const;
      /// Compute the area of the polygon using triangulation
      virtual double area() const;
      /// Compute the 'determinant' of the points
      virtual double determinant() const;
      /// Return the lowest X value in the polygon
      double smallestX() const;
      /// Return the largest X value in the polygon
      double largestX() const;
      /// Return the lowest Y value in the polygon
      double smallestY() const;
      /// Return the largest Y value in the polygon
      double largestY() const;

    private:
      /// Default constructor
      Quadrilateral();
      /// Initalize the object
      void initialize();
      /// Lower left
      Vertex2D m_lowerLeft;
      /// Lower right
      Vertex2D m_lowerRight;
      /// Upper right
      Vertex2D m_upperRight;
      /// Upper left
      Vertex2D m_upperLeft;
      /// Lowest X value
      double m_lowestX;
      /// Highest X value
      double m_highestX;
      /// Lowest Y value
      double m_lowestY;
      /// Highest Y value
      double m_highestY;
    };


  } // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_QUADRILATERAL_H_ */
