// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include <map>
#include <vector>

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
*/
class MANTID_GEOMETRY_DLL DetectorGroup : public virtual IDetector {
public:
  DetectorGroup();
  DetectorGroup(const std::vector<IDetector_const_sptr> &dets);

  void addDetector(const IDetector_const_sptr &det);

  // IDetector methods
  IDetector *cloneParameterized(const ParameterMap *) const override { return nullptr; }
  detid_t getID() const override;
  std::size_t nDets() const override;
  Kernel::V3D getPos() const override;
  std::optional<Kernel::V2D> getSideBySideViewPos() const override;
  double getDistance(const IComponent &comp) const override;
  double getTwoTheta(const Kernel::V3D &observer, const Kernel::V3D &axis) const override;
  double getSignedTwoTheta(const Kernel::V3D &observer, const Kernel::V3D &axis,
                           const Kernel::V3D &instrumentUp) const override;
  double getPhi() const override;
  double getPhiOffset(const double &offset) const override;
  double solidAngle(const Geometry::SolidAngleParams &params) const override;
  bool isParametrized() const override;
  bool isValid(const Kernel::V3D &point) const override;
  bool isOnSide(const Kernel::V3D &point) const override;
  /// Try to find a point that lies within (or on) the object
  int getPointInObject(Kernel::V3D &point) const override;
  /// Get the bounding box for this component and store it in the given argument
  void getBoundingBox(BoundingBox &boundingBox) const override;

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
  std::set<std::string> getParameterNames(bool recursive = true) const override;
  /// return the parameter names and the component they are from
  std::map<std::string, ComponentID> getParameterNamesByComponent() const override;
  /// Returns a boolean indicating whether the parameter exists or not
  bool hasParameter(const std::string &name, bool recursive = true) const override;
  // Hack used untill Geomertry can not exprot different types parematers
  // properly
  std::string getParameterType(const std::string &name, bool recursive = true) const override;
  /**
   * Get a parameter defined as a double
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of size 0 as this is not a parameterized component
   */
  std::vector<double> getNumberParameter(const std::string &pname, bool recursive = true) const override;
  /**
   * Get a parameter defined as a Kernel::V3D
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of size 0 as this is not a parameterized component
   */
  std::vector<Kernel::V3D> getPositionParameter(const std::string &pname, bool recursive = true) const override;
  /**
   * Get a parameter defined as a Kernel::Quaternion
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of size 0 as this is not a parameterized component
   */
  std::vector<Kernel::Quat> getRotationParameter(const std::string &pname, bool recursive = true) const override;

  /**
   * Get a parameter defined as a string
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of size 0 as this is not a parameterized component
   */
  std::vector<std::string> getStringParameter(const std::string &pname, bool recursive = true) const override;

  /**
   * Get a parameter defined as an integer
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of size 0 as this is not a parameterized component
   */
  std::vector<int> getIntParameter(const std::string &pname, bool recursive = true) const override;

  /**
   * Get a parameter defined as an integer
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of size 0 as this is not a parameterized component
   */
  std::vector<bool> getBoolParameter(const std::string &pname, bool recursive = true) const override;

  /**
   * Get a string representation of a parameter
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A empty string as this is not a parameterized component
   */
  std::string getParameterAsString(const std::string &pname, bool recursive = true) const override;

  /**
   * Get a visibility attribute of a parameter
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent components
   * @return A boolean containing the visibility attribute of the parameter, false if does not exist
   */
  bool getParameterVisible(const std::string &pname, bool recursive = true) const override;

  /** returns the detector's group topology if it has been calculated before or
  invokes the procedure of
  calculating such topology if it was not */
  det_topology getTopology(Kernel::V3D &center) const override;

  /// Return separator for list of names of detectors
  std::string getNameSeparator() const { return ";"; }
  /** Returns const pointer to itself. This currently (2914/04/24) contradicts
     the logic behind getComponentID overload, so CopyInstrumentParameters will
     fail on
      grouped instrument but it is something TO DO:      */
  IComponent const *getBaseComponent() const override { return const_cast<const DetectorGroup *>(this); }

  const ParameterMap &parameterMap() const override;
  size_t index() const override;
  virtual size_t registerContents(class ComponentVisitor &visitor) const override;

protected:
  /// The ID of this effective detector
  int m_id;
  /// The type of collection used for the detectors
  ///          - a map of detector pointers with the detector ID as the key
  // May want to change this to a hash_map in due course
  using DetCollection = std::map<int, IDetector_const_sptr>;
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
  Component *clone() const override { return nullptr; }
  ComponentID getComponentID() const override { return nullptr; }
  std::shared_ptr<const IComponent> getParent() const override { return std::shared_ptr<const IComponent>(); }
  const IComponent *getBareParent() const override { return nullptr; }
  std::vector<std::shared_ptr<const IComponent>> getAncestors() const override {
    return std::vector<std::shared_ptr<const IComponent>>();
  }
  std::string getName() const override;
  std::string getFullName() const override;
  void setParent(IComponent *) override {}
  void setName(const std::string &) override {}

  void setPos(double, double, double) override {}
  void setPos(const Kernel::V3D &) override {}
  void setSideBySideViewPos(const Kernel::V2D &) override {}
  void setRot(const Kernel::Quat &) override {}
  void copyRot(const IComponent &) {}
  int interceptSurface(Track &) const override { return -10; }
  void translate(const Kernel::V3D &) override {}
  void translate(double, double, double) override {}
  void rotate(const Kernel::Quat &) override {}
  void rotate(double, const Kernel::V3D &) override {}
  Kernel::V3D getRelativePos() const override {
    throw std::runtime_error("Cannot call getRelativePos on a DetectorGroup");
  }
  Kernel::Quat getRelativeRot() const override {
    throw std::runtime_error("Cannot call getRelativeRot on a DetectorGroup");
  }
  Kernel::Quat getRotation() const override { return Kernel::Quat(); }
  void printSelf(std::ostream &) const override {}

  // functions inherited from IObjComponent

  void getBoundingBox(double &, double &, double &, double &, double &, double &) const {}

  void draw() const override {}
  void drawObject() const override {}
  void initDraw() const override {}

  /// Returns the shape of the Object
  const std::shared_ptr<const IObject> shape() const override { return std::shared_ptr<const IObject>(); }
  /// Returns the material of the Object
  const Kernel::Material material() const override;

private:
  /// Private, unimplemented copy constructor
  DetectorGroup(const DetectorGroup &);
  /// Private, unimplemented copy assignment operator
  DetectorGroup &operator=(const DetectorGroup &);

  /// function calculates the detectors arrangement (topology)
  void calculateGroupTopology() const;
};

/// Typedef for shared pointer
using DetectorGroup_sptr = std::shared_ptr<DetectorGroup>;
/// Typedef for shared pointer to a const object
using DetectorGroup_const_sptr = std::shared_ptr<const DetectorGroup>;

} // namespace Geometry
} // namespace Mantid
