#include "MantidAPI/InfoComponentVisitor.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {
namespace API {

using namespace Mantid::Geometry;

InfoComponentVisitor::InfoComponentVisitor(
    const Mantid::API::DetectorInfo &detectorInfo)
    : m_detectorInfo(detectorInfo) {}

void InfoComponentVisitor::registerComponentAssembly(
    const ICompAssembly &bank, std::vector<size_t> &parentDetectorIndexes) {

  // Local cache of immediate child detector indexes
  std::vector<size_t> localDetectorIndexes;
  std::vector<IComponent_const_sptr> assemblyChildren;
  bank.getChildren(assemblyChildren, false /*is recursive*/);

  for (const auto &child : assemblyChildren) {
    // register everything under this assembly
    child->registerContents(*this, localDetectorIndexes);
  }

  /*
   * The following allows us to refer to ALL nested
   * detectors from any component in the tree. Otherwise these could
   * be hidden by sub-tree assemblies.
   */
  parentDetectorIndexes.reserve(parentDetectorIndexes.size() +
                                localDetectorIndexes.size());
  parentDetectorIndexes.insert(parentDetectorIndexes.end(),
                               localDetectorIndexes.begin(),
                               localDetectorIndexes.end());
  m_componentDetectorIndexes.emplace_back(localDetectorIndexes);
  m_componentIds.emplace_back(bank.getComponentID());
}

void InfoComponentVisitor::registerGenericComponent(const IComponent &component,
                                                   std::vector<size_t> &) {
  /*
   * For a generic leaf component we extend the component ids list, but
   * the detector indexes entries will of course be empty
   */
  m_componentDetectorIndexes.emplace_back(std::vector<size_t>());
  m_componentIds.emplace_back(component.getComponentID());
}
void InfoComponentVisitor::registerDetector(
    const IDetector &detector, std::vector<size_t> &parentDetectorIndexes) {

  /*
   * Add the detector index to the parent collection, but as each detector
   * is a leaf this components detector indexes are empty.
   */
  parentDetectorIndexes.push_back(m_detectorInfo.indexOf(detector.getID()));
  m_componentDetectorIndexes.emplace_back(std::vector<size_t>());
  m_componentIds.emplace_back(detector.getComponentID());
}
InfoComponentVisitor::~InfoComponentVisitor() {}

std::vector<Mantid::Geometry::ComponentID>
InfoComponentVisitor::componentIds() const {
  return m_componentIds;
}
std::vector<std::vector<size_t>>
InfoComponentVisitor::componentDetectorIndexes() const {
  return m_componentDetectorIndexes;
}

} // namespace API
} // namespace Mantid
