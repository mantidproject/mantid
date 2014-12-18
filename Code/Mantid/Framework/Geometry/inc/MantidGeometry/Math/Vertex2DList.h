#ifndef MANTID_GEOMETRY_VERTEX2DLIST_H_
#define MANTID_GEOMETRY_VERTEX2DLIST_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V2D.h"
#include <vector>

namespace Mantid
{
  namespace Geometry
  {

    /** 
    A Vertex2DList holds a unique list of Vertex2D objects and allows access via
    the vertex index. The list order is never mutated
    
    @author Martyn Gigg, Tessella plc

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class MANTID_GEOMETRY_DLL Vertex2DList 
    {
    public:
      /// Constructor
      Vertex2DList() : m_vertices(0) {}
      /**
       * The list is constructed to the given size with each point at the origin
       * @param npoints :: The list size
       */
      Vertex2DList(const size_t npoints) : m_vertices(npoints, Kernel::V2D()) {}

      /**
       * Returns the number of vertices in the list
       * @returns The number of vertices stored
       */
      inline size_t size() const { return m_vertices.size(); }
      /// Operator access non-const version
      Kernel::V2D& operator[](const size_t index);
      /// Operator access const version
      const Kernel::V2D& operator[](const size_t index) const;
      /// Access the first element
      const Kernel::V2D& front() const;
      /// Access the last element
      const Kernel::V2D& back() const;
      /// Append a point to the list and return its index or return an existing points index
      unsigned int insert(const Kernel::V2D & point);
      
    private:
      /// Is the point already in the list. 
      int indexOf(const Kernel::V2D & point) const;

      /// The actual list
      std::vector<Kernel::V2D> m_vertices;
    };
    
    
  } // namespace Geometry
} // namespace Mantid

#endif  /* MANTID_GEOMETRY_VERTEX2DLIST_H_ */
