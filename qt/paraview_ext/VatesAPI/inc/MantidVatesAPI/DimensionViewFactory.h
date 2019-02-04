// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef _DIMENSION_VIEW_FACTORY_H
#define _DIMENSION_VIEW_FACTORY_H

#include "MantidVatesAPI/DimensionView.h"
namespace Mantid {
namespace VATES {
/**
class Abstract DimensionViewFactory provides Factory Method
*/
class DLLExport DimensionViewFactory {
public:
  virtual DimensionView *create() const = 0;
  virtual ~DimensionViewFactory() {}
};
} // namespace VATES
} // namespace Mantid

#endif
