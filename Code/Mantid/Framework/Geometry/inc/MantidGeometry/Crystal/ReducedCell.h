#ifndef MANTID_GEOMETRY_REDUCED_CELL_H_
#define MANTID_GEOMETRY_REDUCED_CELL_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
namespace Geometry
{
/**
    @class ReducedCell 
  
    Instances of this class represent information about reduced cell types
    including the transformation required to transform the reduced cell to
    a conventional cell.  Essentially, each instance of this class represents
    one row of information from Table 2 in the paper:
    "Lattice Symmetry and Identification -- The Fundamental Role of Reduced
     Cells in Materials Characterization", Alan D. Mighell, Vol. 106, Number 6,
    Nov-Dec 2001, Journal of Research of the National Institute of Standards
    and Technology.  This class is a direct port of the ISAW class
    ReducedCellInfo.
 
    @author Dennis Mikkelson 
    @date   2012-01-02 
     
    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories
  
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
    
    File change history is stored at: 
                 <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>

    Code Documentation is available at 
                 <http://doxygen.mantidproject.org>
 */

  class MANTID_GEOMETRY_DLL ReducedCell
  {
    public:
      // Construct a ReducedCell for the specified form and lattice parameters
      ReducedCell( size_t form_num = 0, 
                   double a = 1,      double b = 1,     double c = 1,
                   double alpha = 90, double beta = 90, double gamma = 90 );

      size_t            GetFormNum();
      std::string       GetCellType();
      std::string       GetCentering();
      
      // Get the "distance" between the scalars for this form and another form
      double            WeightedDistance( const ReducedCell & other );

      // Get transformation between the Niggli cell and the conventional cell
      Kernel::DblMatrix GetTransformation();

      // String constants for cell types
      static const std::string NONE;
      static const std::string CUBIC;
      static const std::string HEXAGONAL;
      static const std::string RHOMBOHEDRAL;
      static const std::string TETRAGONAL;
      static const std::string ORTHORHOMBIC;
      static const std::string MONOCLINIC;
      static const std::string TRICLINIC;

      // String constants for centerings
      static const std::string F_CENTERED;
      static const std::string I_CENTERED;
      static const std::string C_CENTERED;
      static const std::string P_CENTERED;
      static const std::string R_CENTERED;

      enum { NUM_CELL_TYPES = 44 };

    private:
      void                init( size_t form_num,
                                double a_a, double b_b, double c_c,
                                double b_c, double a_c, double a_b  );
      void                foot_note_b( double a_a, double a_c );
      void                foot_note_c( double b_b, double b_c );
      void                foot_note_d( double c_c, double b_c );
      void                foot_note_e( double a_a, double c_c, double a_c );
      void                foot_note_f( double b_b, double c_c, double a_c );
      void                premultiply( size_t index );
      std::vector<double> norm_vals( const ReducedCell &info );

      size_t            form_num;
      double            scalars[6];
      Kernel::DblMatrix transform;
      std::string       cell_type;
      std::string       centering;
  };

} // namespace Mantid
} // namespace Geometry

#endif  /* MANTID_GEOMETRY_REDUCED_CELL_H_ */


