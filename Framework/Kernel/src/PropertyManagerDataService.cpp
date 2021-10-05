// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid::Kernel {

/**
 * Default constructor
 */
PropertyManagerDataServiceImpl::PropertyManagerDataServiceImpl()
    : DataService<PropertyManager>("PropertyManagerDataService") {}

} // namespace Mantid::Kernel
