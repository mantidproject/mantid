// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentVisitor.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Exception.h"

#include <Poco/SAX/AttributesImpl.h>
#include <Poco/XML/XMLWriter.h>

#include <utility>

namespace Mantid::Geometry {

using Kernel::Quat;
using Kernel::V2D;
using Kernel::V3D;

/** Constructor for a parametrized Component.
 * @param base :: a Component that is the base (un-parametrized) component
 * @param map :: a ParameterMap to parameterize the component
 */
Component::Component(const IComponent *base, const ParameterMap *map)
    : m_parent(nullptr), m_base(dynamic_cast<const Component *>(base)), m_map(map) {
  if (!m_base) {
    throw std::invalid_argument("Component::Component() - Cannot construct a "
                                "parameterized component from an invalid base component.");
  }
}

/** Empty constructor
 *  Create a component with null parent
 */
Component::Component() : m_parent(nullptr), m_base(nullptr), m_map(nullptr), m_name(), m_pos(), m_rot() {}

/** Constructor by value
 *  @param name :: Component name
 *  @param parent :: parent Component (optional)
 */
Component::Component(std::string name, IComponent *parent)
    : m_parent(parent), m_base(nullptr), m_map(nullptr), m_name(std::move(name)), m_pos(), m_rot() {}

/** Constructor by value
 *  @param name :: Component name
 *  @param position :: position
 *  absolute or relative if the parent is defined
 *  @param parent :: parent Component
 */
Component::Component(std::string name, const V3D &position, IComponent *parent)
    : m_parent(parent), m_base(nullptr), m_map(nullptr), m_name(std::move(name)), m_pos(position), m_rot() {}

/** Constructor
 *  @param name :: Component name
 *  @param position :: position (relative to parent, if present)
 *  @param rotation :: orientation quaternion (relative to parent, if present)
 *  @param parent :: parent Component (optional)
 */
Component::Component(std::string name, const V3D &position, const Quat &rotation, IComponent *parent)
    : m_parent(parent), m_base(nullptr), m_map(nullptr), m_name(std::move(name)), m_pos(position), m_rot(rotation) {}

//------------------------------------------------------------------------------------------------
/** Return true if the Component is, in fact, parametrized
 *  (that is - it has a valid parameter map)
 */
bool Component::isParametrized() const { return (m_map != nullptr); }

/** Clone method
 *  Make a copy of the Component
 *  @return new(*this)
 */
IComponent *Component::clone() const {
  // TODO : overload to copy the new pmap
  // Create a new one with pmap parameter
  /// if pmap not present throw
  return new Component(*this);
}

/**  Get the component's ID (pointer address)
 *   @return ID
 */
ComponentID Component::getComponentID() const {
  if (m_map)
    return ComponentID(const_cast<Component *>(m_base));
  else
    return ComponentID(const_cast<Component *>(this));
}

const IComponent *Component::getBaseComponent() const {
  if (m_map)
    return const_cast<const Component *>(m_base);
  else
    return const_cast<const Component *>(this);
}

//-------------------------------------------------------------------------------
/** Set the parent. Previous parenting is lost.
 *  @param comp :: the parent Component
 */
void Component::setParent(IComponent *comp) { m_parent = comp; }

//--------------------------------------------------------------------------------------------
/** Get a pointer to the parent.
 *  @return this.parent
 */
std::shared_ptr<const IComponent> Component::getParent() const {
  if (this->m_map) {
    std::shared_ptr<const IComponent> parent = m_base->getParent();
    return ParComponentFactory::create(parent, m_map);
  } else
    return std::shared_ptr<const IComponent>(m_parent, NoDeleting());
}

//--------------------------------------------------------------------------------------------
/** Return true if one of the parents of this component is named something
 *
 * @param expectedName :: case-sensitive name to look for.
 * @param maxDepth :: levels of recursion to look into, -1 for no limit
 *(default)
 * @return true if a parent matches, false otherwise
 */
bool Component::isParentNamed(const std::string &expectedName, int maxDepth) const {
  int depth = 0;
  const IComponent *parent = m_parent;
  while (parent && (depth < maxDepth || maxDepth < 0)) {
    // Correct name? stop searching
    if (parent->getName() == expectedName)
      return true;
    parent = parent->getBareParent();
    depth++;
  }
  return false;
}

//--------------------------------------------------------------------------------------------
/** Returns an array of all ancestors of this component,
 *  starting with the direct parent and moving up
 *  @return An array of pointers to ancestor components
 */
std::vector<std::shared_ptr<const IComponent>> Component::getAncestors() const {
  std::vector<std::shared_ptr<const IComponent>> ancs;

  std::shared_ptr<const IComponent> current = this->getParent();
  while (current) {
    ancs.emplace_back(current);
    current = current->getParent();
  }
  return ancs;
}

//--------------------------------------------------------------------------------------------
/** Set the name of the Component (currently does nothing)
 *  @param s :: name string
 */
void Component::setName(const std::string &s) {
  if (!m_map)
    this->m_name = s;
  else
    throw Kernel::Exception::NotImplementedError("Component::setName (for Parametrized Component)");
}

//--------------------------------------------------------------------------------------------
/** Get the name of the Component
 *  @return this.name
 */
std::string Component::getName() const {
  if (m_map)
    return m_base->getName();
  else
    return m_name;
}

/** Get the full path name of the Component
 *  @return full path name
 */
std::string Component::getFullName() const {
  std::vector<std::shared_ptr<const IComponent>> ancestors = this->getAncestors();
  if (ancestors.empty()) {
    return this->getName();
  } else {
    std::ostringstream oss;
    std::vector<std::shared_ptr<const IComponent>>::reverse_iterator rit;
    for (rit = ancestors.rbegin(); rit < ancestors.rend(); ++rit) {
      oss << (*rit)->getName() << "/";
    }
    oss << this->getName();
    return oss.str();
  }
}

/** Set the position of the Component
 *  The position is with respect to the parent Component
 *  @param x :: x position
 *  @param y :: y position
 *   @param z :: z position
 */
void Component::setPos(double x, double y, double z) {
  if (!m_map)
    m_pos = V3D(x, y, z);
  else
    throw Kernel::Exception::NotImplementedError("Component::setPos (for Parametrized Component)");
}

/** Set the position of the Component
 *  The position is with respect to the parent Component
 *  @param v :: vector position
 */
void Component::setPos(const V3D &v) {
  if (!m_map)
    m_pos = v;
  else
    throw Kernel::Exception::NotImplementedError("Component::setPos (for Parametrized Component)");
}

void Component::setSideBySideViewPos(const Kernel::V2D &pos) { m_sidebysideviewpos = pos; }

/** Set the orientation of the Component
 *  The position is with respect to the parent Component
 *  @param q :: rotation quaternion
 */
void Component::setRot(const Quat &q) {
  if (!m_map)
    m_rot = q;
  else
    throw Kernel::Exception::NotImplementedError("Component::setRot (for Parametrized Component)");
}

/** Translate the Component relative to the parent Component
 *  @param x :: translation on the x-axis
 *   @param y :: translation on the y-axis
 *  @param z :: translation on the z-axis
 */
void Component::translate(double x, double y, double z) {
  if (!m_map) {
    m_pos[0] += x;
    m_pos[1] += y;
    m_pos[2] += z;
  } else
    throw Kernel::Exception::NotImplementedError("Component::translate (for Parametrized Component)");
}

/** Translate the Component relative to the parent Component
 *  @param v :: translation vector
 */
void Component::translate(const V3D &v) {
  if (!m_map)
    m_pos += v;
  else
    throw Kernel::Exception::NotImplementedError("Component::translate (for Parametrized Component)");
}

/** Rotate the Component relative to the parent Component
 *  @param r :: translation vector
 */
void Component::rotate(const Quat &r) {
  if (!m_map)
    m_rot = m_rot * r;
  else
    throw Kernel::Exception::NotImplementedError("Component::rotate (for Parametrized Component)");
}

/** Rotate the Component by an angle in degrees with respect to an axis.
 * @param angle :: the number of degrees to rotate
 * @param axis :: The Vector to rotate around
 */
void Component::rotate(double angle, const V3D &axis) {
  (void)angle; // Avoid compiler warning
  (void)axis;  // Avoid compiler warning
  throw Kernel::Exception::NotImplementedError("Rotate(double angle, const V3D& axis) has not been implemented");
}

/** Get ScaleFactor of detector.  Looks at the "sca" parameter in the parameter
 *map.
 *
 * @returns A vector of the scale factors (1,1,1) if not set
 */
V3D Component::getScaleFactor() const {
  if (m_map) {
    if (hasComponentInfo()) {
      return m_map->componentInfo().scaleFactor(index());
    } else {
      Parameter_sptr par = m_map->get(m_base, ParameterMap::scale());
      if (par) {
        return par->value<V3D>();
      }
    }
  }
  return V3D(1, 1, 1);
}

/// Helper for legacy access mode. Returns the index of the component.
size_t Component::index() const { return m_map->componentIndex(this->getComponentID()); }

bool Component::hasComponentInfo() const {
  const IComponent *root = m_base;
  while (auto parent = root->getBareParent())
    root = parent;
  auto instrument = dynamic_cast<const Instrument *>(root);
  return m_map->hasComponentInfo(instrument);
}

/// Return the relative position to the parent Component
Kernel::V3D Component::getRelativePos() const {
  if (m_map) {

    if (hasComponentInfo()) {
      return m_map->componentInfo().relativePosition(index());
    } else {
      if (m_map->contains(m_base, "pos")) {
        return m_map->get(m_base, "pos")->value<V3D>();
      } else
        return m_base->m_pos;
    }
  } else {
    return m_pos;
  }
}

/// Return the absolute position of the Component
Kernel::V3D Component::getPos() const {
  if (m_map) {
    if (hasComponentInfo()) {
      return m_map->componentInfo().position(index());
    } else {
      // We currently have to treat detectors in a different way because
      // InfoComponentVisitor functionality is incomplete w.r.t DetectorInfo

      // Avoid instantiation of the parent's parameterized object if possible
      const IComponent *baseParent = m_base->m_parent;
      if (!baseParent) {
        return this->getRelativePos();
      } else {
        // Avoid instantiation of parent shared pointer if we can
        V3D absPos = this->getRelativePos();
        // get the parent rotation, try to get it from the cache first to avoid
        // instantiaing the class

        Quat parentRot;
        V3D parentPos;
        if (!(m_map->getCachedLocation(baseParent, parentPos) && m_map->getCachedRotation(baseParent, parentRot))) {
          // Couldn't get them from the cache, so I have to instantiate the
          // class
          std::shared_ptr<const IComponent> parParent = getParent();
          if (parParent) {
            parentRot = parParent->getRotation();
            parentPos = parParent->getPos();
          }
        }
        parentRot.rotate(absPos);
        absPos += parentPos;
        return absPos;
      }
    }
  } else {
    if (!m_parent) {
      return m_pos;
    } else {
      V3D absPos(m_pos);
      m_parent->getRotation().rotate(absPos);
      return absPos + m_parent->getPos();
    }
  }
}

/// Return the position of the component required on the instrument side by side view
std::optional<Kernel::V2D> Component::getSideBySideViewPos() const {
  if (m_map) {
    return m_base->getSideBySideViewPos();
  } else if (!m_parent) {
    return m_sidebysideviewpos;
  } else {
    if ((!m_sidebysideviewpos) && (!m_parent->getSideBySideViewPos()))
      return std::nullopt;
    else {
      V2D absPos = m_sidebysideviewpos ? *m_sidebysideviewpos : V2D{0., 0.};
      V2D parentPos = m_parent->getSideBySideViewPos() ? *m_parent->getSideBySideViewPos() : V2D{0., 0.};
      return absPos + parentPos;
    }
  }
}

/// Return the relative rotation of the Compoonent to the parent
Kernel::Quat Component::getRelativeRot() const {
  if (m_map) {
    if (hasComponentInfo()) {
      return m_map->componentInfo().relativeRotation(index());
    } else {
      if (m_map->contains(m_base, "rot")) {
        return m_map->get(m_base, "rot")->value<Quat>();
      }
      return m_base->m_rot;
    }
  } else {
    return m_rot;
  }
}

/// Return the absolute rotation of the Component
Kernel::Quat Component::getRotation() const {
  if (m_map) {
    if (hasComponentInfo()) {
      return m_map->componentInfo().rotation(index());
    } else {
      // Avoid instantiation of the parent's parameterized object if possible
      const IComponent *baseParent = m_base->m_parent;
      if (!baseParent) {
        return getRelativeRot();
      } else {
        Quat parentRot;
        if (!m_map->getCachedRotation(baseParent, parentRot)) {
          // Get the parent's rotation
          std::shared_ptr<const IComponent> parParent = getParent();
          if (parParent) {
            parentRot = parParent->getRotation();
          }
        }
        return parentRot * getRelativeRot();
      }
    }
  } else {
    // Not parametrized
    if (!m_parent)
      return m_rot;
    else
      return m_parent->getRotation() * m_rot;
  }
}

/** Gets the distance between two Components
 *  @param comp :: The Component to measure against
 *  @returns The distance
 */
double Component::getDistance(const IComponent &comp) const { return getPos().distance(comp.getPos()); }

/**
 * Get the bounding box for this component. It is no shape so gives an empty
 * box.
 * @param boundingBox :: [Out] The resulting bounding box is stored here.
 */
void Component::getBoundingBox(BoundingBox &boundingBox) const { boundingBox = BoundingBox(); }

/**
 * Get the names of the parameters for this component.
 * @param recursive :: If true, the parameters for all of the parent components
 * are also included
 * @returns A set of strings giving the parameter names for this component
 */
std::set<std::string> Component::getParameterNames(bool recursive) const {
  if (!m_map)
    return std::set<std::string>();

  std::set<std::string> names = m_map->names(this);
  if (recursive) {
    // Walk up the tree and find the parameters attached to the parent
    // components
    std::shared_ptr<const IComponent> parent = getParent();
    if (parent) {
      std::set<std::string> parentNames = parent->getParameterNames(true);
      names.insert(parentNames.begin(), parentNames.end());
    }
  }
  return names;
}

/**
 * Get the names of the parameters for this component and it's parents.
 * @returns A map of strings giving the parameter names and the component they
 * are from, warning this contains shared pointers keeping transient objects
 * alive, do not keep longer than needed
 */
std::map<std::string, ComponentID> Component::getParameterNamesByComponent() const {
  auto retVal = std::map<std::string, ComponentID>();
  if (!m_map)
    return retVal;

  auto names = m_map->names(this);
  for (const auto &name : names) {
    retVal.insert(std::pair<std::string, ComponentID>(name, this->getComponentID()));
  }

  // Walk up the tree and find the parameters attached to the parent components
  std::shared_ptr<const IComponent> parent = getParent();
  if (parent) {
    auto parentNames = parent->getParameterNamesByComponent();
    // this should discard duplicates
    retVal.insert(parentNames.begin(), parentNames.end());
  }
  return retVal;
}

/**
 * Returns a boolean indicating if the component has the named parameter
 * @param name :: The name of the parameter
 * @param recursive :: If true the parent components will also be searched
 * (Default: true)
 * @returns A boolean indicating if the search was successful or not.
 */
bool Component::hasParameter(const std::string &name, bool recursive) const {
  if (!m_map)
    return false;

  bool match_found(false);
  if (m_map->contains(this, name)) {
    match_found = true;
  } else if (recursive) {
    std::shared_ptr<const IComponent> parent = getParent();
    if (parent) {
      match_found = parent->hasParameter(name, true);
    } else {
      match_found = false;
    }
  } else {
    match_found = false;
  }
  return match_found;
}

/** Prints a text representation of itself
 * @param os :: The output stream to write to
 */
void Component::printSelf(std::ostream &os) const {
  std::shared_ptr<const IComponent> parent = getParent();
  os << "Name : " << getName() << '\n';
  os << "Type: " << this->type() << '\n';
  if (parent)
    os << "Parent: " << parent->getName() << '\n';
  else
    os << "Parent: None\n";

  os << "Position : " << getPos() << '\n';
  os << "Orientation :" << getRelativeRot() << '\n';
}

/** Prints a text representation
 * @param os :: The output stream to write to
 * @param comp :: The Component to output
 * @returns The output stream
 */
std::ostream &operator<<(std::ostream &os, const Component &comp) {
  comp.printSelf(os);
  return os;
}

//------------------------------------------------------------------------------------------------
/** Reads the XML attributes from Poco XML parser
 * @param attr :: XML attributes  */
void Component::readXMLAttributes(const Poco::XML::Attributes &attr) {
  UNUSED_ARG(attr);
  //    std::string pos = attr.getValue("", "pos");
  //    m_pos.fromString(pos);
  //    std::string rot = attr.getValue("", "rot");
  //    m_rot.fromString(rot);
}

void Component::writeXML(Poco::XML::XMLWriter &writer) const {
  Poco::XML::AttributesImpl attr;
  attr.addAttribute("", "pos", "", "", m_pos.toString());
  writer.startElement("", "Component", "", attr);
  writer.endElement("", "Component", "");
}

//------------------------------------------------------------------------------------------------
/** Append to an open XML string
 * @param xmlStream :: string to append to. */
void Component::appendXML(std::ostream &xmlStream) const {
  xmlStream << "<pos>";
  m_pos.write(xmlStream);
  xmlStream << "</pos>\n";
  xmlStream << "<rot>" << m_rot << "</rot>\n";
}

//--------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------

/**
 * Swap the current references to the un-parameterized component and
 * parameter map for new ones
 * @param base A pointer to the new un-parameterized component
 * @param pmap A pointer to the new parameter map
 */
void Component::swap(const Component *base, const ParameterMap *pmap) {
  m_base = base;
  m_map = pmap;
}
/**
 * Get a parameter's description.
 * Only parameterized component can have description
 *
 * @param pname :: The name of the parameter
 * @param recursive :: If true the search will walk up through the parent
 * components
 * @returns std::string describing parameter if such description is defined
 * or empty string if not.
 */

std::string Component::getParamDescription(const std::string &pname, bool recursive) const {
  if (!m_map) { // no description for non-parameterized components
    return std::string("");
  }
  Parameter_sptr param;
  if (recursive) {
    param = m_map->getRecursive(this, pname);
  } else {
    param = m_map->get(this, pname);
  }
  if (param)
    return param->getDescription();
  else
    return std::string("");
}
/** Get this parameter's description -- no recursive search within children*/
std::string Component::getDescription() const {
  auto name = this->getName();
  return this->getParamDescription(name, false);
}

/**
 * Get a parameter's short description
 * Only parameterized component can have description
 *
 * @param pname ::     The name of the parameter
 * @param recursive :: If true the search will walk up through the parent
 * components
 * @returns std::string describing parameter if such description is defined
 * or empty string if not.
 */
std::string Component::getParamShortDescription(const std::string &pname, bool recursive) const {
  if (!m_map) { // no tooltips for non-parameterized components
    return std::string("");
  }
  Parameter_sptr param;
  if (recursive) {
    param = m_map->getRecursive(this, pname);
  } else {
    param = m_map->get(this, pname);
  }
  if (param)
    return param->getShortDescription();
  else
    return std::string("");
}

/** Get a parameter's tooltip (short description) -- no recursive search within
 * children*/
std::string Component::getShortDescription() const {
  auto name = this->getName();
  return this->getParamShortDescription(name, false);
}
/**Set components description. Works for parameterized components only
 * @param descr ::  String which describes the component.
 */
void Component::setDescription(const std::string &descr) {
  if (m_map) {
    std::string name = this->getName();
    auto param = m_map->getRecursive(this, name);
    if (param) {
      param->setDescription(descr);
    } else {
      // I see no reason why component's parameter map should be constant.
      // But as I do not understand why it is constant, let's HACK!
      (const_cast<ParameterMap *>(m_map))->addString(this, name, "", &descr);
    }
  } else
    throw Kernel::Exception::NotImplementedError("Component::setDescription "
                                                 "not implemented for "
                                                 "non-Parametrized Component)");
}

size_t Component::registerContents(class ComponentVisitor &componentVisitor) const {
  return componentVisitor.registerGenericComponent(*this);
}

/**
 * Get fitting parameter from a look up table or a formula
 * @param pname :: The name of the parameter
 * @param xvalue :: xvalue to interpolate lookup table of parameter map
 * components
 * @returns A list of values
 */
double Component::getFittingParameter(const std::string &pname, double xvalue) const {
  if (m_map) {
    Parameter_sptr parameter = m_map->getRecursive(this, pname, "fitting");
    if (!parameter) {
      throw std::runtime_error(
          std::format("Fitting parameter={} could not be extracted from component={}", pname, this->getName()));
    }

    try {
      const auto &fitParam = parameter->value<FitParameter>();
      return fitParam.getValue(xvalue);
    } catch (...) {
      throw std::runtime_error(
          std::format("Unable to get lookup table for parameter={} from component={}", pname, this->getName()));
    }
  } else {
    throw std::runtime_error(std::format("Parameter map is not available in component={}", this->getName()));
  }
}

} // namespace Mantid::Geometry
