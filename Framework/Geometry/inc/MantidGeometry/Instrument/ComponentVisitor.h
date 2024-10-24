// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cstddef>
namespace Mantid {
namespace Geometry {

class ICompAssembly;
class IDetector;
class IComponent;
class IObjComponent;
class RectangularDetector;
class ObjCompAssembly;

/** ComponentVisitor : Visitor for IComponents. Enables parsing of a full doubly
  linked InstrumentTree without need for dynamic casts. Public methods are
  called by
  accepting IComponent
*/
class ComponentVisitor {
public:
  virtual size_t registerComponentAssembly(const ICompAssembly &assembly) = 0;
  virtual size_t registerGenericComponent(const IComponent &component) = 0;
  virtual size_t registerInfiniteComponent(const Mantid::Geometry::IComponent &component) = 0;
  virtual size_t registerGenericObjComponent(const IObjComponent &objComponent) = 0;
  virtual size_t registerInfiniteObjComponent(const IObjComponent &component) = 0;
  virtual size_t registerDetector(const IDetector &detector) = 0;
  virtual size_t registerGridBank(const ICompAssembly &bank) = 0;
  virtual size_t registerRectangularBank(const ICompAssembly &bank) = 0;
  virtual size_t registerStructuredBank(const ICompAssembly &bank) = 0;
  virtual size_t registerObjComponentAssembly(const ObjCompAssembly &obj) = 0;
  virtual ~ComponentVisitor() = default;
};
} // namespace Geometry
} // namespace Mantid
