#ifndef MANTID_GEOMETRY_NiggliCell_H_
#define MANTID_GEOMETRY_NiggliCell_H_

#include "MantidGeometry/Crystal/UnitCell.h"
#include <nexus/NeXusFile.hpp>

namespace Mantid
{
namespace Geometry
{  
    /** @class NiggliCell NiggliCell.h Geometry/Crystal/NiggliCell.h
    Class to implement UB matrix. 
    See documentation about UB matrix in the Mantid repository.\n
  
    @author Andrei Savici, SNS, ORNL
    @date 2011-04-15    
    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
   */
  class MANTID_GEOMETRY_DLL NiggliCell: public UnitCell
  {
    public:
      // Default constructor. a = b = c = 1, alpha = beta = gamma = 90 degrees
      NiggliCell(const Kernel::DblMatrix & Umatrix = Kernel::DblMatrix(3,3,true));
      //Copy constructor
      NiggliCell(const NiggliCell& other);
      // a,b,c constructor
      NiggliCell(const double _a,const double _b,const double _c,
                      const Kernel::DblMatrix & Umatrix = Kernel::DblMatrix(3,3,true));
      //a,b,c,alpha,beta,gamma constructor
      NiggliCell(const double _a,const double _b,const double _c,const double _alpha,const double _beta,
                      const double _gamma, const Kernel::DblMatrix & Umatrix = Kernel::DblMatrix(3,3,true),
                      const int angleunit=angDegrees);
      //UnitCell constructor
      NiggliCell(const UnitCell & uc , const Kernel::DblMatrix & Umatrix = Kernel::DblMatrix(3,3,true));
      //UnitCell constructor
      NiggliCell(const UnitCell * uc , const Kernel::DblMatrix & Umatrix = Kernel::DblMatrix(3,3,true));
      // Destructor
      virtual ~NiggliCell();

      // Access private variables
      /// Check if a,b,c cell has angles satifying Niggli condition within epsilon
      static bool HasNiggliAngles( const Kernel::V3D  & a_dir,
                                   const Kernel::V3D  & b_dir,
                                   const Kernel::V3D  & c_dir,
                                         double         epsilon  );

      /// Construct a newUB corresponding to a Niggli cell from the given UB
      static bool MakeNiggliUB( const Kernel::DblMatrix  & UB,
                                      Kernel::DblMatrix  & newUB );

    private:
      Kernel::DblMatrix U;
      Kernel::DblMatrix UB;


  };
} // namespace Mantid
} // namespace Geometry
#endif  /* MANTID_GEOMETRY_UNITCELL_H_ */
