#include "MantidAPI/APIComponentVisitor.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {
namespace API {

using namespace Mantid::Geometry;

APIComponentVisitor::APIComponentVisitor(
    const Mantid::API::DetectorInfo &detectorInfo)
    : m_detectorInfo(detectorInfo) {}

void APIComponentVisitor::registerComponentAssembly(
    const ICompAssembly &bank, std::vector<size_t> &parentDetectorIndexes) {

  std::vector<size_t> localDetectorIndexes;
  std::vector<IComponent_const_sptr> bankChildren;
  bank.getChildren(bankChildren, false /*is recursive*/);

  for (const auto &child : bankChildren) {
    child->registerContents(*this, localDetectorIndexes);
  }

  parentDetectorIndexes.reserve(parentDetectorIndexes.size() +
                                localDetectorIndexes.size());
  parentDetectorIndexes.insert(parentDetectorIndexes.end(),
                               localDetectorIndexes.begin(),
                               localDetectorIndexes.end());
  m_componentDetectorIndexes.emplace_back(localDetectorIndexes);
  m_componentIds.emplace_back(bank.getComponentID());
}

void APIComponentVisitor::registerGenericComponent(const IComponent &component,
                                                   std::vector<size_t> &) {
  m_componentDetectorIndexes.emplace_back(std::vector<size_t>());
  m_componentIds.emplace_back(component.getComponentID());
}
void APIComponentVisitor::registerDetector(
    const IDetector &detector, std::vector<size_t> &parentDetectorIndexes) {

  parentDetectorIndexes.push_back(m_detectorInfo.indexOf(detector.getID()));
  m_componentDetectorIndexes.emplace_back(std::vector<size_t>());
  m_componentIds.emplace_back(detector.getComponentID());
}
APIComponentVisitor::~APIComponentVisitor() {}
std::vector<Mantid::Geometry::ComponentID>
APIComponentVisitor::componentIds() const {
  return m_componentIds;
}
std::vector<std::vector<size_t>>
APIComponentVisitor::componentDetectorIndexes() const {
  return m_componentDetectorIndexes;
}

} // namespace API
} // namespace Mantid
