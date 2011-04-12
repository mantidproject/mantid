#ifndef _MD_GEOMETRY_BAISIS_DIMENSION_H
#define _MD_GEOMETRY_BAISIS_DIMENSION_H

/** Represents a 'basis' dimension.e.g. single direction in a multidimensional workspace;
*
*   There are two kinds of dimensions in multidimensional workspace: the reciprocal lattice vectors and orthogonal dimensions, describing other independent
*   variables e.g. energy transfer or temperature;   
*   A reciprocal basis dimension represent a crystallographic direction in a reciprocal unit cell (a cell of a Bravis lattice) 
*
*   Orthogonal dimensions are always orthogonal to each other and to crystallographic directions, 
*   Crystallgraphic directions are non-orhtogonal to each other for a non-cubic reciprocal lattice.
*
*   Basis dimensions are those known from disk, that are in their raw unbinned form.

*   

    @author Owen Arnold, RAL ISIS
    @date 25/11/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/V3D.h"
#include <boost/shared_ptr.hpp>


namespace Mantid
{
  namespace Geometry
  {

    class DLLExport MDBasisDimension
    {
    public:
      /** Constructor for basis dimensions:
	  * @param id            -- arbitrary name, which describes the direction and allows to identify the direction and request it from the geometry by its name
	  *                         e.g. qx or q1, qy or q2, En, T etc.
	  *                         it has to be defined when the class of MD workspaces, specific for a particular problem is constructed, as the workspaces
	  *                         with different id-s can not be mixed in the same binary operation.
	  * @param isReciprocal  -- a boolean value, sepcifying if the direction is reciprocal or orthogonal
	  * @param columnNumbner -- the position of the data, describing the coordinate of MD Data point in this direction in the MDDataPoints table (see MDDPoints class)
	  *                         Has to be consistent with MDDPoint and MDDPoints
	  * @param inDirection   -- direction of this basis vector. Zero vector for orthogonal dimensions and has to be defined properly for reciprocal
	  * @param UnitID        -- the units for this direction. A reciprocal dimension always have a "MomentumTransfer" unit, which overrides the value, specified in 
	  *                         the constructor. An orthogonal dimention can have any unit, known by the unit factory. The default is enerty transfer. 
	  */
      explicit MDBasisDimension(std::string id, bool isReciprocal, int columnNumber,const V3D &inDirection=V3D(),const std::string &UnitID="DeltaE");
      
      bool operator==(const MDBasisDimension &other) const;
      bool operator!=(const MDBasisDimension &other) const;
      bool operator < (const MDBasisDimension &other) const;

      std::string getId() const;
      bool getIsReciprocal() const;
      int getColumnNumber() const;
	  V3D getDirection()const{return direction;}
	  Kernel::Unit & getUnits()const{return *spUnit;}
   private:
	  std::string m_id; //Equivalent to tag in older definitions.
      bool m_isReciprocal;
	  //Column number correponding to the particular tag
      int  m_columnNumber;
      //Some convertable Unit e.g. energy for an orthogonal dimension or momentum transfer for a reciprocal
	  boost::shared_ptr<Kernel::Unit> spUnit;
      /** the direction of the lattice vector in an ortogonal coordinate system, the length of this vector is equal to lattice 
	    * parameter in this direction expressed in units above */
	  V3D direction;
    };

  }
}

#endif
