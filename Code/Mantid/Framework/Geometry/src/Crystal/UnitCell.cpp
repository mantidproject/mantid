#include <MantidGeometry/Crystal/UnitCell.h>
#include <MantidGeometry/V3D.h>
#include <MantidKernel/System.h>
#include <stdexcept>

namespace Mantid
{
namespace Geometry
{
  /** Default constructor. 
  \f$ a = b = c =  1 \mbox{\AA, } \alpha = \beta = \gamma = 90^\circ \f$ */
  UnitCell::UnitCell(): da(6), ra(6), G(3,3), Gstar(3,3), B(3,3)
  {
	  da[0]=da[1]=da[2]=1.;
	  da[3]=da[4]=da[5]=deg2rad*90.0;
	  recalculate();
  }

  /** Copy constructor
  @param other :: The UnitCell from which to copy lattice parameters
  */
  UnitCell::UnitCell(const UnitCell& other):da(other.da),ra(other.ra),G(other.G),Gstar(other.Gstar),B(other.B)
  {
  }

  /** Constructor
  @param _a, _b, _c :: lattice parameters \f$ a, b, c \f$ \n
  with \f$\alpha = \beta = \gamma = 90^\circ \f$*/
  UnitCell::UnitCell(double _a, double _b, double _c): da(6), ra(6), G(3,3), Gstar(3,3), B(3,3)
  {
	  da[0]=_a;da[1]=_b;da[2]=_c;
	  // Angles are 90 degrees in radians ->Pi/2
	  da[3]=da[4]=da[5]=0.5*M_PI;
	  recalculate();
  }

  /** Constructor
  @param _a, _b, _c, _alpha, _beta, _gamma :: lattice parameters\n
  @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
  */
  UnitCell::UnitCell(double _a, double _b, double _c, double _alpha, double _beta, double _gamma,const int angleunit): da(6), ra(6), G(3,3), Gstar(3,3), B(3,3)
  {
	  da[0]=_a;da[1]=_b;da[2]=_c;
	  // Angle transformed in radians
    if(angleunit==angDegrees)
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
  
  /// Destructor
  UnitCell::~UnitCell()
  {
  }

  /** Get lattice parameter
  @return a1 :: lattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )
  @see a()*/
  double UnitCell::a1() const
  {
    return da[0];
  }

  /** Get lattice parameter
  @return a2 :: lattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )
  @see b()*/
  double UnitCell::a2() const
  {
    return da[1];
  }

  /** Get lattice parameter
  @return a3 :: lattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )
  @see c()*/
	    
  double UnitCell::a3() const
  {
    return da[2];
  }

  /** Get lattice parameter
  @return alpha1 :: lattice parameter \f$ \alpha \f$ (in radians)
	@see alpha()*/
  double UnitCell::alpha1() const
  {
    return da[3];
  }

  /** Get lattice parameter
  @return alpha2 :: lattice parameter \f$ \beta \f$ (in radians)
	@see beta()*/
  double UnitCell::alpha2() const
  {
    return da[4];
  }

  /** Get lattice parameter
  @return alpha3 :: lattice parameter \f$ \gamma \f$ (in radians)
  @see gamma()*/
  double UnitCell::alpha3() const
  {
    return da[5];
  }

  /** Get lattice parameter
  @return a :: lattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )
  @see a1()*/
  double UnitCell::a() const
  {
    return da[0];
  }

  /** Get lattice parameter
  @return b :: lattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )
  @see a2()*/
  double UnitCell::b() const
  {
    return da[1];
  }

  /** Get lattice parameter
  @return c :: lattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )
  @see a3()*/
  double UnitCell::c() const
  {
    return da[2];
  }

  /** Get lattice parameter
  @return alpha :: lattice parameter \f$ \alpha \f$ (in degrees)
  @see alpha1()*/
  double UnitCell::alpha() const
  {
    return da[3]*rad2deg;
  }

  /** Get lattice parameter
  @return beta :: lattice parameter \f$ \beta \f$ (in degrees)
  @see alpha2()*/
  double UnitCell::beta() const
  {
    return da[4]*rad2deg;
  }
  
  /** Get lattice parameter
  @return gamma :: lattice parameter \f$ \gamma \f$ (in degrees)
  @see alpha3()*/
  double UnitCell::gamma() const
  {
    return da[5]*rad2deg;
  }

  /** Get reciprocal lattice parameter
  @return b1 :: lattice parameter \f$ a^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
  @see astar()*/
  double UnitCell::b1() const
  {
    return ra[0];
  }

  /** Get reciprocal lattice parameter
  @return b2 :: lattice parameter \f$ b^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
  @see bstar()*/
  double UnitCell::b2() const
  {
    return ra[1];
  }

  /** Get reciprocal lattice parameter
  @return b3 :: lattice parameter \f$ c^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
  @see cstar()*/
  double UnitCell::b3() const
  {
    return ra[2];
  }

  /** Get reciprocal lattice parameter
	@return beta1 :: lattice parameter \f$ \alpha^{*} \f$ (in radians)
	@see alphastar()*/
  double UnitCell::beta1() const
  {
    return ra[3];
  }

  /** Get reciprocal lattice parameter
	@return beta2 :: lattice parameter \f$ \beta^{*} \f$ (in radians)
	@see betastar()*/
  double UnitCell::beta2() const
  {
    return ra[4];
  }

  /** Get reciprocal lattice parameter
	@return beta3 :: lattice parameter \f$ \gamma^{*} \f$ (in radians)
	@see gammastar()*/
  double UnitCell::beta3() const
  {
    return ra[5];
  }

  /** Get reciprocal lattice parameter
  @return astar :: lattice parameter \f$ a^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
  @see b1()*/
  double UnitCell::astar() const
  {
    return ra[0];
  }

  /** Get reciprocal lattice parameter
	@return bstar :: lattice parameter \f$ b^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
	@see b2()*/
  double UnitCell::bstar() const
  {
    return ra[1];
  }

  /** Get reciprocal lattice parameter
  @return cstar :: lattice parameter \f$ c^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
  @see b3()*/
  double UnitCell::cstar() const
  {
    return ra[2];
  }

  /** Get reciprocal lattice parameter
  @return alphastar :: lattice parameter \f$ \alpha^{*} \f$ (in degrees)
  @see beta1()*/
  double UnitCell::alphastar() const
  {
    return ra[3]*rad2deg;
  }

  /** Get reciprocal lattice parameter
  @return  betastar:: lattice parameter \f$ \beta^{*} \f$ (in degrees)
  @see beta2()*/
  double UnitCell::betastar() const
  {
    return ra[4]*rad2deg;
  }

  /** Get reciprocal lattice parameter
  @return  gammastar:: lattice parameter \f$ \gamma^{*} \f$ (in degrees)
  @see beta3()*/
  double UnitCell::gammastar() const
  {
    return ra[5]*rad2deg;
  }

  /** Set lattice parameters
	@param _a, _b, _c, _alpha, _beta, _gamma :: lattice parameters\n
  @param angleunit :: units for angle, of type #AngleUnits . Default is degrees.
	*/
  void UnitCell::set(double _a, double _b, double _c, double _alpha, double _beta, double _gamma,const int angleunit)
  {
    da[0]=_a; da[1]=_b; da[2]=_c;
    if (angleunit==angDegrees)
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

  /** Set lattice parameter
  @param _a :: lattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )*/
  void UnitCell::seta(double _a)
  {
    da[0]=_a;
    recalculate();
  }

  /** Set lattice parameter
  @param _b :: lattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )*/
  void UnitCell::setb(double _b)
  {
    da[1]=_b;
    recalculate();
  }
 
  /** Set lattice parameter
  @param _c :: lattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )*/
  void UnitCell::setc(double _c)
  {
    da[2]=_c;
    recalculate();
  }

  /** Set lattice parameter
  @param _alpha :: lattice parameter \f$ \alpha \f$
  @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
  */
  void UnitCell::setalpha(double _alpha,const int angleunit)
  {
    if (angleunit==angDegrees)
      da[3]=deg2rad*_alpha;
    else 
      da[3]=_alpha;
    recalculate();
  }
 
  /** Set lattice parameter
  @param _beta :: lattice parameter \f$ \beta \f$
  @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
  */ 
  void UnitCell::setbeta(double _beta,const int angleunit)
  {
    if (angleunit==angDegrees)
      da[4]=deg2rad*_beta;
    else 
      da[4]=_beta;
    recalculate();
  }
      
  /** Set lattice parameter
  @param _gamma :: lattice parameter \f$ \gamma \f$
  @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
  */
  void UnitCell::setgamma(double _gamma,const int angleunit)
  {
    if (angleunit==angDegrees)
      da[5]=deg2rad*_gamma;
    else 
      da[5]=_gamma;
    recalculate();
  }

  /// Return d-spacing (\f$ \mbox{ \AA } \f$) for a given h,k,l coordinate
  double UnitCell::d(double h, double k, double l) const
  {
	  return 1./dstar(h,k,l);
  }

  /// Return d*=1/d (\f$ \mbox{ \AA }^{-1} \f$) for a given h,k,l coordinate
  double UnitCell::dstar(double h, double k, double l) const
  {
	  V3D Q(h,k,l); //create a V3D vector h,k,l
    Q=B*Q; //transform into $AA^-1$
	  return Q.norm();
  }

  /// Calculate the angle in degrees or radians between two reciprocal vectors (h1,k1,l1) and (h2,k2,l2)
	double UnitCell::recAngle(double h1, double k1, double l1, double h2, double k2, double l2, const int angleunit) const
  {
	  V3D Q1(h1,k1,l1),Q2(h2,k2,l2);
	  double E,ang;
    Q1=Gstar*Q1;
    E=Q1.scalar_prod(Q2);
	  ang=acos(E/dstar(h1,k1,l1)/dstar(h2,k2,l2));
    if (angleunit==angDegrees)
      return rad2deg*ang;
    else 
      return ang;    
  }

  /// Volume of the direct unit-cell
  double UnitCell::volume() const
  {
  	double volume=G.determinant();
  	return sqrt(volume);
  }

  /// Volume of the reciprocal lattice
  double UnitCell::recVolume() const
  {
  	double recvolume=Gstar.determinant();
  	return sqrt(recvolume);
  }

  /// Get the metric tensor
  /// @return G :: metric tensor
  const Geometry::MantidMat& UnitCell::getG() const
  {
    return G;
  }


	/// Get the reciprocal metric tensor
	/// @return Gstar :: metric tensor of the reciprocal lattice
  const Geometry::MantidMat& UnitCell::getGstar() const
  {
    return Gstar;
  }
	    
  /// Get the B-matrix
  /// @return B :: B matrix in Busing-Levy convention
  const Geometry::MantidMat& UnitCell::getB() const
  {
    return B;
  }

  /// Private function, called at initialization or whenever lattice parameters are changed
  void UnitCell::recalculate()
  {
	  calculateG();
	  calculateGstar();
	  calculateReciprocalLattice();
	  calculateB();
  }
      
  /// Private function to calculate #G matrix
  void UnitCell::calculateG()
  {
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

	/// Private function to calculate #Gstar matrix
  void UnitCell::calculateGstar()
  {
	  // Reciprocal metrix tensor is simply the inverse of the direct one
    Gstar=G;
    if (Gstar.Invert()==0)
			{throw std::range_error("UnitCell not properly initialized");}
    return;
  }


	/// Private function to calculate reciprocal lattice parameters  
  void UnitCell::calculateReciprocalLattice()
  {
    ra[0]=sqrt(Gstar[0][0]);//a*
    ra[1]=sqrt(Gstar[1][1]);//b*
    ra[2]=sqrt(Gstar[2][2]);//c*
    ra[3]=acos(Gstar[1][2]/ra[1]/ra[2]);//alpha* 
    ra[4]=acos(Gstar[0][2]/ra[0]/ra[2]);//beta* 
    ra[5]=acos(Gstar[0][1]/ra[0]/ra[1]);//gamma* 
  }
	
  /// Private function to calculate #B matrix 
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


  /// Recalculate lattice from reciprocal metric tensor (Gstar=transpose(UB)*UB)
  void UnitCell::recalculateFromGstar(MantidMat& NewGstar)
  {
    if (NewGstar[0][0]*NewGstar[1][1]*NewGstar[2][2]<=0.) throw std::invalid_argument("NewGstar");
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
