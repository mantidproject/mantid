#include <MantidGeometry/Crystal/UnitCell.h>
#include <MantidGeometry/V3D.h>
#include <MantidKernel/System.h>

namespace Mantid
{
namespace Geometry
{
  // Default constructor. a = b = c = 1, alpha = beta = gamma = 90 degrees
  UnitCell::UnitCell(): da(6), ra(6), G(3,3), Gstar(3,3), B(3,3)
  {
	  da[0]=da[1]=da[2]=1.;
	  da[3]=da[4]=da[5]=deg2rad*90.0;
	  recalculate();
  }
  
  UnitCell::UnitCell(const UnitCell& other):da(other.da),ra(other.ra),G(other.G),Gstar(other.Gstar),B(other.B)
  {
  }

  UnitCell::UnitCell(double _a, double _b, double _c): da(6), ra(6), G(3,3), Gstar(3,3), B(3,3)
  {
	  da[0]=_a;da[1]=_b;da[2]=_c;
	  // Angles are 90 degrees in radians ->Pi/2
	  da[3]=da[4]=da[5]=0.5*M_PI;
	  recalculate();
  }

  UnitCell::UnitCell(double _a, double _b, double _c, double _alpha, double _beta, double _gamma,const int angleunit): da(6), ra(6), G(3,3), Gstar(3,3), B(3,3)
  {
	  da[0]=_a;da[1]=_b;da[2]=_c;
	  // Angle transformed in radians
    if(angleunit==latDegrees)
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


  double UnitCell::a1() const
  {
    return da[0];
  }

  double UnitCell::a2() const
  {
    return da[1];
  }

  double UnitCell::a3() const
  {
    return da[2];
  }

  double UnitCell::alpha1() const
  {
    return da[3];
  }

  double UnitCell::alpha2() const
  {
    return da[4];
  }

  double UnitCell::alpha3() const
  {
    return da[5];
  }

  double UnitCell::a() const
  {
    return da[0];
  }

  double UnitCell::b() const
  {
    return da[1];
  }

  double UnitCell::c() const
  {
    return da[2];
  }

  double UnitCell::alpha() const
  {
    return da[3]*rad2deg;
  }

  double UnitCell::beta() const
  {
    return da[4]*rad2deg;
  }

  double UnitCell::gamma() const
  {
    return da[5]*rad2deg;
  }

  double UnitCell::b1() const
  {
    return ra[0];
  }

  double UnitCell::b2() const
  {
    return ra[1];
  }

  double UnitCell::b3() const
  {
    return ra[2];
  }

  double UnitCell::beta1() const
  {
    return ra[3];
  }

  double UnitCell::beta2() const
  {
    return ra[4];
  }

  double UnitCell::beta3() const
  {
    return ra[5];
  }

  double UnitCell::astar() const
  {
    return ra[0];
  }

  double UnitCell::bstar() const
  {
    return ra[1];
  }

  double UnitCell::cstar() const
  {
    return ra[2];
  }

  double UnitCell::alphastar() const
  {
    return ra[3]*rad2deg;
  }

  double UnitCell::betastar() const
  {
    return ra[4]*rad2deg;
  }

  double UnitCell::gammastar() const
  {
    return ra[5]*rad2deg;
  }


  void UnitCell::set(double _a, double _b, double _c, double _alpha, double _beta, double _gamma,const int angleunit)
  {
    da[0]=_a; da[1]=_b; da[2]=_c;
    if (angleunit==latDegrees)
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

  void UnitCell::setalpha(double _alpha,const int angleunit)
  {
    if (angleunit==latDegrees)
      da[3]=deg2rad*_alpha;
    else 
      da[3]=_alpha;
    recalculate();
  }
  
  void UnitCell::setbeta(double _beta,const int angleunit)
  {
    if (angleunit==latDegrees)
      da[4]=deg2rad*_beta;
    else 
      da[4]=_beta;
    recalculate();
  }

  void UnitCell::setgamma(double _gamma,const int angleunit)
  {
    if (angleunit==latDegrees)
      da[5]=deg2rad*_gamma;
    else 
      da[5]=_gamma;
    recalculate();
  }

  double UnitCell::d(double h, double k, double l) const
  {
	  return 1./dstar(h,k,l);
  }

  double UnitCell::dstar(double h, double k, double l) const
  {
	  V3D Q(h,k,l); //create a V3D vector h,k,l
    Q=B*Q; //transform into $AA^-1$
	  return Q.norm();
  }

	double UnitCell::recAngle(double h1, double k1, double l1, double h2, double k2, double l2, const int angleunit) const
  {
	  V3D Q1(h1,k1,l1),Q2(h2,k2,l2);
	  double E,ang;
    Q1=Gstar*Q1;
    E=Q1.scalar_prod(Q2);
	  ang=acos(E/dstar(h1,k1,l1)/dstar(h2,k2,l2));
    if (angleunit==latDegrees)
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

  const Geometry::MantidMat& UnitCell::getG() const
  {
    return G;
  }

  const Geometry::MantidMat& UnitCell::getGstar() const
  {
    return Gstar;
  }

  const Geometry::MantidMat& UnitCell::getB() const
  {
    return B;
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
    ra[0]=sqrt(Gstar[0][0]);//a*
    ra[1]=sqrt(Gstar[1][1]);//b*
    ra[2]=sqrt(Gstar[2][2]);//c*
    ra[3]=acos(Gstar[1][2]/ra[1]/ra[2]);//alpha* 
    ra[4]=acos(Gstar[0][2]/ra[0]/ra[2]);//beta* 
    ra[5]=acos(Gstar[0][1]/ra[0]/ra[1]);//gamma* 
  }
	 
  void UnitCell::calculateB()
  {
    // B matrix using a right handed coordinate system with b1 along x and y in the (b1,b2) plane.
	  // This is the convention in Busing and Levy.
	  // | b1 b2cos(beta3)      b3cos(beta2)        |
	  // | 0  b2sin(beta3) -b3sin(beta2)cos(alpha1) |
	  // | 0       0                  1/a3          |
	  B[0][0]=ra[0];
	  B[0][1]=ra[1]*cos(ra[5]);
	  B[0][2]=ra[2]*cos(ra[4]);
	  B[1][0]=0.;
	  B[1][1]=ra[1]*sin(ra[5]);
	  B[1][2]=-ra[2]*sin(ra[4])*cos(da[3]);
	  B[2][0]=0.;
    B[2][1]=0.;
	  B[2][2]=1./da[2];
	  return;
  }

  void UnitCell::recalculateFromGstar(MantidMat& NewGstar)
  {
    Gstar=NewGstar;
    calculateReciprocalLattice();
    G=Gstar;
    G.Invert();    
    da[0]=sqrt(G[0][0]);//a
    da[1]=sqrt(G[1][1]);//b
    da[2]=sqrt(G[2][2]);//c
    da[3]=acos(G[1][2]/da[1]/da[2]);//alpha
    da[4]=acos(G[0][2]/da[0]/da[2]);//beta
    da[5]=acos(G[0][1]/da[0]/da[1]);//gamma 
    calculateB();
    return;
  }  


} // namespace Mantid
} // namespace Geometry
