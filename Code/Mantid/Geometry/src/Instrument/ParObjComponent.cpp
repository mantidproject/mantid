//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/ParObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include <cfloat>

namespace Mantid
{
  namespace Geometry
  {
    /**  Constructor.
    *   @param base A pointer to the base ObjComponent
    *   @param map A pointer to the parameter map 
    */
    ParObjComponent::ParObjComponent(const ObjComponent* base, const ParameterMap& map)
        :IObjComponent(), ParametrizedComponent(base,map)
    {
    }
    /**  Copy constructor.
    *   @param comp A reference to the component to copy from
    */
    ParObjComponent::ParObjComponent(const ParObjComponent& comp)
      :ParametrizedComponent(comp)
    {
    }

    /// Does the point given lie within this object component?
    bool ParObjComponent::isValid(const V3D& point) const
    {
      // If the form of this component is not defined, just treat as a point
      if (!shape()) return (this->getPos() == point);
      // Otherwise pass through the shifted point to the Object::isValid method
      V3D scaleFactor=this->getScaleFactorP();
      return shape()->isValid( factorOutComponentPosition(point) / scaleFactor )!=0;
    }

    /// Does the point given lie on the surface of this object component?
    bool ParObjComponent::isOnSide(const V3D& point) const
    {
      // If the form of this component is not defined, just treat as a point
      if (!shape()) return (this->getPos() == point);
      // Otherwise pass through the shifted point to the Object::isOnSide method
      V3D scaleFactor=this->getScaleFactorP();
      return shape()->isOnSide( factorOutComponentPosition(point) / scaleFactor )!=0;
    }

    /** Checks whether the track given will pass through this Component.
    *  @param track The Track object to test (N.B. Will be modified if hits are found)
    *  @returns The number of track segments added (i.e. 1 if the track enters and exits the object once each)
    *  @throw NullPointerException if the underlying geometrical Object has not been set
    */
    int ParObjComponent::interceptSurface(Track& track) const
    {
      // If the form of this component is not defined, throw NullPointerException
      if (!shape()) throw Kernel::Exception::NullPointerException("ParObjComponent::interceptSurface","shape");

      V3D scaleFactor=this->getScaleFactorP();
      V3D trkStart = factorOutComponentPosition(track.startPoint()) / scaleFactor;
      V3D trkDirection = takeOutRotation(track.direction()) / scaleFactor;

      Track probeTrack(trkStart, trkDirection);
      int intercepts = shape()->interceptSurface(probeTrack);

      Track::LType::const_iterator it;
      for (it = probeTrack.begin(); it < probeTrack.end(); ++it)
      {
        V3D in = it->entryPoint;
        this->getRotation().rotate(in);
        //use the scale factor
        in *= scaleFactor;
        //
        in += this->getPos();
        V3D out = it->exitPoint;
        this->getRotation().rotate(out);
        //use the scale factor
        out *= scaleFactor;
        //
        out += this->getPos();
        track.addLink(in,out,it->distFromStart, this->getComponentID());
      }

      return intercepts;
    }

    /** Finds the approximate solid angle covered by the component when viewed from the point given
    *  @param observer The position from which the component is being viewed
    *  @returns The solid angle in steradians
    *  @throw NullPointerException if the underlying geometrical Object has not been set
    */
    double ParObjComponent::solidAngle(const V3D& observer) const
    {
      // If the form of this component is not defined, throw NullPointerException
      if (!shape()) throw Kernel::Exception::NullPointerException("ParObjComponent::solidAngle","shape");
      // Otherwise pass through the shifted point to the Object::solidAngle method
      V3D scaleFactor=this->getScaleFactorP();
      if((scaleFactor-V3D(1.0,1.0,1.0)).norm()<1e-12)
        return shape()->solidAngle( factorOutComponentPosition(observer) );
      else
        return shape()->solidAngle( factorOutComponentPosition(observer), scaleFactor );
    }

    /**
    * Given an input estimate of the axis aligned (AA) bounding box (BB), return an improved set of values.
    * The AA BB is determined in the frame of the object and the initial estimate will be transformed there.
    * The returned BB will be the frame of the ParObjComponent and may not be optimal.
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
    void ParObjComponent::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const
    {
      if (!shape()) throw Kernel::Exception::NullPointerException("ParObjComponent::getBoundingBox","shape");
      
      Geometry::V3D min(xmin,ymin,zmin),  max(xmax,ymax,zmax);

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
    }

    /**
    * Get the bounding box for this object-component. The underlying shape has a bounding box defined in its own coorindate
    * system. This needs to be adjusted for the actual position and rotation of this ObjComponent.
    * @param [Out] The bounding box for this object component will be stored here.
    */
    void ParObjComponent::getBoundingBox(BoundingBox& absoluteBB) const
    {
      // Check this object component has a defined shape and bounding box
      boost::shared_ptr<BoundingBox> shapeBox = shape()->getBoundingBox();
      if( !shapeBox ) 
      {
        absoluteBB = BoundingBox();
        return;
      }
      
      // Start with the box in the shape's coordinates and
      // modify in place for speed
      absoluteBB = BoundingBox(*shapeBox);
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
    int ParObjComponent::getPointInObject(V3D& point) const
    {
      // If the form of this component is not defined, throw NullPointerException
      if (!shape()) throw Kernel::Exception::NullPointerException("ParObjComponent::getPointInObject","shape");
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
    const V3D ParObjComponent::factorOutComponentPosition(const V3D& point) const
    {
      // First subtract the component's position, then undo the rotation
      return takeOutRotation( point - this->getPos() );
    }

    /// Rotates a point by the reverse of the component's rotation
    const V3D ParObjComponent::takeOutRotation(V3D point) const
    {
      // Get the total rotation of this component and calculate the inverse (reverse rotation)
      Quat unRotate = this->getRotation();
      unRotate.inverse();
      // Now rotate our point by the angle calculated above
      unRotate.rotate(point);

      // Can not Consider scaling factor here as this transform used by solidAngle as well
      // as IsValid etc. While this would work for latter, breaks former
      //V3D scaleFactor=this->getScaleFactorP();
      //point/=scaleFactor;

      return point;
    }


    /**
    * Draws the ParObjComponent, If the handler is not set then this function does nothing.
    */
    void ParObjComponent::draw() const
    {
      if(Handle() == NULL)return;
      //Render the ParObjComponent and then render the object
      Handle()->Render();
    }

    /**
    * Draws the Object
    */
    void ParObjComponent::drawObject() const
    {
      shape()->draw();
    }

    /**
    * Initializes the ParObjComponent for rendering, this function should be called before rendering.
    */
    void ParObjComponent::initDraw() const
    {
      if(Handle()==NULL)return;
      //Render the ParObjComponent and then render the object
      shape()->initDraw();
      Handle()->Initialize();
    }

  } // namespace Geometry
} // namespace Mantid
