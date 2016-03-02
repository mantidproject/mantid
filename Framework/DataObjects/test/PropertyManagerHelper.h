#ifndef DATAOBJECTSTEST_PROPERTYMANAGERHELPER_H_
#define DATAOBJECTSTEST_PROPERTYMANAGERHELPER_H_

#include "MantidKernel/PropertyManager.h"

/**
* Helper class to use IPropertyManager methods in the DataObjects tests
*/
class PropertyManagerHelper : public Mantid::Kernel::PropertyManager {
public:
  PropertyManagerHelper() : PropertyManager() {}

  using PropertyManager::declareProperty;
  using PropertyManager::setProperty;
  using IPropertyManager::TypedValue;
  using IPropertyManager::getValue;
};

#endif
