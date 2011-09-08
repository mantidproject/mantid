#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid
{
namespace Geometry
{
  using Mantid::Kernel::DblMatrix;
  using Mantid::Kernel::V3D;

  /** Default constructor
  @param Umatrix :: orientation matrix U. By default this will be identity matrix
  */
  OrientedLattice::OrientedLattice(DblMatrix Umatrix):UnitCell()
  {
    if (Umatrix.isRotation()==true)
    { 
      U=Umatrix;
      UB=U*getB();
    }
    else throw std::invalid_argument("U is not a proper rotation");
  }

  /** Copy constructor
  @param other :: The OrientedLattice from which to copy information
  */    
  OrientedLattice::OrientedLattice(const OrientedLattice& other):UnitCell(other),U(other.U),UB(other.UB)
  {
  }

  /** Constructor
  @param _a, _b, _c :: lattice parameters \f$ a, b, c \f$ \n
  with \f$\alpha = \beta = \gamma = 90^\circ \f$*/      
  OrientedLattice::OrientedLattice(const double _a,const double _b,const double _c,DblMatrix Umatrix):UnitCell(_a,_b,_c)
  {
    if (Umatrix.isRotation()==true)
    { 
      U=Umatrix;
      UB=U*getB();
    }
    else throw std::invalid_argument("U is not a proper rotation");
  }

  /** Constructor
  @param _a, _b, _c, _alpha, _beta, _gamma :: lattice parameters\n
  @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
  */
  OrientedLattice::OrientedLattice(const double _a,const double _b,const double _c,const double _alpha,const double _beta,const double _gamma,DblMatrix Umatrix, const int angleunit):UnitCell(_a,_b,_c,_alpha,_beta,_gamma,angleunit)
  {
    if (Umatrix.isRotation()==true)
    { 
      U=Umatrix;
      UB=U*getB();
    }
    else throw std::invalid_argument("U is not a proper rotation");
  }

  /** UnitCell constructor
  @param uc :: UnitCell
  @param Umatrix :: orientation matrix U. By default this will be identity matrix
  */
  OrientedLattice::OrientedLattice(UnitCell uc,DblMatrix Umatrix):UnitCell(uc),U(Umatrix)
  {
    if (Umatrix.isRotation()==true)
    { 
      U=Umatrix;
      UB=U*getB();
    }
    else throw std::invalid_argument("U is not a proper rotation");
  }

  /// Destructor
  OrientedLattice::~OrientedLattice()
  {
  } 


  /// Get the U matrix
  /// @return U :: U orientation matrix
  const DblMatrix& OrientedLattice::getU() const
  {
    return U;
  }
  
  /** Get the UB matrix.
   The UB Matrix uses the inelastic convention:
     q = UB . (hkl)
   where q is the wavevector transfer of the LATTICE (not the neutron).
     and |q| = 1.0/d_spacing

   @return UB :: UB orientation matrix
   */
  const DblMatrix& OrientedLattice::getUB() const
  {
    return UB;
  }

  void OrientedLattice::setU(DblMatrix& newU)
  {
    if (newU.isRotation()==true)
    {
      U=newU;
      UB=U*getB();
    }
    else throw std::invalid_argument("U is not a proper rotation");
  }

  void OrientedLattice::setUB(DblMatrix& newUB)
  {
    if (UB.determinant()>0)
    {
      UB=newUB;
      DblMatrix newGstar,B;
      newGstar=newUB.Tprime()*newUB;
      this->recalculateFromGstar(newGstar);
      B=this->getB();
      B.Invert();
      U=newUB*B;
    }
    else throw std::invalid_argument("determinant of UB is not greater than 0");
  }



 /** Set the U rotation matrix, used to transform a vector expressed in the
   *  "orthogonal associated with the reciprocal lattice cell system of coordinates (RLU)"
   *  into another coordinate system defined by vectors u and v, expressed in RLU coordinate system
   *  Author: Alex Buts
   *  @param u :: vector of ?
   *  @param v :: vector of ?
   *  @return the U matrix calculated
   **/
  DblMatrix OrientedLattice::setUFromVectors(const V3D &u, const V3D &v)
  {
    //get  B-matrix of Busing and Levy
    DblMatrix B = this->getB();

    // get orthogonal system, adjacent to the unit cell;
    V3D e1 = B*u;
    e1.normalize();
    V3D V  = B*v;
    V3D e3  =e1.cross_prod(V);
    e3.normalize();
    double norm2 = e3.norm2();
    if(norm2<FLT_EPSILON){
      throw(std::invalid_argument(" two parallel vectors do not define the projection plane"));
    }

    V3D e2= e3.cross_prod(e1);

    DblMatrix Transf(3,3);
    Transf.setColumn(0,e1);
    Transf.setColumn(1,e2);
    Transf.setColumn(2,e3);
    // some det may be -1
    double det = Transf.determinant();
    Transf /=det;

    this->setU(Transf);
    return Transf;
  }


  /** Save the object to an open NeXus file.
   * @param file :: open NeXus file
   * @param group :: name of the group to create
   */
  void OrientedLattice::saveNexus(::NeXus::File * file, const std::string & group) const
  {
    file->makeGroup(group, "NXcrystal", 1);
    file->writeData("unit_cell_a", this->a());
    file->writeData("unit_cell_b", this->b());
    file->writeData("unit_cell_c", this->c());
    file->writeData("unit_cell_alpha", this->alpha());
    file->writeData("unit_cell_beta", this->beta());
    file->writeData("unit_cell_gamma", this->gamma());
    // Save the UB matrix
    std::vector<double> ub = this->UB.get_vector();
    std::vector<int> dims(2,3); // 3x3 matrix
    file->writeData("orientation_matrix", ub, dims);

    file->closeGroup();
  }

  /** Load the object from an open NeXus file.
   * @param file :: open NeXus file
   * @param group :: name of the group to open
   */
  void OrientedLattice::loadNexus(::NeXus::File * file, const std::string & group)
  {
    file->openGroup(group, "NXcrystal");
    std::vector<double> ub;
    file->readData("orientation_matrix", ub);
    // Turn into a matrix
    DblMatrix ubMat(ub);
    // This will set the lattice parameters and the U matrix:
    this->setUB(ubMat);
    file->closeGroup();
  }

}//Namespace Geometry
}//Namespace Mantid
