// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid {
namespace Kernel {

/**
 * Default constructor
 */
PropertyManagerDataServiceImpl::PropertyManagerDataServiceImpl()
    : DataService<PropertyManager>("PropertyManagerDataService") {}

} // Namespace Kernel
} // Namespace Mantid
