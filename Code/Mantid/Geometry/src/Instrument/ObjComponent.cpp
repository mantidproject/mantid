//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include <cfloat>

namespace Mantid
{
  namespace Geometry
  {

    /** Constructor for a parametrized ObjComponent
     * @param base: the base (un-parametrized) IComponent
     * @param map: pointer to the ParameterMap
     * */
    ObjComponent::ObjComponent(const IComponent* base, ParameterMap_const_sptr map)
    : Component(base,map), m_shape()
    {

    }


    /** Constructor
    *  @param name   The name of the component
    *  @param parent The Parent geometry object of this component
    */
    ObjComponent::ObjComponent(const std::string& name, IComponent* parent)
    : IObjComponent(), Component(name,parent), m_shape()
    {
    }

    /** Constructor
    *  @param name   The name of the component
    *  @param shape  A pointer to the object describing the shape of this component
    *  @param parent The Parent geometry object of this component
    */
    ObjComponent::ObjComponent(const std::string& name, boost::shared_ptr<Object> shape, IComponent* parent)
    : IObjComponent(), Component(name,parent), m_shape(shape)
    {
    }

    /// Copy constructor
    ObjComponent::ObjComponent(const ObjComponent& rhs) :
    IObjComponent(),Component(rhs), m_shape(rhs.m_shape)
    {
    }

    /// Destructor
    ObjComponent::~ObjComponent()
    {
    }


    /** Return the shape of the component
     */
    const boost::shared_ptr<const Object> ObjComponent::shape()const
    {
      if (isParametrized())
        return dynamic_cast<const ObjComponent*>(m_base)->m_shape;
      else
        return m_shape;
    }


    /// Does the point given lie within this object component?
    bool ObjComponent::isValid(const V3D& point) const
    {
      // If the form of this component is not defined, just treat as a point
      if (!shape()) return (this->getPos() == point);

      // Otherwise pass through the shifted point to the Object::isValid method
      V3D scaleFactor = V3D(1,1,1); //=this->getScaleFactorP();
      return shape()->isValid( factorOutComponentPosition(point) / scaleFactor )!=0;
    }

    /// Does the point given lie on the surface of this object component?
    bool ObjComponent::isOnSide(const V3D& point) const
    {
      // If the form of this component is not defined, just treat as a point
      if (!shape()) return (this->getPos() == point);
      // Otherwise pass through the shifted point to the Object::isOnSide method
      V3D scaleFactor = V3D(1,1,1); // = this->getScaleFactorP();
      return shape()->isOnSide( factorOutComponentPosition(point) / scaleFactor )!=0;
    }

    /** Checks whether the track given will pass through this Component.
    *  @param track The Track object to test (N.B. Will be modified if hits are found)
    *  @returns The number of track segments added (i.e. 1 if the track enters and exits the object once each)
    *  @throw NullPointerException if the underlying geometrical Object has not been set
    */
    int ObjComponent::interceptSurface(Track& track) const
    {
      // If the form of this component is not defined, throw NullPointerException
      if (!shape()) throw Kernel::Exception::NullPointerException("ObjComponent::interceptSurface","shape");

      //TODO: If scaling parameters are ever enabled, would they need need to be used here?
      V3D trkStart = factorOutComponentPosition(track.startPoint());
      V3D trkDirection = takeOutRotation(track.direction());

      Track probeTrack(trkStart, trkDirection);
      int intercepts = shape()->interceptSurface(probeTrack);

      Track::LType::const_iterator it;
      for (it = probeTrack.begin(); it != probeTrack.end(); ++it)
      {
        V3D in = it->entryPoint;
        this->getRotation().rotate(in);
        //use the scale factor
        in *= m_ScaleFactor;
        in += this->getPos();
        V3D out = it->exitPoint;
        this->getRotation().rotate(out);
        //use the scale factor
        out *= m_ScaleFactor;
        out += this->getPos();
        track.addLink(in,out,out.distance(track.startPoint()),this->getComponentID());
      }

      return intercepts;
    }

    /** Finds the approximate solid angle covered by the component when viewed from the point given
    *  @param observer The position from which the component is being viewed
    *  @returns The solid angle in steradians
    *  @throw NullPointerException if the underlying geometrical Object has not been set
    */
    double ObjComponent::solidAngle(const V3D& observer) const
    {
      // If the form of this component is not defined, throw NullPointerException
      if (!shape()) throw Kernel::Exception::NullPointerException("ObjComponent::solidAngle","shape");
      // Otherwise pass through the shifted point to the Object::solidAngle method
      V3D scaleFactor=this->getScaleFactorP();
      if((scaleFactor-V3D(1.0,1.0,1.0)).norm()<1e-12)
        return shape()->solidAngle( factorOutComponentPosition(observer) );
      else
        return shape()->solidAngle( factorOutComponentPosition(observer)*scaleFactor, scaleFactor );
    }

    /**
    * Given an input estimate of the axis aligned (AA) bounding box (BB), return an improved set of values.
    * The AA BB is determined in the frame of the object and the initial estimate will be transformed there.
    * The returned BB will be the frame of the ObjComponent and may not be optimal.
    * Takes input axis aligned bounding box max and min points and calculates the bounding box for the
    * object and returns them back in max and min points. Cached values used after first call.
    *
    * @param xmax :: Maximum value for the bounding box in x direction
    * @param ymax :: Maximum value for the bounding box in y direction
    * @param zmax :: Maximum value for the bounding box in z direction
    * @param xmin :: Minimum value for the bounding box in x direction
    * @param ymin :: Minimum value for the bounding box in y direction
    * @param zmin :: Minimum value for the bounding box in z direction
    */
    void ObjComponent::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const
    {
      if (!shape()) throw Kernel::Exception::NullPointerException("ObjComponent::getBoundingBox","shape");

      Geometry::V3D min(xmin,ymin,zmin),        max(xmax,ymax,zmax);

      min-=this->getPos();
      max-=this->getPos();
      // Scale
      min/=m_ScaleFactor;
      max/=m_ScaleFactor;

      //Rotation
      Geometry::Quat inverse(this->getRotation());
      inverse.inverse();
      // Find new BB
      inverse.rotateBB(min[0],min[1],min[2],max[0],max[1],max[2]);

      // pass bounds to getBoundingBox
      shape()->getBoundingBox(max[0],max[1],max[2],min[0],min[1],min[2]);

      //Apply scale factor
      min*=m_ScaleFactor;
      max*=m_ScaleFactor;
      // Re-rotate
      (this->getRotation()).rotateBB(min[0],min[1],min[2],max[0],max[1],max[2]);
      min+=this->getPos();
      max+=this->getPos();
      xmin=min[0];xmax=max[0];
      ymin=min[1];ymax=max[1];
      zmin=min[2];zmax=max[2];
      return;
    }

    /**
    * Get the bounding box for this object-component. The underlying shape has a bounding box defined in its own coorindate
    * system. This needs to be adjusted for the actual position and rotation of this ObjComponent.
    * @param absoluteBB [Out] The bounding box for this object component will be stored here.
    */
    void ObjComponent::getBoundingBox(BoundingBox& absoluteBB) const
    {
      // Start with the box in the shape's coordinates
      const BoundingBox & shapeBox = shape()->getBoundingBox();
      absoluteBB = BoundingBox(shapeBox);
      if ( shapeBox.isNull() ) return;
      // modify in place for speed
      // Scale
      absoluteBB.xMin() *= m_ScaleFactor.X();
      absoluteBB.xMax() *= m_ScaleFactor.X();
      absoluteBB.yMin() *= m_ScaleFactor.Y(); 
      absoluteBB.yMax() *= m_ScaleFactor.Y();
      absoluteBB.zMin() *= m_ScaleFactor.Z(); 
      absoluteBB.zMax() *= m_ScaleFactor.Z();
      // Rotate
      (this->getRotation()).rotateBB(absoluteBB.xMin(),absoluteBB.yMin(),absoluteBB.zMin(),
        absoluteBB.xMax(),absoluteBB.yMax(),absoluteBB.zMax());
      // Shift
      const V3D localPos = this->getPos();
      absoluteBB.xMin() += localPos.X(); 
      absoluteBB.xMax() += localPos.X();
      absoluteBB.yMin() += localPos.Y(); 
      absoluteBB.yMax() += localPos.Y();
      absoluteBB.zMin() += localPos.Z(); 
      absoluteBB.zMax() += localPos.Z();
    }

    /**
    * Try to find a point that lies within (or on) the object
    * @param point On exit, set to the point value (if found)
    * @return 1 if point found, 0 otherwise
    */
    int ObjComponent::getPointInObject(V3D& point) const
    {
      // If the form of this component is not defined, throw NullPointerException
      if (!shape()) throw Kernel::Exception::NullPointerException("ObjComponent::getPointInObject","shape");
      // Call the Object::getPointInObject method, which may give a point in Object coordinates
      int result= shape()->getPointInObject( point );
      // transform point back to component space
      if(result)
      {
        //Scale up
        V3D scaleFactor=this->getScaleFactorP();
        point*=scaleFactor;
        Quat Rotate = this->getRotation();
        Rotate.rotate(point);
        point+=this->getPos();
      }
      return result;
    }

    /// Find the point that's in the same place relative to the constituent geometrical Object
    /// if the position and rotation introduced by the Component is ignored
    const V3D ObjComponent::factorOutComponentPosition(const V3D& point) const
    {
      // First subtract the component's position, then undo the rotation
      return takeOutRotation( point - this->getPos() );
    }

    /// Rotates a point by the reverse of the component's rotation
    const V3D ObjComponent::takeOutRotation(V3D point) const
    {
      // Get the total rotation of this component and calculate the inverse (reverse rotation)
      Quat unRotate = this->getRotation();
      unRotate.inverse();
      // Now rotate our point by the angle calculated above
      unRotate.rotate(point);

      // Can not Consider scaling factor here as this transform used by solidAngle as well
      // as IsValid etc. While this would work for latter, breaks former
      //TODO: What is the answere here? ObjComponentTest needs these lines IN!

      //Consider scaling factor
      point/=m_ScaleFactor;
      //point/=getScaleFactorP();

      return point;
    }



    /** Get ScaleFactor of the object
    * @returns A vector of the scale factors (1,1,1) if not set
    */
    V3D ObjComponent::getScaleFactorP() const
    {
      if (isParametrized())
      {
        Parameter_sptr par = m_map->get(m_base,"sca");
        if (par)
        {
          //Return the parametrized scaling
          return par->value<V3D>();
        }
      }
      //Not parametrized; or, no parameter for "sca" aka scale
      return m_ScaleFactor;
    }


    /**
    * Draws the objcomponent, If the handler is not set then this function does nothing.
    */
    void ObjComponent::draw() const
    {
      if(Handle()==NULL)return;
      //Render the ObjComponent and then render the object
      Handle()->Render();
    }

    /**
    * Draws the Object
    */
    void ObjComponent::drawObject() const
    {
      if(shape()!=NULL)
        shape()->draw();
    }

    /**
    * Initializes the ObjComponent for rendering, this function should be called before rendering.
    */
    void ObjComponent::initDraw() const
    {
      if(Handle()==NULL)return;
      //Render the ObjComponent and then render the object
      if(shape()!=NULL)
        shape()->initDraw();
      Handle()->Initialize();
    }

  } // namespace Geometry
} // namespace Mantid
