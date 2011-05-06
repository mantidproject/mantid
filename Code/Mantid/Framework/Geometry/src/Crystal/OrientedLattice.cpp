#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid
{
namespace Geometry
{
  /** Default constructor
  @param Umatrix :: orientation matrix U. By default this will be identity matrix
  */
  OrientedLattice::OrientedLattice(MantidMat Umatrix):UnitCell(),U(Umatrix)
  {
  }

  /** Copy constructor
  @param other :: The OrientedLattice from which to copy information
  */    
  OrientedLattice::OrientedLattice(const OrientedLattice& other):UnitCell(other),U(other.U)
  {
  }

  /** Constructor
  @param _a, _b, _c :: lattice parameters \f$ a, b, c \f$ \n
  with \f$\alpha = \beta = \gamma = 90^\circ \f$*/      
  OrientedLattice::OrientedLattice(const double _a,const double _b,const double _c,MantidMat Umatrix):UnitCell(_a,_b,_c),U(Umatrix)
  {
  }

  /** Constructor
  @param _a, _b, _c, _alpha, _beta, _gamma :: lattice parameters\n
  @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
  */
  OrientedLattice::OrientedLattice(const double _a,const double _b,const double _c,const double _alpha,const double _beta,const double _gamma,MantidMat Umatrix, const int angleunit):UnitCell(_a,_b,_c,_alpha,_beta,_gamma,angleunit),U(Umatrix)
  {
  }

  /** UnitCell constructor
  @param uc :: UnitCell
  @param Umatrix :: orientation matrix U. By default this will be identity matrix
  */
  OrientedLattice::OrientedLattice(UnitCell uc,MantidMat Umatrix):UnitCell(uc),U(Umatrix)
  {
  }

  /// Destructor
  OrientedLattice::~OrientedLattice()
  {
  } 


  /// Get the U matrix
  /// @return U :: U orientation matrix
  const MantidMat& OrientedLattice::getU() const
  {
    return U;
  }
  
  /// Get the UB matrix
  /// @return UB :: UB orientation matrix
  const MantidMat OrientedLattice::getUB() const
  {
    MantidMat UB;
    UB=U*this->getB();
    return UB;
  }

  void OrientedLattice::setU(MantidMat& newU)
  {
    U=newU;
  }


  void OrientedLattice::setUB(MantidMat& newUB)
  {
    MantidMat newGstar,B;
    newGstar=newUB.Tprime()*newUB;
    this->recalculateFromGstar(newGstar);
    B=this->getB();
    B.Invert();
    U=newUB*B;
  }
}//Namespace Geometry
}//Namespace Mantid
