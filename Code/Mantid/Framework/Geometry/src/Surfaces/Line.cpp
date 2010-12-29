#include "MantidGeometry/Surfaces/Line.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Support.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Tolerance.h"

namespace Mantid
{

  namespace Geometry
  {

    Kernel::Logger& Line::PLog(Kernel::Logger::get("Line"));

    Line::Line() : Origin(),Direct()
      /*!
      Constructor
      */
    {}

    Line::Line(const Geometry::V3D& O,const Geometry::V3D& D) 
      : Origin(O),Direct(D)
      /*!
      Constructor
      */
    {
      Direct.normalize();
    }

    Line::Line(const Line& A) : 
    Origin(A.Origin),Direct(A.Direct)
      /*!
      Copy Constructor
      \param A :: Line to copy
      */
    {}

    Line*
      Line::clone() const
      /*! 
      Virtual copy constructor (not currently used)
      \return the cloned line
      */
    {
      return new Line(*this);
    }

    Line&
      Line::operator=(const Line& A)
      /*!
      Assignment operator
      \param A :: Line to copy
      \return *this
      */
    {
      if (this!=&A)
      {
        Origin=A.Origin;
        Direct=A.Direct;
      }
      return *this;
    }

    Line::~Line()
      /*!
      Destructor
      */
    {}

    Geometry::V3D
      Line::getPoint(const double lambda) const
      /*!
      Return the point on the line given lambda*direction
      \param lambda :: line position scalar
      \returns \f$ \vec{O}+ \lambda \vec{D} \f$
      */
    {
      return Origin+Direct*lambda;
    }

    double
      Line::distance(const Geometry::V3D& A) const
      /*!
      Distance of a point from the line
      \param A :: test Point
      \returns absolute distance (not signed)
      */
    {
      const double lambda=Direct.scalar_prod(A-Origin);
      Geometry::V3D L=getPoint(lambda);
      L-=A;
      return L.norm();
    }

    int 
      Line::isValid(const Geometry::V3D& A) const
      /*! 
      Calculate is point is on line by using distance to determine
      if the point is within Tolerance of the line
      \param A :: Point to test
      \retval 1 : the point is on the line
      \retval 0 : Point is not on the line
      */ 
    {
      return (distance(A)>Tolerance) ? 0 : 1;
    }

    void
      Line::rotate(const Geometry::Matrix<double>& MA) 
      /*!
      Applies the rotation matrix to the 
      object.
      \param MA :: Rotation Matrix
      */
    {
      Origin.rotate(MA);
      Direct.rotate(MA);
      Direct.normalize();
      return;
    }

    void 
      Line::displace(const Geometry::V3D& Pt)
      /*! 
      Apply a displacement Pt 
      \param Pt :: Point value of the displacement
      */ 
    {
      Origin+=Pt;
      return;
    }

    int
      Line::lambdaPair(const int ix,const std::pair<
      std::complex<double>,std::complex<double> >& SQ,
      std::vector<Geometry::V3D>& PntOut) const
      /*! 
      Helper function to decide which roots to take.
      The assumption is that lambda has been solved by quadratic
      equation and we require the points that correspond to these
      values.
      Note: have changed this so that only positive roots are returned.
      This makes the quadratic solutions consistent with the ones returned
      when asking if a line hits a plane. It is not clear if some other use
      cases exist.
      \param ix : number of solutions in SQ (0,1,2)
      \param SQ : solutions to lambda (the distance along the line
      \param PntOut : Output vector of points (added to)
      \return Number of real unique points found.
      */
    { 
      // check results
      if (ix<1)
        return 0;

      int nCnt(0);          // number of good points

      Geometry::V3D Ans;
      if (SQ.first.imag()==0.0 && SQ.first.real()>=0.0) // +ve roots only
      {
        const double lambda=SQ.first.real();
        Geometry::V3D Ans=getPoint(lambda);
        PntOut.push_back(Ans);
        if (ix<2)        // only one unique root.
          return 1;
        nCnt=1;
      }
      if (SQ.second.imag()==0.0 && SQ.second.real()>=0.0) // +ve roots only
      {
        const double lambda=SQ.second.real();
        if (!nCnt)   // first point wasn't good.
        {
          PntOut.push_back(getPoint(lambda));
          return 1;
        }
        Geometry::V3D Ans2=getPoint(lambda);
        // If points too close return only 1 item.
        if (Ans.distance(Ans2)<Tolerance)
          return 1;

        PntOut.push_back(Ans2);
        return 2;
      }
      return 0; //both point imaginary
    }

    int
      Line::intersect(std::vector<Geometry::V3D>& VecOut,
      const Quadratic& Sur) const
      /*!
      For the line that intersects the surfaces 
      add the point(s) to the VecOut, return number of points
      added. It does not check the points for validity.
      \param VecOut :: intersection points of the line and surface
      \param Sur :: Surface to intersect with a line
      \return Number of points found. 
      */
    {
      const std::vector<double> BN=Sur.copyBaseEqn();
      const double a(Origin[0]),b(Origin[1]),c(Origin[2]);
      const double d(Direct[0]),e(Direct[1]),f(Direct[2]);
      double Coef[3];
      Coef[0] = BN[0]*d*d+BN[1]*e*e+BN[2]*f*f+
        BN[3]*d*e+BN[4]*d*f+BN[5]*e*f;
      Coef[1] = 2*BN[0]*a*d+2*BN[1]*b*e+2*BN[2]*c*f+
        BN[3]*(a*e+b*d)+BN[4]*(a*f+c*d)+BN[5]*(b*f+c*e)+
        BN[6]*d+BN[7]*e+BN[8]*f;
      Coef[2] = BN[0]*a*a+BN[1]*b*b+BN[2]*c*c+
        BN[3]*a*b+BN[4]*a*c+BN[5]*b*c+BN[6]*a+BN[7]*b+
        BN[8]*c+BN[9];

      std::pair<std::complex<double>,std::complex<double> > SQ;
      const int ix=solveQuadratic(Coef,SQ);
      return lambdaPair(ix,SQ,VecOut);
    }  

    int 
      Line::intersect(std::vector<Geometry::V3D>& PntOut ,const Plane& Pln) const
      /*! 
      For the line that intersects the cylinder generate 
      add the point to the VecOut, return number of points
      added. It does not check the points for validity. 

      \param PntOut :: Vector of points found by the line/cylinder intersection
      \param Pln :: Plane for intersect
      \return Number of points found by intersection
      */
    {

      const double OdotN=Origin.scalar_prod(Pln.getNormal());
      const double DdotN=Direct.scalar_prod(Pln.getNormal());
      if (fabs(DdotN)<Tolerance)     // Plane and line parallel
        return 0;
      const double u=(Pln.getDistance()-OdotN)/DdotN;
      if (u<=0)
        return 0;
      PntOut.push_back(getPoint(u));
      return 1;
    }

    int 
      Line::intersect(std::vector<Geometry::V3D>& PntOut ,const Cylinder& Cyl) const
      /*! 
      For the line that intersects the cylinder generate 
      add the point to the VecOut, return number of points
      added. It does not check the points for validity. 

      \param PntOut :: Vector of points found by the line/cylinder intersection
      \param Cyl :: Cylinder to intersect line with
      \return Number of points found by intersection
      */
    {
      const Geometry::V3D Cent=Cyl.getCentre();
      const Geometry::V3D Ax=Origin-Cent;
      const Geometry::V3D N= Cyl.getNormal();
      const double R=Cyl.getRadius();
      const double vDn = N.scalar_prod(Direct);
      const double vDA = N.scalar_prod(Ax);
      // First solve the equation of intersection
      double C[3];
      C[0]= 1.0-(vDn*vDn);
      C[1]= 2.0*(Ax.scalar_prod(Direct)-vDA*vDn);
      C[2]= Ax.scalar_prod(Ax)-(R*R+vDA*vDA);
      std::pair<std::complex<double>,std::complex<double> > SQ;
      const int ix = solveQuadratic(C,SQ);
      // This takes the centre displacement into account:
      return lambdaPair(ix,SQ,PntOut);  
    }

    int 
      Line::intersect(std::vector<Geometry::V3D>& PntOut ,const Sphere& Sph) const
      /*! 
      For the line that intersects the cylinder generate 
      add the point to the VecOut, return number of points
      added. It does not check the points for validity. 

      \param PntOut :: Vector of points found by the line/sphere intersection
      \param Sph :: Sphere to intersect line with
      \returns Number of points found by intersection
      */
    {
      // Nasty stripping of useful stuff from sphere
      const Geometry::V3D Ax=Origin-Sph.getCentre();
      const double R=Sph.getRadius();
      // First solve the equation of intersection
      double C[3];
      C[0]=1;
      C[1]=2.0*Ax.scalar_prod(Direct);
      C[2]=Ax.scalar_prod(Ax)-R*R;
      std::pair<std::complex<double>,std::complex<double> > SQ;
      const int ix = solveQuadratic(C,SQ);
      return lambdaPair(ix,SQ,PntOut);
    }

    //SETING

    int 
      Line::setLine(const Geometry::V3D& O,const Geometry::V3D& D) 
      /*!
      sets the line given the Origne and direction
      \param O :: origin
      \param D :: direction
      \retval  0 ::  Direction == 0 ie no line
      \retval 1 :: on success
      */
    {
      if (D.nullVector())
        return 0;
      Origin=O;
      Direct=D;
      Direct.normalize();
      return 1;
    }

    void
      Line::print() const
      /*!
      Print statement for debugging
      */
    {
      std::cout<<"Line == "<<Origin<<" :: "<<Direct<<std::endl;
      return;
    }


  }   // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
