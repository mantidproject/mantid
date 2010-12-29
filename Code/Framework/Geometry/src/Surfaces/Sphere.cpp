#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Support.h"
#include "MantidGeometry/Tolerance.h"

namespace Mantid
{

  namespace Geometry
  {

    Kernel::Logger& Sphere::PLog(Kernel::Logger::get("Sphere"));

    // The number of slices to use to approximate a sphere
    int Sphere::g_nslices = 5;

    // The number of slices to use to approximate a sphere
    int Sphere::g_nstacks = 5;

    Sphere::Sphere() : Quadratic(),
      Centre(0,0,0),Radius(0.0)
      /*!
      Default constructor 
      make sphere at the origin radius zero 
      */
    {
      setBaseEqn();
    }

    Sphere::Sphere(const Sphere &A) : 
    Quadratic(A),Centre(A.Centre),Radius(A.Radius)
      /*!
      Default Copy constructor 
      \param A :: Sphere to copy
      */
    { }

    Sphere*
      Sphere::clone() const
      /*!
      Makes a clone (implicit virtual copy constructor) 
      \return new (*this)
      */
    {
      return new Sphere(*this);
    }

    Sphere&
      Sphere::operator=(const Sphere &A) 
      /*!
      Default Assignment operator
      \param A :: Sphere to copy
      \return *this
      */
    {
      if (this!=&A)
      {
        Quadratic::operator=(A);
        Centre=A.Centre;
        Radius=A.Radius;
      }
      return *this;
    }

    Sphere::~Sphere()
      /*!
      Destructor
      */
    {}

    int
      Sphere::setSurface(const std::string& Pstr)
      /*! 
      Processes a standard MCNPX cone string    
      Recall that cones can only be specified on an axis
      Valid input is: 
      - so radius 
      - s cen_x cen_y cen_z radius
      - sx - cen_x radius
      \return : 0 on success, neg of failure 
      */
    {
      std::string Line=Pstr;
      std::string item;
      if (!StrFunc::section(Line,item) || 
        tolower(item[0])!='s' || item.length()>2)
        return -1;

      double cent[3]={0,0,0};
      double R;
      if (item.length()==2)       // sx/sy/sz
      {
        if (tolower(item[1])!='o')
        {
          const int pType=static_cast<int>(tolower(item[1])-'x');
          if (pType<0 || pType>2)
            return -3;
          if (!StrFunc::section(Line,cent[pType]))
            return -4;
        }
      }
      else if (item.length()==1)
      {
        int index;
        for(index=0;index<3 && StrFunc::section(Line,cent[index]);
          index++);
          if (index!=3)
            return -5;
      }
      else
        return -6;
      if (!StrFunc::section(Line,R))
        return -7;

      Centre=Geometry::V3D(cent);
      Radius=R;
      setBaseEqn();
      return 0;
    } 


    int
      Sphere::side(const Geometry::V3D& Pt) const
      /*!
      Calculate where the point Pt is relative to the 
      sphere.
      \param Pt :: Point to test
      \retval -1 :: Pt within sphere
      \retval 0 :: point on the surface (within CTolerance)
      \retval 1 :: Pt outside the sphere 
      */
    {
      const double displace = centreToPoint(Pt) - Radius;
      //MG:  Surface test  - This does not use onSurface since it would double the amount of
      // computation if the object is not on the surface which is most likely
      if( std::abs(displace) < Tolerance )
      {
        return 0;
      }
      return (displace > 0.0) ? 1 : -1;
    }

    int
      Sphere::onSurface(const Geometry::V3D& Pt) const
      /*!
      Calculate if the point Pt on the surface of the sphere
      (within tolerance CTolerance)
      \param Pt :: Point to check
      \return 1 :: on the surfacae or 0 if not.
      */
    {
      if( distance(Pt) > Tolerance )
      {
        return 0;
      }
      return 1;
    }

    double
      Sphere::distance(const Geometry::V3D& Pt) const
      /*! 
      Determine the shortest distance from the Surface 
      to the Point. 
      \param Pt :: Point to calculate distance from
      \return distance (Positive only)
      */
    {
      const Geometry::V3D disp_vec = Pt - Centre;
      return std::abs(disp_vec.norm() - Radius);
    }


    void
      Sphere::displace(const Geometry::V3D& Pt) 
      /*!
      Apply a shift of the centre
      \param Pt :: distance to add to the centre
      */
    {
      Centre+=Pt;
      Quadratic::displace(Pt);
      return;
    }

    void
      Sphere::rotate(const Geometry::Matrix<double>& MA) 
      /*!
      Apply a Rotation matrix
      \param MA :: matrix to rotate by
      */
    {
      Centre.rotate(MA);
      Quadratic::rotate(MA);
      return;
    }

    double Sphere::centreToPoint(const V3D & pt) const
    {
      /*!
      Compute the distance between the given point and the centre of the sphere
      \param pt :: The chosen point 
      */
      const Geometry::V3D displace_vec = pt - Centre;
      return displace_vec.norm();
    }

    void 
      Sphere::setCentre(const Geometry::V3D& A)
      /*!
      Set the centre point
      \param A :: New Centre Point
      */
    {
      Centre=A;
      setBaseEqn();
      return;
    }

    void 
      Sphere::setBaseEqn()
      /*!
      Sets an equation of type (general sphere)
      \f[ x^2+y^2+z^2+Gx+Hy+Jz+K=0 \f]
      */
    {
      BaseEqn[0]=1.0;     // A x^2
      BaseEqn[1]=1.0;     // B y^2
      BaseEqn[2]=1.0;     // C z^2 
      BaseEqn[3]=0.0;     // D xy
      BaseEqn[4]=0.0;     // E xz
      BaseEqn[5]=0.0;     // F yz
      BaseEqn[6]= -2.0*Centre[0];     // G x
      BaseEqn[7]= -2.0*Centre[1];     // H y
      BaseEqn[8]= -2.0*Centre[2];     // J z
      BaseEqn[9]= Centre.scalar_prod(Centre)-Radius*Radius;        // K const
      return;
    }


    void 
      Sphere::write(std::ostream& OX) const
      /*! 
      Object of write is to output a MCNPX plane info 
      \param OX :: Output stream (required for multiple std::endl)  
      \todo (Needs precision) 
      */
    {
      std::ostringstream cx;
      Quadratic::writeHeader(cx);
      cx.precision(Surface::Nprecision);
      if (Centre.distance(Geometry::V3D(0,0,0))<Tolerance)
      {
        cx<<"so "<<Radius;
      }
      else
      {
        cx<<"s "<<Centre<<" "<<Radius;
      }
      StrFunc::writeMCNPX(cx.str(),OX);
      return;
    }
    /**
    * Calculates the bounding box for the sphere and returns the bounding box values.
    * @param xmax :: input and output for the bounding box X axis max value
    * @param ymax :: input and output for the bounding box Y axis max value
    * @param zmax :: input and output for the bounding box Z axis max value
    * @param xmin :: input and output for the bounding box X axis min value
    * @param ymin :: input and output for the bounding box Y axis min value
    * @param zmin :: input and output for the bounding box Z axis min value
    */
    void Sphere::getBoundingBox(double& xmax,double &ymax,double &zmax,double &xmin,double &ymin,double &zmin)
    {
      xmax=Centre[0]+Radius;
      ymax=Centre[1]+Radius;
      zmax=Centre[2]+Radius;
      xmin=Centre[0]-Radius;
      ymin=Centre[1]-Radius;
      zmin=Centre[2]-Radius;
    }

  }  // NAMESPACE Geometry

}  // NAMESPACE Mantid
