#include "MantidGeometry/Instrument/CacheComponentVisitor.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {
namespace Geometry {

void CacheComponentVisitor::registerComponentAssembly(
    const Geometry::ICompAssembly &assembly) {

  std::vector<IComponent_const_sptr> assemblyChildren;
  assembly.getChildren(assemblyChildren, false /*is recursive*/);

  for (const auto &child : assemblyChildren) {
    // register everything under this assembly
    child->registerContents(*this);
  }
  m_componentIds.emplace_back(assembly.getComponentID());
}

void CacheComponentVisitor::registerGenericComponent(
    const Geometry::IComponent &component) {

  m_componentIds.emplace_back(component.getComponentID());
}

void CacheComponentVisitor::registerDetector(
    const Geometry::IDetector &detector) {
  m_componentIds.emplace_back(detector.getComponentID());
}

std::vector<Geometry::IComponent *>
CacheComponentVisitor::componentIds() const {
  return m_componentIds;
}

} // namespace Geometry
} // namespace Mantid
