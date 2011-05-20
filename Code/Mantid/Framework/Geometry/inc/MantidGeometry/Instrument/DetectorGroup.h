#ifndef MANTID_GEOMETRY_DETECTORGROUP_H_
#define MANTID_GEOMETRY_DETECTORGROUP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>

namespace Mantid
{
  namespace Geometry
  {
    /** Holds a collection of detectors.
	Responds to IDetector methods as though it were a single detector.
	Currently, detectors in a group are treated as pointlike (or at least)
	homogenous entities. This means that it's up to the use to make
	only sensible groupings of similar detectors since no weighting according
	to solid angle size takes place and the DetectorGroup's position is just
	a simple average of its constituents.
	
	@author Russell Taylor, Tessella Support Services plc
	@date 08/04/2008
	
	Copyright &copy; 2008-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
	
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
    class DLLExport DetectorGroup : public virtual IDetector
    {
    public:
      DetectorGroup(const std::vector<IDetector_sptr>& dets, bool warnAboutMasked = false);
      virtual ~DetectorGroup();

      void addDetector(IDetector_sptr det, bool& warn);

      // IDetector methods
      detid_t getID() const;
      std::size_t nDets() const;
      V3D getPos() const;
      double getDistance(const IComponent& comp) const;
      double getTwoTheta(const V3D& observer, const V3D& axis) const;
      double getPhi() const;
      double solidAngle(const V3D& observer) const; 
      bool isParametrized() const;
      bool isMasked() const;
      bool isMonitor() const;
      std::map<detid_t, double> getNeighbours(double radius);
      bool isValid(const V3D& point) const;
      virtual bool isOnSide(const V3D& point) const;
      ///Try to find a point that lies within (or on) the object
      int getPointInObject(V3D& point) const;
      /// Get the bounding box for this component and store it in the given argument
      virtual void getBoundingBox(BoundingBox& boundingBox) const;
      /// What detectors are contained in the group?
      std::vector<detid_t> getDetectorIDs();
      /// What detectors are contained in the group?
      std::vector<IDetector_sptr> getDetectors() const;

      /** @name ParameterMap access */
      //@{
      // 06/05/2010 MG: Templated virtual functions cannot be defined so we have to resort to
      // one for each type, luckily there won't be too many
      /// Return the parameter names
      virtual std::set<std::string> getParameterNames(bool recursive = true) const;
      /// Returns a boolean indicating whether the parameter exists or not
      bool hasParameter(const std::string & name, bool recursive = true) const;
      /**
       * Get a parameter defined as a double
       * @param pname :: The name of the parameter
       * @param recursive :: If true the search will walk up through the parent components
       * @returns A list of size 0 as this is not a parameterized component
       */
      std::vector<double> getNumberParameter(const std::string& pname, bool recursive = true) const;
      /**
       * Get a parameter defined as a V3D
       * @param pname :: The name of the parameter
       * @param recursive :: If true the search will walk up through the parent components
       * @returns A list of size 0 as this is not a parameterized component
       */
      std::vector<V3D> getPositionParameter(const std::string& pname, bool recursive = true) const;
      /**
       * Get a parameter defined as a Quaternion
       * @param pname :: The name of the parameter
       * @param recursive :: If true the search will walk up through the parent components
       * @returns A list of size 0 as this is not a parameterized component
       */
      std::vector<Quat> getRotationParameter(const std::string& pname, bool recursive = true) const;

      /**
       * Get a parameter defined as a string
       * @param pname :: The name of the parameter
       * @param recursive :: If true the search will walk up through the parent components
       * @returns A list of size 0 as this is not a parameterized component
       */
      std::vector<std::string> getStringParameter(const std::string& pname, bool recursive = true) const;

	protected:
      /// The ID of this effective detector
      int m_id;
      /// The type of collection used for the detectors
      ///          - a map of detector pointers with the detector ID as the key
      // May want to change this to a hash_map in due course
      typedef std::map<int, IDetector_sptr> DetCollection;
      /// The collection of grouped detectors
      DetCollection m_detectors;

   
      // functions inherited from IComponent
      Component* clone() const{ return NULL; }
      ComponentID getComponentID(void) const{ return NULL; }
      boost::shared_ptr<const IComponent> getParent() const
      {
        return boost::shared_ptr<const IComponent>();
      }
      virtual const IComponent* getBareParent() const { return NULL; }
      std::vector<boost::shared_ptr<const IComponent> > getAncestors() const
      {
        return std::vector<boost::shared_ptr<const IComponent> >();
      }
      std::string getName() const{return "";}
      void setParent(IComponent*){}
      void setName(const std::string&){}

      void setPos(double, double, double){}
      void setPos(const V3D&){}
      void setRot(const Quat&){}
      void copyRot(const IComponent&){}
      int interceptSurface(Track&) const{ return -10; }
      void translate(const V3D&){}
      void translate(double, double, double){}
      void rotate(const Quat&){}
      void rotate(double,const V3D&){}
      const V3D& getRelativePos() const { throw std::runtime_error("Cannot call getRelativePos on a DetectorGroup");  }
      const Quat& getRelativeRot() const{ throw std::runtime_error("Cannot call getRelativeRot on a DetectorGroup"); }
      const Quat getRotation() const{ return Quat(); }
      void printSelf(std::ostream&) const{}

      // functions inherited from IObjComponent

      void getBoundingBox(double &, double &, double &, double &, double &, double &) const{};

      void draw() const{};
      void drawObject() const{};
      void initDraw() const{};

      /// Returns the shape of the Object
      const boost::shared_ptr<const Object> shape() const
      {
        return boost::shared_ptr<const Object>();
      }
      /// Returns the material of the Object
      const boost::shared_ptr<const Material> material() const
      {
        return boost::shared_ptr<const Material>();
      }
    private:
      /// Private, unimplemented copy constructor
      DetectorGroup(const DetectorGroup&);
      /// Private, unimplemented copy assignment operator
      DetectorGroup& operator=(const DetectorGroup&);

   /// Static reference to the logger class
      static Kernel::Logger& g_log;
  
    };

    /// Typedef for shared pointer
    typedef boost::shared_ptr<DetectorGroup> DetectorGroup_sptr;
    /// Typedef for shared pointer to a const object
    typedef boost::shared_ptr<const DetectorGroup> DetectorGroup_const_sptr;

  } // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_DETECTORGROUP_H_*/
