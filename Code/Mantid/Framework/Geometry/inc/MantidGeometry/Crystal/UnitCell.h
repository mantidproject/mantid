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
  /** @class UnitCell UnitCell.h Geometry/Crystal/Component.h
    Class to implement unit cell of crystals. 
    It is based on code by Laurent Chapon. It does not contain information about lattice orientation.
    See documentation about UB matrix in the Mantid repository.\n
    For documentation purposes, units for lengths are assumed to be \f$ \mbox{ \AA } \f$, and for reciprocal lattice lengths
    \f$ \mbox{ \AA }^{-1} \f$,  but can be anything, as long as used consistently. Note that the convention used for
    reciprocal lattice follows the one in International Tables for Crystallography, meaning that for an orthogonal lattice
    \f$ a^{*} = 1/a \f$ , not   \f$ a^{*} = 2 \pi /a \f$

    References:
      - International Tables for Crystallography (2006). Vol. B, ch. 1.1, pp. 2-9
      - W. R. Busing and H. A. Levy, Angle calculations for 3- and 4-circle X-ray and neutron diffractometers - Acta Cryst. (1967). 22, 457-464


    @author Andrei Savici, SNS, ORNL
    @date 2011-03-23     
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
   */

  /// Degrees to radians conversion factor
  const double deg2rad=M_PI/180.; 
  /// Radians to degrees conversion factor
  const double rad2deg=180./M_PI;
  /// Flag for angle units used in UnitCell class
  enum AngleUnits {latDegrees/** Degrees*/
    ,latRadians/** Radians*/};

  class DLLExport UnitCell
  {
    public:
      /** Default constructor. 
       \f$ a = b = c =  1 \mbox{\AA, } \alpha = \beta = \gamma = 90^\circ \f$ */
      UnitCell(); 
      /** Copy constructor
       @param other :: The UnitCell from which to copy lattice parameters
      */
      UnitCell(const UnitCell& other); 
      /** Constructor
        @param _a, _b, _c :: lattice parameters \f$ a, b, c \f$ \n
       with \f$\alpha = \beta = \gamma = 90^\circ \f$*/
      UnitCell(const double _a,const double _b,const double _c); 
      /** Constructor
       * @param _a, _b, _c, _alpha, _beta, _gamma :: lattice parameters\n
       * @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
       */
      UnitCell(const double _a,const double _b,const double _c,const double _alpha,const double _beta,const double _gamma,const int angleunit=latDegrees);
      /// Destructor
      ~UnitCell(); 

      // Get and set lattice parameters
	    // Direct lattice parameters, angle in radians.
      /** Get lattice parameter
       @return a1 :: lattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )
       @see a()*/
	    double a1() const;
      /** Get lattice parameter
       @return a2 :: lattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )
	     @see b()*/
	    double a2() const;
      /** Get lattice parameter
       @return a3 :: lattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )
       @see c()*/
	    double a3() const;
      /** Get lattice parameter
       @return alpha1 :: lattice parameter \f$ \alpha \f$ (in radians)
	     @see alpha()*/
	    double alpha1() const;
	    /** Get lattice parameter
       @return alpha2 :: lattice parameter \f$ \beta \f$ (in radians)
	     @see beta()*/
	    double alpha2() const;
      /** Get lattice parameter
       @return alpha3 :: lattice parameter \f$ \gamma \f$ (in radians)
       @see gamma()*/
	    double alpha3() const;
	    // Direct lattice parameters, angle in degrees.
      /** Get lattice parameter
       @return a :: lattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )
       @see a1()*/
	    double a() const;
	    /** Get lattice parameter
       @return b :: lattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )
       @see a2()*/
	    double b() const;
      /** Get lattice parameter
       @return c :: lattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )
       @see a3()*/
	    double c() const;
      /** Get lattice parameter
       @return alpha :: lattice parameter \f$ \alpha \f$ (in degrees)
       @see alpha1()*/
	    double alpha() const;
	    /** Get lattice parameter
       @return beta :: lattice parameter \f$ \beta \f$ (in degrees)
       @see alpha2()*/
	    double beta() const;
	    /** Get lattice parameter
       @return gamma :: lattice parameter \f$ \gamma \f$ (in degrees)
       @see alpha3()*/
	    double gamma() const;
	    // Reciprocal lattice parameters, angle in radians.
	    /** Get reciprocal lattice parameter
      @return b1 :: lattice parameter \f$ a^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
      @see astar()*/
	    double b1() const;
      /** Get reciprocal lattice parameter
      @return b2 :: lattice parameter \f$ b^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
      @see bstar()*/
	    double b2() const;
      /** Get reciprocal lattice parameter
      @return b3 :: lattice parameter \f$ c^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
      @see cstar()*/
	    double b3() const;
	    /** Get reciprocal lattice parameter
	    @return beta1 :: lattice parameter \f$ \alpha^{*} \f$ (in radians)
	    @see alphastar()*/
	    double beta1() const;
	    /** Get reciprocal lattice parameter
	    @return beta2 :: lattice parameter \f$ \beta^{*} \f$ (in radians)
	    @see betastar()*/
	    double beta2() const;
	    /** Get reciprocal lattice parameter
	    @return beta3 :: lattice parameter \f$ \gamma^{*} \f$ (in radians)
	    @see gammastar()*/
	    double beta3() const;
	    // Reciprocal lattice parameters, angle in degrees.
	    /** Get reciprocal lattice parameter
      @return astar :: lattice parameter \f$ a^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
      @see b1()*/
	    double astar() const;
	    /** Get reciprocal lattice parameter
	    @return bstar :: lattice parameter \f$ b^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
	    @see b2()*/
	    double bstar() const;
      /** Get reciprocal lattice parameter
      @return cstar :: lattice parameter \f$ c^{*} \f$ (in \f$ \mbox{\AA}^{-1} \f$ )
      @see b3()*/
	    double cstar() const;
	    /** Get reciprocal lattice parameter
      @return alphastar :: lattice parameter \f$ \alpha^{*} \f$ (in degrees)
      @see beta1()*/
	    double alphastar() const;
      /** Get reciprocal lattice parameter
      @return  betastar:: lattice parameter \f$ \beta^{*} \f$ (in degrees)
      @see beta2()*/
	    double betastar() const;
      /** Get reciprocal lattice parameter
      @return  gammastar:: lattice parameter \f$ \gamma^{*} \f$ (in degrees)
      @see beta3()*/
	    double gammastar() const;
      // Set lattice
	    /** Set lattice parameters
	     * @param _a, _b, _c, _alpha, _beta, _gamma :: lattice parameters\n
       * @param angleunit :: units for angle, of type #AngleUnits . Default is degrees.
	     */
      void set(double _a, double _b, double _c, double _alpha, double _beta, double _gamma,const int angleunit=latDegrees);
      /** Set lattice parameter
      @param _a :: lattice parameter \f$ a \f$ (in \f$ \mbox{\AA} \f$ )*/
      void seta(double _a);
      /** Set lattice parameter
      @param _b :: lattice parameter \f$ b \f$ (in \f$ \mbox{\AA} \f$ )*/
      void setb(double _b);
      /** Set lattice parameter
      @param _c :: lattice parameter \f$ c \f$ (in \f$ \mbox{\AA} \f$ )*/
      void setc(double _c);
      /** Set lattice parameter
      @param _alpha :: lattice parameter \f$ \alpha \f$
      @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
       */
      void setalpha(double _alpha,const int angleunit=latDegrees);
      /** Set lattice parameter
      @param _beta :: lattice parameter \f$ \beta \f$
      @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
       */
      void setbeta(double _beta,const int angleunit=latDegrees);
      /** Set lattice parameter
      @param _gamma :: lattice parameter \f$ \gamma \f$
      @param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
       */
      void setgamma(double _gamma,const int angleunit=latDegrees);

      // Access private variables
      /// Get the metric tensor
      /// @return G :: metric tensor
	    const Geometry::MantidMat& getG() const;
	    /// Get the reciprocal metric tensor
	    /// @return Gstar :: metric tensor of the reciprocal lattice
      const Geometry::MantidMat& getGstar() const;
	    /// Get the B-matrix
      /// @return B :: B matrix in Busing-Levy convention
      const Geometry::MantidMat& getB() const;
	
      // Calculate things about lattice and vectors
      /// Return d-spacing (\f$ \mbox{ \AA } \f$) for a given h,k,l coordinate
	    double d(double h, double k, double l) const; 
	    /// Return d*=1/d (\f$ \mbox{ \AA }^{-1} \f$) for a given h,k,l coordinate
      double dstar(double h,double k, double l) const; 
	    /// Calculate the angle in degrees or radians between two reciprocal vectors (h1,k1,l1) and (h2,k2,l2)
      double recAngle(double h1, double k1, double l1, double h2, double k2, double l2, const int angleunit=latDegrees) const;
	  	/// Volume of the direct unit-cell
      double volume()const; 
      /// Volume of the reciprocal lattice
	    double recVolume() const;

      /// Recalculate lattice from reciprocal metric tensor (Gstar=transpose(UB)*UB)
      void recalculateFromGstar(Geometry::Matrix<double>& NewGstar);

    private:	    
      /// Lattice parameter a,b,c,alpha,beta,gamma (in \f$ \mbox{ \AA } \f$ and radians)
      std::vector <double> da; 
	    /// Reciprocal lattice parameters (in \f$ \mbox{ \AA }^{-1} \f$ and radians)
      std::vector <double> ra; 
      /** Metric tensor
       \f[ \left( \begin{array}{ccc}
        aa & ab\cos(\gamma) & ac\cos(\beta) \\
        ab\cos(\gamma) & bb & bc\cos(\alpha) \\
        ac\cos(\beta) & bc\cos(\alpha) & cc \end{array} \right) \f]
       */
	    MantidMat G;
	    /** Reciprocal lattice tensor
	     *\f[ \left( \begin{array}{ccc}
        a^*a^* & a^*b^*\cos(\gamma^*) & a^*c^*\cos(\beta^*) \\
        a^*b^*\cos(\gamma^*) & b^*b^* & b^*c^*\cos(\alpha^*) \\
        a^*c^*\cos(\beta^*) & b^*c^*\cos(\alpha^*) & c^*c^* \end{array} \right) \f]
	     */
      MantidMat Gstar; 
	  	/** B matrix for a right-handed coordinate system, in Busing-Levy convention
	  	 \f[ \left( \begin{array}{ccc}
        a^* & b^*\cos(\gamma^*) & c^*\cos(\beta^*) \\
        0 & b^*\sin(\gamma^*) & -c^*\sin(\beta^*)\cos(\alpha) \\
        0 & 0 & 1/c \end{array} \right) \f]
	  	 */
      MantidMat B;  

      /// Private function, called at initialization or whenever lattice parameters are changed.
      ///Calls #calculateG() , #calculateGstar(),  #calculateReciprocalLattice(), #calculateB()
      void recalculate();
      /// Private function to calculate #G matrix
	    void calculateG();
	    /// Private function to calculate #Gstar matrix
	    void calculateGstar();
	    /// Private function to calculate reciprocal lattice parameters
	    void calculateReciprocalLattice();
	    /// Private function to calculate #B matrix
	    void calculateB();   
  };
} // namespace Mantid
} // namespace Geometry

#endif  /* MANTID_GEOMETRY_UNITCELL_H_ */
