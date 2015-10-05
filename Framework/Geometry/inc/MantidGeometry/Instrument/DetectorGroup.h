#ifndef MANTID_GEOMETRY_DETECTORGROUP_H_
#define MANTID_GEOMETRY_DETECTORGROUP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include <vector>
#include <map>

namespace Mantid {
namespace Geometry {
/** Holds a collection of detectors.
Responds to IDetector methods as though it were a single detector.
Currently, detectors in a group are treated as pointlike (or at least)
homogenous entities. This means that it's up to the use to make
only sensible groupings of similar detectors since no weighting according
to solid angle size takes place and the DetectorGroup's position is just
a simple average of its constituents.

@author Russell Taylor, Tessella Support Services plc
@date 08/04/2008

Copyright &copy; 2008-2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_GEOMETRY_DLL DetectorGroup : public virtual IDetector {
public:
  DetectorGroup();
  DetectorGroup(const std::vector<IDetector_const_sptr> &dets,
                bool warnAboutMasked = false);
  virtual ~DetectorGroup();

  void addDetector(IDetector_const_sptr det, bool &warn);

  // IDetector methods
  detid_t getID() const;
  std::size_t nDets() const;
  Kernel::V3D getPos() const;
  double getDistance(const IComponent &comp) const;
  double getTwoTheta(const Kernel::V3D &observer,
                     const Kernel::V3D &axis) const;
  double getSignedTwoTheta(const Kernel::V3D &observer, const Kernel::V3D &axis,
                           const Kernel::V3D &instrumentUp) const;
  double getPhi() const;
  double getPhiOffset(const double &offset) const;
  double solidAngle(const Kernel::V3D &observer) const;
  bool isParametrized() const;
  bool isMasked() const;
  bool isMonitor() const;
  bool isValid(const Kernel::V3D &point) const;
  virtual bool isOnSide(const Kernel::V3D &point) const;
  /// Try to find a point that lies within (or on) the object
  int getPointInObject(Kernel::V3D &point) const;
  /// Get the bounding box for this component and store it in the given argument
  virtual void getBoundingBox(BoundingBox &boundingBox) const;

  /// What detectors are contained in the group?
  std::vector<detid_t> getDetectorIDs() const;
  /// What detectors are contained in the group?
  std::vector<IDetector_const_sptr> getDetectors() const;

  /** @name ParameterMap access */
  //@{
  // 06/05/2010 MG: Templated virtual functions cannot be defined so we have to
  // resort to
  // one for each type, luckily there won't be too many
  /// Return the parameter names
  virtual std::set<std::string> getParameterNames(bool recursive = true) const;
  /// return the parameter names and the component they are from
  virtual std::map<std::string, ComponentID>
  getParameterNamesByComponent() const;
  /// Returns a boolean indicating whether the parameter exists or not
  bool hasParameter(const std::string &name, bool recursive = true) const;
  // Hack used untill Geomertry can not exprot different types parematers
  // properly
  std::string getParameterType(const std::string &name,
                               bool recursive = true) const;
  /**
  * Get a parameter defined as a double
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of size 0 as this is not a parameterized component
  */
  std::vector<double> getNumberParameter(const std::string &pname,
                                         bool recursive = true) const;
  /**
  * Get a parameter defined as a Kernel::V3D
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of size 0 as this is not a parameterized component
  */
  std::vector<Kernel::V3D> getPositionParameter(const std::string &pname,
                                                bool recursive = true) const;
  /**
  * Get a parameter defined as a Kernel::Quaternion
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of size 0 as this is not a parameterized component
  */
  std::vector<Kernel::Quat> getRotationParameter(const std::string &pname,
                                                 bool recursive = true) const;

  /**
  * Get a parameter defined as a string
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of size 0 as this is not a parameterized component
  */
  std::vector<std::string> getStringParameter(const std::string &pname,
                                              bool recursive = true) const;

  /**
  * Get a parameter defined as an integer
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of size 0 as this is not a parameterized component
  */
  std::vector<int> getIntParameter(const std::string &pname,
                                   bool recursive = true) const;

  /**
  * Get a parameter defined as an integer
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of size 0 as this is not a parameterized component
  */
  std::vector<bool> getBoolParameter(const std::string &pname,
                                     bool recursive = true) const;

  /**
   * Get a string representation of a parameter
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A empty string as this is not a parameterized component
   */
  std::string getParameterAsString(const std::string &pname,
                                   bool recursive = true) const;

  /** returns the detector's group topology if it has been calculated before or
  invokes the procedure of
  calculating such topology if it was not */
  det_topology getTopology(Kernel::V3D &center) const;

  /// Return separator for list of names of detectors
  std::string getNameSeparator() const { return ";"; }
  /** Returns const pointer to itself. This currently (2914/04/24) contradicts
     the logic behind getComponentID overload, so CopyInstrumentParameters will
     fail on
      grouped instrument but it is something TO DO:      */
  virtual IComponent const *getBaseComponent() const {
    return const_cast<const DetectorGroup *>(this);
  }

protected:
  /// The ID of this effective detector
  int m_id;
  /// The type of collection used for the detectors
  ///          - a map of detector pointers with the detector ID as the key
  // May want to change this to a hash_map in due course
  typedef std::map<int, IDetector_const_sptr> DetCollection;
  /// The collection of grouped detectors
  DetCollection m_detectors;
  /** the parameter describes the topology of the detector's group namely if
  * detectors form a box or a ring.
  *  the topology is undefined on construction and calculated on first request
  */
  mutable det_topology group_topology;
  /// group centre is the geometrical centre of the detectors group calculated
  /// when the calculate group topology is invoked
  mutable Kernel::V3D groupCentre;

  // functions inherited from IComponent
  Component *clone() const { return NULL; }
  ComponentID getComponentID(void) const { return NULL; }
  boost::shared_ptr<const IComponent> getParent() const {
    return boost::shared_ptr<const IComponent>();
  }
  virtual const IComponent *getBareParent() const { return NULL; }
  std::vector<boost::shared_ptr<const IComponent>> getAncestors() const {
    return std::vector<boost::shared_ptr<const IComponent>>();
  }
  std::string getName() const;
  std::string getFullName() const;
  void setParent(IComponent *) {}
  void setName(const std::string &) {}

  void setPos(double, double, double) {}
  void setPos(const Kernel::V3D &) {}
  void setRot(const Kernel::Quat &) {}
  void copyRot(const IComponent &) {}
  int interceptSurface(Track &) const { return -10; }
  void translate(const Kernel::V3D &) {}
  void translate(double, double, double) {}
  void rotate(const Kernel::Quat &) {}
  void rotate(double, const Kernel::V3D &) {}
  const Kernel::V3D getRelativePos() const {
    throw std::runtime_error("Cannot call getRelativePos on a DetectorGroup");
  }
  const Kernel::Quat &getRelativeRot() const {
    throw std::runtime_error("Cannot call getRelativeRot on a DetectorGroup");
  }
  const Kernel::Quat getRotation() const { return Kernel::Quat(); }
  void printSelf(std::ostream &) const {}

  // functions inherited from IObjComponent

  void getBoundingBox(double &, double &, double &, double &, double &,
                      double &) const {};

  void draw() const {};
  void drawObject() const {};
  void initDraw() const {};

  /// Returns the shape of the Object
  const boost::shared_ptr<const Object> shape() const {
    return boost::shared_ptr<const Object>();
  }
  /// Returns the material of the Object
  const boost::shared_ptr<const Kernel::Material> material() const {
    return boost::shared_ptr<const Kernel::Material>();
  }

private:
  /// Private, unimplemented copy constructor
  DetectorGroup(const DetectorGroup &);
  /// Private, unimplemented copy assignment operator
  DetectorGroup &operator=(const DetectorGroup &);

  /// function calculates the detectors arrangement (topology)
  void calculateGroupTopology() const;
};

/// Typedef for shared pointer
typedef boost::shared_ptr<DetectorGroup> DetectorGroup_sptr;
/// Typedef for shared pointer to a const object
typedef boost::shared_ptr<const DetectorGroup> DetectorGroup_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif /*MANTID_GEOMETRY_DETECTORGROUP_H_*/
