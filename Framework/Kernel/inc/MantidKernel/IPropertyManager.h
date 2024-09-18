// Mantid Repository : https://github.com/mantidproject/mantid
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/PropertyWithValue.h"

#ifndef Q_MOC_RUN
#include <memory>
#endif

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace Json {
class Value;
}

namespace Mantid {
namespace Types {
namespace Core {
class DateAndTime;
}
} // namespace Types
namespace Kernel {

class DataItem;
class IPropertySettings;
class OptionalBool;
class Property;
class PropertyManager;
class SplittingInterval;
class LogFilter;
template <typename T> class TimeSeriesProperty;
template <typename T> class Matrix;

/** @class IPropertyManager IPropertyManager.h Kernel/IPropertyManager.h

 Interface to PropertyManager

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class PropertyMgr (see
 http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 20/11/2007
 @author Roman Tolchenov, Tessella plc
 @date 02/03/2009
 */
class MANTID_KERNEL_DLL IPropertyManager {
public:
  virtual ~IPropertyManager() = default;

  /// Function to declare properties (i.e. store them)
  virtual void declareProperty(std::unique_ptr<Property> p, const std::string &doc = "") = 0;

  /// Function to declare properties (i.e. store them)
  virtual void declareOrReplaceProperty(std::unique_ptr<Property> p, const std::string &doc = "") = 0;

  /** Add a property of the template type to the list of managed properties
   *  @param name :: The name to assign to the property
   *  @param value :: The initial value to assign to the property
   *  @param validator :: Pointer to the (optional) validator.
   *  @param doc :: The (optional) documentation string
   *  @param direction :: The (optional) direction of the property, in, out or
   * inout
   *  @throw Exception::ExistsError if a property with the given name already
   * exists
   *  @throw std::invalid_argument  if the name argument is empty
   */
  template <typename T>
  void declareProperty(const std::string &name, T value, IValidator_sptr validator = std::make_shared<NullValidator>(),
                       const std::string &doc = "", const unsigned int direction = Direction::Input) {
    std::unique_ptr<PropertyWithValue<T>> p = std::make_unique<PropertyWithValue<T>>(name, value, validator, direction);
    declareProperty(std::move(p), doc);
  }

  /** Add a property to the list of managed properties with no validator
   *  @param name :: The name to assign to the property
   *  @param value :: The initial value to assign to the property
   *  @param doc :: The documentation string
   *  @param direction :: The (optional) direction of the property, in
   * (default), out or inout
   *  @throw Exception::ExistsError if a property with the given name already
   * exists
   *  @throw std::invalid_argument  if the name argument is empty
   */
  template <typename T>
  void declareProperty(const std::string &name, T value, const std::string &doc,
                       const unsigned int direction = Direction::Input) {
    std::unique_ptr<PropertyWithValue<T>> p =
        std::make_unique<PropertyWithValue<T>>(name, value, std::make_shared<NullValidator>(), direction);
    declareProperty(std::move(p), doc);
  }

  /** Add a property of the template type to the list of managed properties
   *  @param name :: The name to assign to the property
   *  @param value :: The initial value to assign to the property
   *  @param direction :: The direction of the property, in, out or inout
   *  @throw Exception::ExistsError if a property with the given name already
   * exists
   *  @throw std::invalid_argument  if the name argument is empty
   */
  template <typename T> void declareProperty(const std::string &name, T value, const unsigned int direction) {
    std::unique_ptr<PropertyWithValue<T>> p =
        std::make_unique<PropertyWithValue<T>>(name, value, std::make_shared<NullValidator>(), direction);
    declareProperty(std::move(p));
  }

  /** Specialised version of declareProperty template method to prevent the
   * creation of a
   *  PropertyWithValue of type const char* if an argument in quotes is passed
   * (it will be
   *  converted to a string). The validator, if provided, needs to be a string
   * validator.
   *  @param name :: The name to assign to the property
   *  @param value :: The initial value to assign to the property
   *  @param validator :: Pointer to the (optional) validator. Ownership will be
   * taken over.
   *  @param doc :: The (optional) documentation string
   *  @param direction :: The (optional) direction of the property, in, out or
   * inout
   *  @throw Exception::ExistsError if a property with the given name already
   * exists
   *  @throw std::invalid_argument if the name argument is empty
   *  @throw std::invalid_argument if value is a nullptr
   */
  void declareProperty(const std::string &name, const char *value,
                       IValidator_sptr validator = std::make_shared<NullValidator>(),
                       const std::string &doc = std::string(), const unsigned int direction = Direction::Input) {
    if (value == nullptr)
      throw std::invalid_argument("Attempted to set " + name + " to nullptr");
    // Simply call templated method, converting character array to a string
    declareProperty(name, std::string(value), std::move(validator), doc, direction);
  }

  /** Specialised version of declareProperty template method to prevent the
   * creation of a
   *  PropertyWithValue of type const char* if an argument in quotes is passed
   * (it will be
   *  converted to a string). The validator, if provided, needs to be a string
   * validator.
   *  @param name :: The name to assign to the property
   *  @param value :: The initial value to assign to the property
   *  @param doc :: The (optional) documentation string
   *  @param validator :: Pointer to the (optional) validator. Ownership will be
   * taken over.
   *
   *
   *  @param direction :: The (optional) direction of the property, in, out or
   * inout
   *  @throw Exception::ExistsError if a property with the given name already
   * exists
   *  @throw std::invalid_argument if the name argument is empty
   *  @throw std::invalid_argument if value is a nullptr
   */
  void declareProperty(const std::string &name, const char *value, const std::string &doc,
                       IValidator_sptr validator = std::make_shared<NullValidator>(),
                       const unsigned int direction = Direction::Input) {
    if (value == nullptr)
      throw std::invalid_argument("Attempted to set " + name + " to nullptr");
    // Simply call templated method, converting character array to a string
    declareProperty(name, std::string(value), std::move(validator), doc, direction);
  }

  /** Add a property of string type to the list of managed properties
   *  @param name :: The name to assign to the property
   *  @param value :: The initial value to assign to the property
   *  @param direction :: The direction of the property, in, out or inout
   *  @throw Exception::ExistsError if a property with the given name already
   * exists
   *  @throw std::invalid_argument  if the name argument is empty
   */
  void declareProperty(const std::string &name, const char *value, const unsigned int direction) {
    if (value == nullptr)
      throw std::invalid_argument("Attempted to set " + name + " to nullptr");

    declareProperty(name, std::string(value), std::make_shared<NullValidator>(), "", direction);
  }

  /// Removes the property from management
  virtual void removeProperty(const std::string &name, const bool delproperty = true) = 0;

  /// Removes the property from management and returns a pointer to it
  virtual std::unique_ptr<Property> takeProperty(const size_t index) = 0;

  virtual void resetProperties() = 0;

  /** Sets all the declared properties from a string.
    @param propertiesString :: A list of name = value pairs separated by a
     semicolon
    @param ignoreProperties :: A set of names of any properties NOT to set
    from the propertiesArray
  */
  virtual void setPropertiesWithString(
      const std::string &propertiesString,
      const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>()) = 0;

  /** Sets all properties from a string.
      @param propertiesJson :: A string of name = value pairs formatted
        as a json name value pair collection
      @param ignoreProperties :: A set of names of any properties NOT to set
      from the propertiesArray
      @param createMissing :: If the property does not exist then create it
   */
  virtual void
  setProperties(const std::string &propertiesJson,
                const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>(),
                bool createMissing = false) = 0;

  /** Sets all the properties from a json object
     @param jsonValue :: A json name value pair collection
     @param ignoreProperties :: A set of names of any properties NOT to set
     from the propertiesArray
     @param createMissing :: If the property does not exist then create it
  */
  virtual void
  setProperties(const ::Json::Value &jsonValue,
                const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>(),
                bool createMissing = false) = 0;

  /** Sets property value from a string
      @param name :: Property name
      @param value :: New property value
   */
  virtual void setPropertyValue(const std::string &name, const std::string &value) = 0;

  /** Sets property value from a Json::Value
      @param name :: Property name
      @param value :: New property value
   */
  virtual void setPropertyValueFromJson(const std::string &name, const Json::Value &value) = 0;

  /// Set the value of a property by an index
  virtual void setPropertyOrdinal(const int &index, const std::string &value) = 0;

  /// Checks whether the named property is already in the list of managed
  /// property.
  virtual bool existsProperty(const std::string &name) const = 0;

  /// Validates all the properties in the collection
  virtual bool validateProperties() const = 0;
  /// Returns the number of properties under management
  virtual size_t propertyCount() const = 0;
  /// Get the value of a property as a string
  virtual std::string getPropertyValue(const std::string &name) const = 0;

  /// Get the list of managed properties.
  virtual const std::vector<Property *> &getProperties() const = 0;

  /// Get the list of managed property names.
  virtual std::vector<std::string> getDeclaredPropertyNames() const noexcept = 0;

  /** Templated method to set the value of a PropertyWithValue
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property
   * of different type
   */
  template <typename T> IPropertyManager *setProperty(const std::string &name, const T &value) {
    return doSetProperty(name, value);
  }

  /** Templated method to set the value of a PropertyWithValue from a
   * std::unique_ptr
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property
   * of different type
   */
  template <typename T> IPropertyManager *setProperty(const std::string &name, std::unique_ptr<T> value) {
    setTypedProperty(name, std::move(value), std::is_convertible<std::unique_ptr<T>, std::shared_ptr<DataItem>>());
    this->afterPropertySet(name);
    return this;
  }

  /** Specialised version of setProperty template method to handle const char *
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property
   *  @throw std::invalid_argument If value is a nullptr
   * of different type
   */
  IPropertyManager *setProperty(const std::string &name, const char *value) {
    if (value == nullptr)
      throw std::invalid_argument("Attempted to set " + name + " to nullptr");
    return setProperty(name, std::string(value));
  }

  /** Specialised version of setProperty template method to handle std::string
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property
   * of different type
   */
  IPropertyManager *setProperty(const std::string &name, const std::string &value) {
    this->setPropertyValue(name, value);
    return this;
  }

  /// Update values of the existing properties.
  void updatePropertyValues(const IPropertyManager &other);

  /// Return the property manager serialized as a string.
  virtual std::string asString(bool withDefaultValues = false) const = 0;

  /// Return the property manager serialized as a json object.
  virtual ::Json::Value asJson(bool withDefaultValues = false) const = 0;

  void setPropertySettings(const std::string &name, std::unique_ptr<IPropertySettings> settings);

  /** Set the group for a given property
   * @param name :: property name
   * @param group :: Name of the group it belongs to     */
  void setPropertyGroup(const std::string &name, const std::string &group) {
    Property *prop = getPointerToProperty(name);
    if (prop)
      prop->setGroup(group);
  }

  /// Get the list of managed properties in a given group.
  std::vector<Property *> getPropertiesInGroup(const std::string &group) const;

  virtual void filterByProperty(Mantid::Kernel::LogFilter * /*logFilter*/, const std::vector<std::string> &
                                /* excludedFromFiltering */) {
    throw std::logic_error("Not implemented yet.");
  }

protected:
  /// Get a property by an index
  virtual Property *getPointerToPropertyOrdinal(const int &index) const = 0;

  /** Templated method to get the value of a property.
   *  No generic definition, only specialised ones. Although the definitions are
   * mostly the
   *  same, Visual Studio can't cope with
   *  @param name :: The name of the property (case insensitive)
   *  @return The value of the property
   *  @throw std::runtime_error If an attempt is made to assign a property to a
   * different type
   *  @throw Exception::NotFoundError If the property requested does not exist
   */
  template <typename T> T getValue(const std::string &name) const;

  /// Clears all properties under management
  virtual void clear() = 0;
  /// Override this method to perform a custom action right after a property was
  /// set.
  /// The argument is the property name. Default - do nothing.
  virtual void afterPropertySet(const std::string &) {}

  /// Utility class that enables the getProperty() method to effectively be
  /// templated on the return type
  struct MANTID_KERNEL_DLL TypedValue {
    /// Reference to the containing PropertyManager
    const IPropertyManager &pm;
    /// The name of the property desired
    const std::string prop;

    /// Constructor
    TypedValue(const IPropertyManager &p, const std::string &name) : pm(p), prop(name) {}

    // Unfortunately, MSVS2010 can't handle just having a single templated cast
    // operator T()
    // (it has problems with the string one). This operator is needed to convert
    // a TypedValue
    // into what we actually want. It can't even handle just having a
    // specialization for strings.
    // So we have to explicitly define an operator for each type of property
    // that we have.

    operator int16_t();
    operator uint16_t();
    operator int32_t();
    operator uint32_t();
    operator int64_t();
    operator uint64_t();
    operator OptionalBool();

#ifdef __APPLE__
    operator unsigned long();
#endif
    /// explicit specialization for bool()
    operator bool();
    /// explicit specialization for double()
    operator double();
    /// explicit specialization for std::string()
    operator std::string();
    /// explicit specialization for Property*()
    operator Property *();

    /// explicit specialization for std::vector
    template <typename T> operator std::vector<T>() { return pm.getValue<std::vector<T>>(prop); }
    /// explicit specialization for std::vector
    template <typename T> operator std::vector<std::vector<T>>() {
      return pm.getValue<std::vector<std::vector<T>>>(prop);
    }
    /// explicit specialization for std::shared_ptr
    template <typename T> operator std::shared_ptr<T>() { return pm.getValue<std::shared_ptr<T>>(prop); }
    /// explicit specialization for std::shared_ptr to const T
    template <typename T> operator std::shared_ptr<const T>() { return pm.getValue<std::shared_ptr<T>>(prop); }
    /// explicit version for Matrices
    template <typename T> operator Matrix<T>() { return pm.getValue<Matrix<T>>(prop); }
  };

public:
  /// Get the value of a property
  virtual TypedValue getProperty(const std::string &name) const = 0;
  /// Get a pointer to property by name
  virtual Property *getPointerToProperty(const std::string &name) const = 0;

private:
  /** Helper method to set the value of a PropertyWithValue
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property
   * of different type
   */
  template <typename T> IPropertyManager *doSetProperty(const std::string &name, const T &value) {
    setTypedProperty(name, value, std::is_convertible<T, std::shared_ptr<DataItem>>());
    this->afterPropertySet(name);
    return this;
  }

  /** Helper method to set the value of a PropertyWithValue, variant for
   * shared_ptr types. This variant is required to enforce checks for complete
   * types, do not remove it.
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property
   * of different type
   */
  template <typename T> IPropertyManager *doSetProperty(const std::string &name, const std::shared_ptr<T> &value) {
    // CAREFUL: is_convertible has undefined behavior for incomplete types. If T
    // is forward-declared in the calling code, e.g., an algorithm that calls
    // setProperty, compilation and linking do work. However, the BEHAVIOR IS
    // UNDEFINED and the compiler will not complain, but things crash or go
    // wrong badly. To circumvent this we call `sizeof` here to force a compiler
    // error if T is an incomplete type.
    static_cast<void>(sizeof(T)); // DO NOT REMOVE, enforces complete type
    setTypedProperty(name, value, std::is_convertible<T *, DataItem *>());
    this->afterPropertySet(name);
    return this;
  }

  /**
   * Set a property value that is not convertible to a DataItem_sptr
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property
   * of different type
   */
  template <typename T>
  IPropertyManager *setTypedProperty(const std::string &name, const T &value, const std::false_type &) {
    PropertyWithValue<T> *prop = dynamic_cast<PropertyWithValue<T> *>(getPointerToProperty(name));
    if (prop) {
      *prop = value;
    } else {
      throw std::invalid_argument("Attempt to assign to property (" + name + ") of incorrect type");
    }
    return this;
  }
  /**
   * Set a property value that is convertible to a DataItem_sptr
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property
   * of different type
   */
  template <typename T>
  IPropertyManager *setTypedProperty(const std::string &name, const T &value, const std::true_type &) {
    // T is convertible to DataItem_sptr
    std::shared_ptr<DataItem> data = std::static_pointer_cast<DataItem>(value);
    std::string error = getPointerToProperty(name)->setDataItem(data);
    if (!error.empty()) {
      throw std::invalid_argument(error);
    }
    return this;
  }

  /**
   * Set a property value from std::unique_ptr that is convertible to a
   * DataItem_sptr
   *  @param name :: The name of the property (case insensitive)
   *  @param value :: The value to assign to the property
   *  @throw Exception::NotFoundError If the named property is unknown
   *  @throw std::invalid_argument If an attempt is made to assign to a property
   * of different type
   */
  template <typename T>
  IPropertyManager *setTypedProperty(const std::string &name, std::unique_ptr<T> value, const std::true_type &) {
    // T is convertible to DataItem_sptr
    std::shared_ptr<DataItem> data(std::move(value));
    std::string error = getPointerToProperty(name)->setDataItem(data);
    if (!error.empty()) {
      throw std::invalid_argument(error);
    }
    return this;
  }
};

} // namespace Kernel
} // namespace Mantid

/// A macro for defining getValue functions for new types. Puts them in the
/// Mantid::Kernel namespace so
/// the macro should be used outside of any namespace scope
#define DEFINE_IPROPERTYMANAGER_GETVALUE(type)                                                                         \
  namespace Mantid {                                                                                                   \
  namespace Kernel {                                                                                                   \
  template <> DLLExport type IPropertyManager::getValue<type>(const std::string &name) const {                         \
    PropertyWithValue<type> *prop = dynamic_cast<PropertyWithValue<type> *>(getPointerToProperty(name));               \
    if (prop) {                                                                                                        \
      return *prop;                                                                                                    \
    } else {                                                                                                           \
      std::string message = "Attempt to assign property " + name + " to incorrect type. Expected type " #type;         \
      throw std::runtime_error(message);                                                                               \
    }                                                                                                                  \
  }                                                                                                                    \
  }                                                                                                                    \
  }
