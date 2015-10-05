#ifndef MANTID_KERNEL_PROPERTYMANAGER_H_
#define MANTID_KERNEL_PROPERTYMANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <map>

#include "MantidKernel/IPropertyManager.h"

namespace Mantid {
namespace Kernel {
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class SplittingInterval;
template <typename T> class TimeSeriesProperty;

/**
 Property manager helper class.
 This class is used by algorithms and services for helping to manage their own
 set of properties.

 N.B. ONCE YOU HAVE DECLARED A PROPERTY TO THE MANAGER IT IS OWNED BY THIS CLASS
 (I.E. DON'T DELETE IT!)

 Property values of any type except std::string can be extracted using
 getProperty().
 For assignment of string properties it is necessary to use getPropertyValue().

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class PropertyMgr (see
 http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 20/11/2007

 Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_KERNEL_DLL PropertyManager : virtual public IPropertyManager {
public:
  PropertyManager();
  PropertyManager(const PropertyManager &);
  PropertyManager &operator=(const PropertyManager &);
  PropertyManager &operator+=(const PropertyManager &rhs);

  void filterByTime(const Kernel::DateAndTime &start,
                    const Kernel::DateAndTime &stop);
  void splitByTime(std::vector<SplittingInterval> &splitter,
                   std::vector<PropertyManager *> outputs) const;
  void filterByProperty(const TimeSeriesProperty<bool> &filter);

  virtual ~PropertyManager();

  // Function to declare properties (i.e. store them)
  void declareProperty(Property *p, const std::string &doc = "");

  // Sets all the declared properties from
  void setProperties(const std::string &propertiesArray);
  void setPropertyValue(const std::string &name, const std::string &value);
  void setPropertyOrdinal(const int &index, const std::string &value);

  bool existsProperty(const std::string &name) const;
  bool validateProperties() const;
  size_t propertyCount() const;
  std::string getPropertyValue(const std::string &name) const;
  const std::vector<Property *> &getProperties() const;

  /// removes the property from properties map
  void removeProperty(const std::string &name, const bool delproperty = true);
  /// Clears the whole property map
  void clear();

  /// Get the value of a property
  TypedValue getProperty(const std::string &name) const;
  /// Return the property manager serialized as a string.
  virtual std::string asString(bool withDefaultValues = false,
                               char separator = ',') const;

protected:
  using IPropertyManager::declareProperty;

  friend class PropertyManagerOwner;

  Property *getPointerToProperty(const std::string &name) const;
  Property *getPointerToPropertyOrdinal(const int &index) const;
  Property *getPointerToPropertyOrNull(const std::string &name) const;

private:
  /// Transform the given string to a key for the property index
  const std::string createKey(const std::string &text) const;

  /// typedef for the map holding the properties
  typedef std::map<std::string, Property *> PropertyMap;
  /// The properties under management
  PropertyMap m_properties;
  /// Stores the order in which the properties were declared.
  std::vector<Property *> m_orderedProperties;
};

/// Typedef for a shared pointer to a PropertyManager
typedef boost::shared_ptr<PropertyManager> PropertyManager_sptr;

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYMANAGER_H_*/
