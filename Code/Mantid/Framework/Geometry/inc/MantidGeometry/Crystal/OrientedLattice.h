#ifndef MANTID_GEOMETRY_ORIENTEDLATTICE_H_
#define MANTID_GEOMETRY_ORIENTEDLATTICE_H_

#include "MantidGeometry/Crystal/UnitCell.h"
#include <nexus/NeXusFile.hpp>

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
   */
  class MANTID_GEOMETRY_DLL OrientedLattice: public UnitCell
  {
    public:
      // Default constructor. a = b = c = 1, alpha = beta = gamma = 90 degrees
      OrientedLattice(const Kernel::DblMatrix & Umatrix = Kernel::DblMatrix(3,3,true));
      //Copy constructor
      OrientedLattice(const OrientedLattice& other); 
      // a,b,c constructor
      OrientedLattice(const double _a,const double _b,const double _c,
                      const Kernel::DblMatrix & Umatrix = Kernel::DblMatrix(3,3,true));
      //a,b,c,alpha,beta,gamma constructor
      OrientedLattice(const double _a,const double _b,const double _c,const double _alpha,const double _beta,
                      const double _gamma, const Kernel::DblMatrix & Umatrix = Kernel::DblMatrix(3,3,true),
                      const int angleunit=angDegrees);
      //UnitCell constructor
      OrientedLattice(const UnitCell & uc , const Kernel::DblMatrix & Umatrix = Kernel::DblMatrix(3,3,true));
      // Destructor
      virtual ~OrientedLattice();  

      // Access private variables
      const Kernel::DblMatrix& getU() const;
      const Kernel::DblMatrix& getUB() const;
      void setU(const Kernel::DblMatrix& newU, const bool force = false);
      void setUB(const Kernel::DblMatrix& newUB);
      //get u and v vectors for Horace/Mslice
      Kernel::V3D getuVector();
      Kernel::V3D getvVector();
      /// Return q(hkl) from the lab coordinates
      Kernel::V3D hklFromQ(const Kernel::V3D & Q) const;
      /// Create the U matrix from two vectors
      const Kernel::DblMatrix & setUFromVectors(const Kernel::V3D &u, const Kernel::V3D &v);
      /// Save the lattice to an open NeXus file
      void saveNexus(::NeXus::File * file, const std::string & group) const;
      /// Load the lattice to from an open NeXus file
      void loadNexus(::NeXus::File * file, const std::string & group);

    private:
      Kernel::DblMatrix U;
      Kernel::DblMatrix UB;

      /** Make recalculateFromGstar private. */
      void recalculateFromGstar(const Kernel::DblMatrix& NewGstar)
      { UnitCell::recalculateFromGstar(NewGstar); }

  };
} // namespace Mantid
} // namespace Geometry
#endif  /* MANTID_GEOMETRY_UNITCELL_H_ */
