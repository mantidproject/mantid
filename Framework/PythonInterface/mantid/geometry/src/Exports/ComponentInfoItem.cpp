// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ComponentInfoItem.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include <boost/python/class.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/module.hpp>

using Mantid::Geometry::ComponentInfo;
using Mantid::Geometry::ComponentInfoItem;
using Mantid::Kernel::V3D;
using namespace boost::python;
using namespace Mantid::PythonInterface::Converters;
using namespace Mantid::PythonInterface::Policies;

// Export DetectorInfoItem
void export_ComponentInfoItem() {

  // Export to Python
  class_<ComponentInfoItem<ComponentInfo>>("ComponentInfoItem", no_init)
      .add_property("isDetector", &ComponentInfoItem<ComponentInfo>::isDetector)
      .add_property(
          "componentsInSubtree",
          make_function(&ComponentInfoItem<ComponentInfo>::componentsInSubtree,
                        return_value_policy<VectorToNumpy>()))
      .add_property(
          "detectorsInSubtree",
          make_function(&ComponentInfoItem<ComponentInfo>::detectorsInSubtree,
                        return_value_policy<VectorToNumpy>()))
      .add_property("position", &ComponentInfoItem<ComponentInfo>::position)
      .add_property("rotation", &ComponentInfoItem<ComponentInfo>::rotation)
      .add_property("parent", &ComponentInfoItem<ComponentInfo>::parent)
      .add_property("hasParent", &ComponentInfoItem<ComponentInfo>::hasParent)
      .add_property("scaleFactor",
                    &ComponentInfoItem<ComponentInfo>::scaleFactor)
      .add_property("name", &ComponentInfoItem<ComponentInfo>::name)
      .add_property(
          "children",
          make_function(&ComponentInfoItem<ComponentInfo>::children,
                        return_value_policy<VectorRefToNumpy<WrapReadOnly>>()))
      .add_property("index", &ComponentInfoItem<ComponentInfo>::index);
}
