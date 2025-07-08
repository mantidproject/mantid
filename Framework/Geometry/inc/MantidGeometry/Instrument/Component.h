// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/V3D.h"

#include <string>
#include <typeinfo>
#include <vector>

namespace Poco {
namespace XML {
class Attributes;
class XMLWriter;
} // namespace XML
} // namespace Poco

namespace Mantid {
namespace Geometry {

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
template <typename T> class ComponentPool;

/** @class Component Component.h Geometry/Component.h

Component is a wrapper for a Component which can modify some of its
parameters, e.g. position, orientation. Implements IComponent.

@author Roman Tolchenov, Tessella Support Services plc
@date 4/12/2008
*/
class MANTID_GEOMETRY_DLL Component : public virtual IComponent {
public:
  /// @return the name of this type
  virtual std::string typeName() const { return "Component"; }

  /// Constructor for parametrized component
  Component(const IComponent *base, const ParameterMap *map);

  //! Create Empty Component at Origin, with no orientation and null parent
  Component();
  //! Create a named component with a parent component (optional)
  explicit Component(std::string name, IComponent *parent = nullptr);
  //! Create a named component with positioning vector, and parent component
  //(optional)
  Component(std::string name, const Kernel::V3D &position, IComponent *parent = nullptr);
  //! Create a named component with positioning vector, orientation and parent
  // component
  Component(std::string name, const Kernel::V3D &position, const Kernel::Quat &rotation, IComponent *parent = nullptr);

  IComponent *clone() const override;

  //! Returns the ComponentID - a unique identifier of the component.
  ComponentID getComponentID() const override;
  //! Returns const pointer to base component if this component is parametrized
  // or pointer to itself if not. Currently is the same as getComponentID bar
  // const cast;
  IComponent const *getBaseComponent() const override;

  //! Assign a parent IComponent. Previous parent link is lost
  void setParent(IComponent *) override;

  //! Return a pointer to the current parent. as shared pointer
  std::shared_ptr<const IComponent> getParent() const override;
  //! Return an array of all ancestors
  std::vector<std::shared_ptr<const IComponent>> getAncestors() const override;

  bool isParentNamed(const std::string &expectedName, int maxDepth = -1) const;

  //! Set the IComponent name
  void setName(const std::string &) override;

  //! Get the IComponent name
  std::string getName() const override;

  //! Get the full pathname
  std::string getFullName() const override;

  //! Set the IComponent position, x, y, z respective to parent (if present)
  // otherwise absolute
  void setPos(double, double, double) override;
  void setPos(const Kernel::V3D &) override;

  //! Set the IComponent position in the side by side instrument view
  void setSideBySideViewPos(const Kernel::V2D &) override;

  //! Set the orientation Kernel::Quaternion relative to parent (if present)
  // otherwise absolute
  void setRot(const Kernel::Quat &) override;

  //! Translate the IComponent (vector form). This is relative to parent if
  // present.
  void translate(const Kernel::V3D &) override;

  //! Translate the IComponent (x,y,z form). This is relative to parent if
  // present.
  void translate(double, double, double) override;

  //! Rotate the IComponent. This is relative to parent.
  void rotate(const Kernel::Quat &) override;

  //! Rotate the IComponent by an angle in degrees with respect to an axis.
  void rotate(double, const Kernel::V3D &) override;

  //! Get the position relative to the parent IComponent (absolute if no parent)
  Kernel::V3D getRelativePos() const override;

  //! Get the position of the IComponent. Tree structure is traverse through the
  // parent chain
  Kernel::V3D getPos() const override;

  std::optional<Kernel::V2D> getSideBySideViewPos() const override;

  //! Get the relative Orientation
  Kernel::Quat getRelativeRot() const override;

  //! Get the absolute orientation of the IComponent
  Kernel::Quat getRotation() const override;

  //! Get the distance to another IComponent
  double getDistance(const IComponent &) const override;

  /// Get the bounding box for this component and store it in the given argument
  void getBoundingBox(BoundingBox &boundingBox) const override;

  /** @name ParameterMap access */
  //@{
  // 06/05/2010 MG: Templated virtual functions cannot be defined so we have to
  // resort to
  // one for each type, luckily there won't be too many
  /// Return the parameter names
  std::set<std::string> getParameterNames(bool recursive = true) const override;
  /// return the parameter names and the component they are from
  std::map<std::string, ComponentID> getParameterNamesByComponent() const override;
  /// Returns a boolean indicating if the component has the named parameter
  bool hasParameter(const std::string &name, bool recursive = true) const override;

  /**
   * Get a parameter defined as a double
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of values
   */
  std::vector<double> getNumberParameter(const std::string &pname, bool recursive = true) const override {
    return getParameter<double>(pname, recursive);
  }

  /**
   * Get a parameter defined as an int
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of values
   */
  std::vector<int> getIntParameter(const std::string &pname, bool recursive = true) const override {
    return getParameter<int>(pname, recursive);
  }

  /**
   * Get a parameter's type -- this is HACK until Python can export property
   * regardless of the property type
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns std::string describing parameter type or empty string if the type
   * is not found
   */
  std::string getParameterType(const std::string &pname, bool recursive = true) const override {
    Parameter_sptr param;
    if (recursive) {
      param = m_map->getRecursive(this, pname);
    } else {
      param = m_map->get(this, pname);
    }
    if (param)
      return std::string(param->type());
    else
      return std::string("");
  }
  /** Get this component parameter's description -- no recursive search within
   * children*/
  std::string getDescription() const;

  /** Get description of a parameter attached to this component  */
  std::string getParamDescription(const std::string &pname, bool recursive = true) const;

  /** Get a component's parameter short description */
  std::string getParamShortDescription(const std::string &pname, bool recursive = true) const;
  /** Get a components's short description*/
  std::string getShortDescription() const;
  /**Set components description. Works for parameterized components only */
  void setDescription(const std::string &descr);
  /**
   * Get a parameter defined as a bool
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of values
   */
  std::vector<bool> getBoolParameter(const std::string &pname, bool recursive = true) const override {
    return getParameter<bool>(pname, recursive);
  }

  /**
   * Get a parameter defined as a Kernel::V3D
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of values
   */
  std::vector<Kernel::V3D> getPositionParameter(const std::string &pname, bool recursive = true) const override {
    return getParameter<Kernel::V3D>(pname, recursive);
  }

  /**
   * Get a parameter defined as a Kernel::Quaternion
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of values
   */
  std::vector<Kernel::Quat> getRotationParameter(const std::string &pname, bool recursive = true) const override {
    return getParameter<Kernel::Quat>(pname, recursive);
  }

  /**
   * Get a parameter defined as a string
   * @param pname :: The name of the parameter
   * @param recursive :: If true the search will walk up through the parent
   * components
   * @returns A list of values
   */
  std::vector<std::string> getStringParameter(const std::string &pname, bool recursive = true) const override {
    return getParameter<std::string>(pname, recursive);
  }
  //@}

  std::string getParameterAsString(const std::string &pname, bool recursive = true) const override {
    std::string retVal;
    if (m_map) {
      retVal = m_map->getString(this, pname, recursive);
    }
    return retVal;
  }

  /**
   *  Get a visibility attribute of a parameter from the parameter map
   * @param p_name :: The name of the parameter
   * @param recursive :: If true then the lookup will walk up the tree if this
   * component does not have parameter
   * @return A boolean containing the visibility attribute of the parameter, false if does not exist
   */
  bool getParameterVisible(const std::string &p_name, bool recursive) const override {
    if (m_map) {
      Parameter_sptr param;
      if (recursive) {
        param = m_map->getRecursive(this, p_name);
      } else {
        param = m_map->get(this, p_name);
      }
      if (param != Parameter_sptr()) {
        return param->visible();
      } else {
        return false;
      }
    } else {
      // Not parametrized = not visible by default
      return false;
    }
  }

  /// Get fitting parameter
  double getFittingParameter(const std::string &pname, double xvalue) const;

  void printSelf(std::ostream &) const override;

  /// Returns the address of the base component
  const IComponent *base() const { return m_base; }

  /// Returns the ScaleFactor
  Kernel::V3D getScaleFactor() const override;

  /** Returns the bare pointer to the IComponent parent */
  const IComponent *getBareParent() const override { return m_parent; }

  virtual void readXMLAttributes(const Poco::XML::Attributes &attr);
  virtual void writeXML(Poco::XML::XMLWriter &writer) const;
  virtual void appendXML(std::ostream &xmlStream) const;

  bool isParametrized() const override;

  virtual size_t registerContents(class ComponentVisitor &componentVisitor) const override;
  bool hasComponentInfo() const;
  size_t index() const;

protected:
  /// Parent component in the tree
  const IComponent *m_parent;
  /// The base component - this is the unmodified component (without the
  /// parameters). Stored
  /// as a pointer to Component so that it's properties can be accessed without
  /// casting each time
  const Component *m_base;
  /// A  pointer to const ParameterMap containing the parameters
  const ParameterMap *m_map;

  //! Name of the component
  std::string m_name;
  //! Position w
  Kernel::V3D m_pos;
  //! Orientation
  Kernel::Quat m_rot;
  //! Position of component in instrument viewer side by side view
  std::optional<Kernel::V2D> m_sidebysideviewpos;

  /**
   *  Get a parameter from the parameter map
   * @param p_name :: The name of the parameter
   * @param recursive :: If true then the lookup will walk up the tree if this
   * component does not have parameter
   * @return A list of size 0 or 1 containing the parameter value or
   * nothing if it does not exist
   */
  template <class TYPE> std::vector<TYPE> getParameter(const std::string &p_name, bool recursive) const {
    if (m_map) {
      Parameter_sptr param;
      if (recursive) {
        param = m_map->getRecursive(this, p_name);
      } else {
        param = m_map->get(this, p_name);
      }
      if (param != Parameter_sptr()) {
        return std::vector<TYPE>(1, param->value<TYPE>());
      } else {
        return std::vector<TYPE>(0);
      }
    } else {
      // Not parametrized = return empty vector
      return std::vector<TYPE>(0);
    }
  }

protected:
  // This method is only required for efficient caching of parameterized
  // components and
  // should not form part of the interface. It is an implementation detail.
  template <typename T> friend class ComponentPool;
  /// Swap the current references to the un-parameterized component and
  /// parameter map for new ones
  void swap(const Component *base, const ParameterMap *pmap);
};

} // namespace Geometry
} // namespace Mantid
