#ifndef MANTID_GEOMETRY_ORIENTEDLATTICE_H_
#define MANTID_GEOMETRY_ORIENTEDLATTICE_H_
#include <MantidGeometry/Crystal/UnitCell.h>

namespace Mantid
{
namespace Geometry
{  
    /** @class OrientedLattice OrientedLattice.h Geometry/Crystal/OrientedLattice.h
    Class to implement UB matrix. 
    See documentation about UB matrix in the Mantid repository.\n
  
    @author Andrei Savici, SNS, ORNL
    @date 2011-04-15    
    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport OrientedLattice: public UnitCell
  {
    public:
      // Default constructor. a = b = c = 1, alpha = beta = gamma = 90 degrees
      OrientedLattice(MantidMat Umatrix=MantidMat(3,3,true)); 
      //Copy constructor
      OrientedLattice(const OrientedLattice& other); 
      // a,b,c constructor
      OrientedLattice(const double _a,const double _b,const double _c,MantidMat Umatrix=MantidMat(3,3,true)); 
      //a,b,c,alpha,beta,gamma constructor
      OrientedLattice(const double _a,const double _b,const double _c,const double _alpha,const double _beta,const double _gamma,MantidMat Umatrix=MantidMat(3,3,true),const int angleunit=angDegrees);
      //UnitCell constructor
      OrientedLattice(UnitCell uc ,MantidMat Umatrix=MantidMat(3,3,true));
      // Destructor
      virtual ~OrientedLattice();  

      // Access private variables
      const MantidMat& getU() const;
      const MantidMat getUB() const;
      void setU(MantidMat& newU);
      void setUB(MantidMat& newUB);

    private:
      MantidMat U;
  };
} // namespace Mantid
} // namespace Geometry
#endif  /* MANTID_GEOMETRY_UNITCELL_H_ */
