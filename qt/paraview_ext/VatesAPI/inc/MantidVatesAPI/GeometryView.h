// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GEOMETRY_VIEW_H
#define GEOMETRY_VIEW_H
#include "MantidKernel/System.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidVatesAPI/DimensionViewFactory.h"
namespace Mantid {
namespace VATES {
class QString;

/**
@class GeometryView
Abstract view for controlling multi-dimensional geometries.
@author Owen Arnold, Tessella plc
@date 03/06/2011
*/
class DLLExport GeometryView {
public:
  virtual void addDimensionView(DimensionView *) = 0;
  virtual std::string getGeometryXMLString() const = 0;
  virtual const DimensionViewFactory &getDimensionViewFactory() = 0;
  virtual ~GeometryView(){};
  virtual void raiseModified() = 0;
  virtual Mantid::VATES::BinDisplay getBinDisplayMode() const = 0;
};
} // namespace VATES
} // namespace Mantid

#endif
