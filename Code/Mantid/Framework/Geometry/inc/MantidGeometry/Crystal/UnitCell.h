#ifndef MANTID_GEOMETRY_UNITCELL_H_
#define MANTID_GEOMETRY_UNITCELL_H_
#include <MantidKernel/System.h>
#include <MantidGeometry/Math/Matrix.h>
#include <cmath>
#include <vector>

namespace Mantid
{
namespace Geometry
{  

  /** UnitCell : Class to implement unit cell of crystals, based on code
   *     by L. Chapon. It does not contain information about lattice orientation
   * 
   * @author Andrei Savici, SNS, ORNL
   * @date 2011-03-23 
   */
  const double deg2rad=M_PI/180.;
  const double rad2deg=180./M_PI;
  enum AngleUnits {Degrees,Radians};

  class DLLExport UnitCell
  {
    public:
      UnitCell(); //Default constructor
      UnitCell(const UnitCell& other); //Copy constructor
      UnitCell(const double _a,const double _b,const double _c); //Constructor a, b, c, alpha=90degrees, beta=90 degrees, gamma=90 degrees
      UnitCell(const double _a,const double _b,const double _c,const double _alpha,const double _beta,const double _gamma,const int Unit=Degrees); //Constructor a, b, c, alpha, beta, gamma, AngleUnits (degrees or radians)
      ~UnitCell(); //Destructor

      // Get and set lattice parameters
	    // Direct lattice parameters, angle in radians.
	    double a1() const {return da[0];}
	    double a2() const {return da[1];}
	    double a3() const {return da[2];}
	    double alpha1() const {return da[3];}
	    double alpha2() const {return da[4];}
	    double alpha3() const {return da[5];}
	    // Direct lattice parameters, angle in degrees.
	    double a() const {return da[0];}
	    double b() const {return da[1];}
	    double c() const {return da[2];}
	    double alpha() const {return da[3]*rad2deg;}
	    double beta() const {return da[4]*rad2deg;}
	    double gamma() const {return da[5]*rad2deg;}
	    // Reciprocal lattice parameters, angle in radians.
	    double b1() const {return ra[0];}
	    double b2() const {return ra[1];}
	    double b3() const {return ra[2];}
	    double beta1() const {return ra[3];}
	    double beta2() const {return ra[4];}
	    double beta3() const {return ra[5];}
	    // Reciprocal lattice parameters, angle in degrees.
	    double astar() const {return ra[0];}
	    double bstar() const {return ra[1];}
	    double cstar() const {return ra[2];}
	    double alphastar() const {return ra[3]*rad2deg;}
	    double betastar() const {return ra[4]*rad2deg;}
	    double gammastar() const {return ra[5]*rad2deg;}
      // Set lattice
      void set(double _a, double _b, double _c, double _alpha, double _beta, double _gamma,const int Unit=Degrees);
      void seta(double _a);
      void setb(double _b);
      void setc(double _c);
      void setalpha(double _alpha,const int Unit=Degrees);
      void setbeta(double _beta,const int Unit=Degrees);
      void setgamma(double _gamma,const int Unit=Degrees);

      // Access private variables
	    const Geometry::MantidMat& getG() const {return G;} //Get the metric tensor
	    const Geometry::MantidMat& getGstar() const { return Gstar;} //Get the reciprocal metric tensor
	    const Geometry::MantidMat& getB() const {return B;} //Get the B-matrix
	
      // Calculate things about lattice and vectors
	    double d(double h, double k, double l) const; //Return d-spacing ($\AA$) for a given h,k,l coordinate
	    double dstar(double h,double k, double l) const; //Return d*=1/d ($\AA^{-1}$) for a given h,k,l coordinate
	    double recAngle(double h1, double k1, double l1, double h2, double k2, double l2, const int Unit=Degrees) const; //Calculate the angle in degrees or radians between two reciprocal vectors (h1,k1,l1) and (h2,k2,l2)
	  	double volume()const; //Volume of the direct unit-cell
	    double recVolume() const; //Volume of the reciprocal lattice

      // Recalculate lattice from reciprocal metric tensor (Gstar=transpose(UB)*UB)
      void recalculateFromGstar(Geometry::Matrix<double>& NewGstar);

    private:	    
      std::vector <double> da; //Lattice parameter a,b,c,alpha,beta,gamma (in $\AA$ and radians)
	    std::vector <double> ra; //Reciprocal lattice parameters (in $\AA^{-1}$ and radians)

	    MantidMat G; //Metric tensor
	    MantidMat Gstar; //Reciprocal lattice tensor
	  	MantidMat B; //B matrix for a right-handed coordinate system 

      void recalculate();
	    void calculateG();
	    void calculateGstar();
	    void calculateReciprocalLattice();
	    void calculateB();   
  };
} // namespace Mantid
} // namespace Geometry

#endif  /* MANTID_GEOMETRY_UNITCELL_H_ */
