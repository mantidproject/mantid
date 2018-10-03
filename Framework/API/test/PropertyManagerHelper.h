// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef APITEST_PROPERTYMANAGERHELPER_H_
#define APITEST_PROPERTYMANAGERHELPER_H_

#include "MantidKernel/PropertyManager.h"

/**
 * Helper class to use IPropertyManager methods in the DataObjects tests
 */
class PropertyManagerHelper : public Mantid::Kernel::PropertyManager {
public:
  PropertyManagerHelper() : PropertyManager() {}

  using IPropertyManager::TypedValue;
  using IPropertyManager::getValue;
  using PropertyManager::declareProperty;
  using PropertyManager::setProperty;
};

#endif
