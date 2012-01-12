/* File: ConventionalCell.cpp */

#include <stdexcept>
#include <algorithm>

#include "MantidKernel/V3D.h"
#include "MantidGeometry/Crystal/ConventionalCell.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"


namespace Mantid
{
namespace Geometry
{
  using Mantid::Kernel::V3D;
  using Mantid::Kernel::DblMatrix;

  /**
   *  Construct a ConventionalCell for the specified orientation matrix
   *  and the specified row of Table 2.  The form number must be between
   *  1 and 44.
   *  @param  UB        The orientation matrix corresponding to a Niggli
   *                    reduced cell.
   *  @param  form_num  The row number from Table 2, that specifies the
   *                    reduced form number.
   */
  ConventionalCell::ConventionalCell( const  Kernel::DblMatrix & UB,
                                             size_t              form_num )
  {
    form_number = form_num;
    std::vector<double> lat_par;
    IndexingUtils::GetLatticeParameters( UB, lat_par );
    
    ReducedCell form_0 = ReducedCell( 0,
                                      lat_par[0], lat_par[1], lat_par[2],
                                      lat_par[3], lat_par[4], lat_par[5] );
    ReducedCell form_i = ReducedCell( form_num,
                                      lat_par[0], lat_par[1], lat_par[2],
                                      lat_par[3], lat_par[4], lat_par[5] );
    init( UB, form_0, form_i );
  }


  /**
   *  Get the form number corresponding to this conventional cell. 
   *
   *  @return the form number for this conventional cell.
   */
  size_t ConventionalCell::GetFormNum()
  {
    return form_number;
  }


  /**
   *  Get the error in the scalars for this conventional cell.
   *
   *  @return  The maximum absolute weighted difference between the
   *           scalars for this conventional cell and form 0.
   */
  double ConventionalCell::GetError()
  {
    return scalars_error;
  }


  /**
   *  Get the cell type for this conventional cell. 
   *
   *  @return a std::string specifying the cell type.
   */
  std::string ConventionalCell::GetCellType()
  {
    return cell_type;
  }


  /**
   *  Get centering for this conventional cell. 
   *
   *  @return a std::string specifying the centering type.
   */
  std::string ConventionalCell::GetCentering()
  {
    return centering;
  }


  /**
   *  Get a copy of the original UB matrix that was passed in to the 
   *  constructor for this object.
   *
   *  @return  a 3x3 matrix with the original UB matrix
   */
  Kernel::DblMatrix ConventionalCell::GetOriginalUB()
  {
    return original_UB;
  }


  /**
   *  Get a copy of the orientation matrix that indexes the peaks in a
   *  way that corresponds to the conventional cell.
   *
   *  @return  a 3x3 matrix with the new UB matrix.
   */
  Kernel::DblMatrix ConventionalCell::GetNewUB()
  {
    return adjusted_UB;
  }


  /**
   *  Get the sum of the sides, |a|+|b|+|c| of the conventional cell.
   *
   *  @return The sum of the sides of the conventional cell.
   */
  double ConventionalCell::GetSumOfSides()
  {
    std::vector<double> lat_par;
    IndexingUtils::GetLatticeParameters( adjusted_UB, lat_par );
    return lat_par[0] + lat_par[1] + lat_par[2];
  }


  /**
   *  Initialize the fields of this ConventionalCell object, using
   *  a specified matrix and two forms. 
   *
   *  @param UB       The orientation matrix for the Niggli cell
   *  @param form_0   The reduced cell form built with the lattice parameters
   *                  for UB and form number zero.
   *  @param form_i   The reduced cell form built with the lattice parameters
   *                  for UB and the form number of the desired conventional
   *                  cell. 
   */ 
  void ConventionalCell::init( const Kernel::DblMatrix & UB,
                                     ReducedCell       & form_0,
                                     ReducedCell       & form_i )
  {
    scalars_error = form_0.WeightedDistance( form_i );
    cell_type = form_i.GetCellType();
    centering = form_i.GetCentering();

    original_UB = Kernel::DblMatrix( UB );

    Kernel::DblMatrix cell_tran = form_i.GetTransformation();
    cell_tran.Invert();
    adjusted_UB = UB * cell_tran;    

    if ( cell_type == ReducedCell::ORTHORHOMBIC )
    {
      SetSidesIncreasing( adjusted_UB );
    }
    else if ( cell_type == ReducedCell::TETRAGONAL  )
    {
      StandardizeTetragonal( adjusted_UB );
    }
    else if ( cell_type == ReducedCell::HEXAGONAL   ||
              cell_type == ReducedCell::RHOMBOHEDRAL  )
    {
      StandardizeHexagonal( adjusted_UB );
    }
  }


  /**
   *  Change UB to a new matrix corresponding to a unit cell with the sides
   *  in increasing order of magnitude.  This is used to arrange the UB matrix
   *  for an orthorhombic cell into a standard order.
   *
   *  @param UB on input this should correspond to an orthorhombic cell. 
   *            On output, it will correspond to an orthorhombic cell with
   *            sides in increasing order.
   */
  void ConventionalCell::SetSidesIncreasing( Kernel::DblMatrix & UB )
  {
    V3D a_dir;
    V3D b_dir;
    V3D c_dir;
    IndexingUtils::GetABC( UB, a_dir, b_dir, c_dir );

    std::vector<V3D> edges;
    edges.push_back( a_dir );
    edges.push_back( b_dir );
    edges.push_back( c_dir );
    std::sort( edges.begin(), edges.end(), IndexingUtils::CompareMagnitude );

    V3D a = edges[0];
    V3D b = edges[1];
    V3D c = edges[2];

    V3D acrossb = a.cross_prod( b );           // keep a,b,c right handed
    if ( acrossb.scalar_prod(c) < 0 )
    {
      c = c * (-1);
    } 
    IndexingUtils::GetUB( UB, a, b, c );
  }


  /**
   *  Change UB to a new matrix corresponding to a unit cell with the first 
   *  two sides approximately equal in magnitude.  This is used to arrange 
   *  the UB matrix for a tetragonal cell into a standard order.
   *
   *  @param UB on input this should correspond to a tetragonal cell.  
   *            On output, it will correspond to a tetragonal cell with the 
   *            first two sides, a and b, set to the two sides that are most
   *            nearly equal in length. 
   */
  void ConventionalCell::StandardizeTetragonal( Kernel::DblMatrix & UB )
  {
    V3D a;
    V3D b;
    V3D c;
    IndexingUtils::GetABC( UB, a, b, c );

    double a_b_diff = fabs( a.norm() - b.norm() ) /
                      fmin( a.norm(), b.norm() );

    double a_c_diff = fabs( a.norm() - c.norm() ) /
                      fmin( a.norm(), c.norm() );

    double b_c_diff = fabs( b.norm() - c.norm() ) /
                      fmin( b.norm(), c.norm() );

                          // if needed, change UB to have the two most nearly
                          // equal sides first.
    if ( a_c_diff <= a_b_diff && a_c_diff <= b_c_diff )
    {
      IndexingUtils::GetUB( UB, c, a, b );
    }
    else if ( b_c_diff <= a_b_diff && b_c_diff <= a_c_diff )
    {
      IndexingUtils::GetUB( UB, b, c, a );
    }
  
  }


  /**
   *  Change UB to a new matrix corresponding to a hexagonal unit cell with
   *  angles approximately 90, 90, 120.  This is used to arrange 
   *  the UB matrix for a hexagonal or rhombohedral cell into a standard order.
   *
   *  @param UB on input this should correspond to a hexagonal or rhombohedral
   *            On output, it will correspond to a hexagonal cell with angles
   *            approximately 90, 90, 120.
   */
  void ConventionalCell::StandardizeHexagonal( Kernel::DblMatrix & UB )
  {
    V3D a;
    V3D b;
    V3D c;
    IndexingUtils::GetABC( UB, a, b, c );

    double alpha = b.angle( c ) * 180.0/M_PI;
    double beta  = c.angle( a ) * 180.0/M_PI;
                                                // first, make the non 90 
                                                // degree angle last
    if ( fabs(alpha-90) > 20 )
    {
      IndexingUtils::GetUB( UB, b, c, a );
    }
    else if ( fabs(beta-90) > 20 )
    {
      IndexingUtils::GetUB( UB, c, a, b );
    }
                                                // if the non 90 degree angle
                                                // is about 60 degrees, make
                                                // it about 120 degrees.
    IndexingUtils::GetABC( UB, a, b, c );
    double gamma = a.angle( b ) * 180.0/M_PI;
    if ( fabs( gamma - 60 ) < 10 )
    {
      a = a * ( -1 );                           // reflect a and c to change
      c = c * ( -1 );                           // alpha and gamma to their
      IndexingUtils::GetUB( UB, a, b, c );      // supplementary angle
    }
  }


} // namespace Mantid
} // namespace Geometry
                         
