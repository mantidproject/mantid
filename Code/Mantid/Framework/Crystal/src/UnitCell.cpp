#include "MantidCrystal/UnitCell.h"
#include "MantidGeometry/V3D.h"
#include <exception>
namespace Mantid
{
namespace Crystal
{

  UnitCell::UnitCell(): G(3,3), Gstar(3,3), B(3,3)
  {
	  da[0]=da[1]=da[2]=1.;
	  da[3]=da[4]=da[5]=deg2rad*90.0;
	  recalculate();
  }
  
  UnitCell::UnitCell(const UnitCell& other):da(other.da),ra(other.ra),G(other.G),Gstar(other.Gstar),B(other.B)
  {
  }

  UnitCell::UnitCell(double _a, double _b, double _c): G(3,3), Gstar(3,3), B(3,3)
  {
	  da[0]=_a;da[1]=_b;da[2]=_c;
	  // Angles are 90 degrees in radians ->Pi/2
	  da[3]=da[4]=da[5]=0.5*M_PI;
	  recalculate();
  }

  UnitCell::UnitCell(double _a, double _b, double _c, double _alpha, double _beta, double _gamma,const int Unit): G(3,3), Gstar(3,3), B(3,3)
  {
	  da[0]=_a;da[1]=_b;da[2]=_c;
	  // Angle transformed in radians
    if(Unit==Degrees)
      {
	      da[3]=deg2rad*_alpha;
	      da[4]=deg2rad*_beta;
	      da[5]=deg2rad*_gamma;
      }
    else
      {	      
        da[3]=_alpha;
	      da[4]=_beta;
	      da[5]=_gamma;
      }
	  recalculate();
  }

  UnitCell::~UnitCell()
  {
  }

  void UnitCell::set(double _a, double _b, double _c, double _alpha, double _beta, double _gamma,const int Unit)
  {
    da[0]=_a; da[1]=_b; da[2]=_c;
    if (Unit==Degrees)
      {
	      da[3]=deg2rad*_alpha;
	      da[4]=deg2rad*_beta;
	      da[5]=deg2rad*_gamma;
      }
    else
      {	      
        da[3]=_alpha;
	      da[4]=_beta;
	      da[5]=_gamma;
      }
	  recalculate();
  }

  void UnitCell::seta(double _a)
  {
    da[0]=_a;
    recalculate();
  }

  void UnitCell::setb(double _b)
  {
    da[1]=_b;
    recalculate();
  }

  void UnitCell::setc(double _c)
  {
    da[2]=_c;
    recalculate();
  }

  void UnitCell::setalpha(double _alpha,const int Unit)
  {
    if (Unit==Degrees) 
      da[3]=deg2rad*_alpha;
    else 
      da[3]=_alpha;
    recalculate();
  }
  
  void UnitCell::setbeta(double _beta,const int Unit)
  {
    if (Unit==Degrees) 
      da[4]=deg2rad*_beta;
    else 
      da[4]=_beta;
    recalculate();
  }

  void UnitCell::setgamma(double _gamma,const int Unit)
  {
    if (Unit==Degrees) 
      da[5]=deg2rad*_gamma;
    else 
      da[5]=_gamma;
    recalculate();
  }

  double UnitCell::d(double h, double k, double l) const
  {
	  return 1/dstar(h,k,l);
  }

  double UnitCell::dstar(double h, double k, double l) const
  {
	  Geometry::V3D Q(h,k,l); //create a V3D vector h,k,l
    Q=B*Q; //transform into $AA^-1$
	  return Q.norm();
  }

	double UnitCell::recAngle(double h1, double k1, double l1, double h2, double k2, double l2, const int Unit) const
  {
	  Geometry::V3D Q1(h1,k1,l1),Q2(h2,k2,l2);
	  double E,ang;
    Q1=Gstar*Q1;
    E=Q1.scalar_prod(Q2);
	  ang=acos(E/dstar(h1,k1,l1)/dstar(h2,k2,l2));
    if (Unit==Degrees) 
      return rad2deg*ang;
    else 
      return ang;    
  }

  double UnitCell::volume() const
  {
  	double volume=G.determinant();
  	return sqrt(volume);
  }


  double UnitCell::recVolume() const
  {
  	double recvolume=Gstar.determinant();
  	return sqrt(recvolume);
  }

      
  void UnitCell::recalculateFromGstar(Geometry::Matrix<double>& NewGstar)
  {
    // TODO - not finished
    Gstar=NewGstar;
    G=Gstar;
    G.Invert();
    return;
  }  


  void UnitCell::recalculate()
  {
	  calculateG();
	  calculateGstar();
	  calculateReciprocalLattice();
	  calculateB();
  }

  void UnitCell::calculateG()
  {
	  // Calculate the metric tensor.
		G[0][0]=da[0]*da[0];
		G[1][1]=da[1]*da[1];
		G[2][2]=da[2]*da[2];
		G[0][1]=da[0]*da[1]*cos(da[5]);
		G[0][2]=da[0]*da[2]*cos(da[4]);
		G[1][2]=da[1]*da[2]*cos(da[3]);
		G[1][0]=G[0][1];
		G[2][0]=G[0][2];
		G[2][1]=G[1][2];
		return;
  }

  void UnitCell::calculateGstar()
  {
	  // Reciprocal metrix tensor is simply the inverse of the direct one
    Gstar=G;
    if (Gstar.Invert()==0)
			{}//throw std::range_error("UnitCell not properly initialized");
    return;
  }
	   
  void UnitCell::calculateReciprocalLattice()
  {
    //TODO
  }
	 
  void UnitCell::calculateB()
  {
    //TODO
  }

} // namespace Mantid
} // namespace Crystal
