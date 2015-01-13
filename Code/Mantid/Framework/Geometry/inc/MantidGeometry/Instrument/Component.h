#ifndef MANTID_GEOMETRY_Component_H_
#define MANTID_GEOMETRY_Component_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include <string>
#include <sstream>
#include <typeinfo>
#include <vector>
#include <Poco/SAX/Attributes.h>
#ifdef _MSC_VER
// Disable a flood of warnings from Poco about inheriting from
// std::basic_istream
// See
// http://connect.microsoft.com/VisualStudio/feedback/details/733720/inheriting-from-std-fstream-produces-c4250-warning
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

#include <Poco/XML/XMLWriter.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Mantid {
namespace Kernel {
class V3D;
class Quat;
}

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

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL Component : public virtual IComponent {
public:
  /// @return the name of this type
  virtual std::string typeName() const { return "Component"; }

  /// Constructor for parametrized component
  Component(const IComponent *base, const ParameterMap *map);

  //! Create Empty Component at Origin, with no orientation and null parent
  Component();
  //! Create a named component with a parent component (optional)
  explicit Component(const std::string &name, IComponent *parent = 0);
  //! Create a named component with positioning vector, and parent component
  //(optional)
  Component(const std::string &name, const Kernel::V3D &position,
            IComponent *parent = 0);
  //! Create a named component with positioning vector, orientation and parent
  // component
  Component(const std::string &name, const Kernel::V3D &position,
            const Kernel::Quat &rotation, IComponent *parent = 0);

  ///  destructor
  ~Component();

  IComponent *clone() const;

  //! Returns the ComponentID - a unique identifier of the component.
  ComponentID getComponentID() const;
  //! Returns const pointer to base component if this component is parametrized
  // or pointer to itself if not. Currently is the same as getComponentID bar
  // const cast;
  IComponent const *getBaseComponent() const;

  //! Assign a parent IComponent. Previous parent link is lost
  void setParent(IComponent *);

  //! Return a pointer to the current parent. as shared pointer
  boost::shared_ptr<const IComponent> getParent() const;
  //! Return an array of all ancestors
  std::vector<boost::shared_ptr<const IComponent>> getAncestors() const;

  bool isParentNamed(const std::string &expectedName, int maxDepth = -1) const;

  //! Set the IComponent name
  void setName(const std::string &);

  //! Get the IComponent name
  std::string getName() const;

  //! Get the full pathname
  std::string getFullName() const;

  //! Set the IComponent position, x, y, z respective to parent (if present)
  // otherwise absolute
  void setPos(double, double, double);
  void setPos(const Kernel::V3D &);

  //! Set the orientation Kernel::Quaternion relative to parent (if present)
  // otherwise absolute
  void setRot(const Kernel::Quat &);

  //! Translate the IComponent (vector form). This is relative to parent if
  // present.
  void translate(const Kernel::V3D &);

  //! Translate the IComponent (x,y,z form). This is relative to parent if
  // present.
  void translate(double, double, double);

  //! Rotate the IComponent. This is relative to parent.
  void rotate(const Kernel::Quat &);

  //! Rotate the IComponent by an angle in degrees with respect to an axis.
  void rotate(double, const Kernel::V3D &);

  //! Get the position relative to the parent IComponent (absolute if no parent)
  virtual const Kernel::V3D getRelativePos() const;

  //! Get the position of the IComponent. Tree structure is traverse through the
  // parent chain
  virtual Kernel::V3D getPos() const;

  //! Get the relative Orientation
  const Kernel::Quat &getRelativeRot() const;

  //! Get the absolute orientation of the IComponent
  virtual const Kernel::Quat getRotation() const;

  //! Get the distance to another IComponent
  double getDistance(const IComponent &) const;

  /// Get the bounding box for this component and store it in the given argument
  virtual void getBoundingBox(BoundingBox &boundingBox) const;

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
  /// Returns a boolean indicating if the component has the named parameter
  virtual bool hasParameter(const std::string &name,
                            bool recursive = true) const;

  /**
  * Get a parameter defined as a double
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of values
  */
  std::vector<double> getNumberParameter(const std::string &pname,
                                         bool recursive = true) const {
    return getParameter<double>(pname, recursive);
  }

  /**
  * Get a parameter defined as an int
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of values
  */
  std::vector<int> getIntParameter(const std::string &pname,
                                   bool recursive = true) const {
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
  std::string getParameterType(const std::string &pname,
                               bool recursive = true) const {
    Parameter_sptr param = Parameter_sptr(); // Null shared pointer
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

  /**
  * Get a parameter defined as a bool
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of values
  */
  std::vector<bool> getBoolParameter(const std::string &pname,
                                     bool recursive = true) const {
    return getParameter<bool>(pname, recursive);
  }

  /**
  * Get a parameter defined as a Kernel::V3D
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of values
  */
  std::vector<Kernel::V3D> getPositionParameter(const std::string &pname,
                                                bool recursive = true) const {
    return getParameter<Kernel::V3D>(pname, recursive);
  }

  /**
  * Get a parameter defined as a Kernel::Quaternion
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of values
  */
  std::vector<Kernel::Quat> getRotationParameter(const std::string &pname,
                                                 bool recursive = true) const {
    return getParameter<Kernel::Quat>(pname, recursive);
  }

  /**
  * Get a parameter defined as a string
  * @param pname :: The name of the parameter
  * @param recursive :: If true the search will walk up through the parent
  * components
  * @returns A list of values
  */
  std::vector<std::string> getStringParameter(const std::string &pname,
                                              bool recursive = true) const {
    return getParameter<std::string>(pname, recursive);
  }
  //@}

  std::string getParameterAsString(const std::string &pname,
                                   bool recursive = true) const {
    std::string retVal = "";
    if (m_map) {
      retVal = m_map->getString(this, pname, recursive);
    }
    return retVal;
  }

  void printSelf(std::ostream &) const;

  /// Returns the address of the base component
  const IComponent *base() const { return m_base; }

  /// Returns the ScaleFactor
  virtual Kernel::V3D getScaleFactor() const;

  /** Returns the bare pointer to the IComponent parent */
  const IComponent *getBareParent() const { return m_parent; }

  virtual void readXMLAttributes(const Poco::XML::Attributes &attr);
  virtual void writeXML(Poco::XML::XMLWriter &writer) const;
  virtual void appendXML(std::ostream &xmlStream) const;

  bool isParametrized() const;

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

  /**
  *  Get a parameter from the parameter map
  * @param p_name :: The name of the parameter
  * @param recursive :: If true then the lookup will walk up the tree if this
  * component does not have parameter
  * @return A list of size 0 or 1 containing the parameter value or
  * nothing if it does not exist
  */
  template <class TYPE>
  std::vector<TYPE> getParameter(const std::string &p_name,
                                 bool recursive) const {
    if (m_map) {
      Parameter_sptr param = Parameter_sptr(); // Null shared pointer
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

#endif /*MANTID_GEOMETRY_Component_H_*/
