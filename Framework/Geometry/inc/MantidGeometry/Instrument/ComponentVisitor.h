#ifndef COMPONENTVISITOR_H
#define COMPONENTVISITOR_H

#include <vector>
#include <cstddef>
namespace Mantid {
namespace Geometry {

class ICompAssembly;
class IDetector;
class IComponent;

class ComponentVisitor {
public:
  virtual void
  registerComponentAssembly(const ICompAssembly &assembly,
                            std::vector<size_t> &detectorIndexes) = 0;
  virtual void
  registerGenericComponent(const IComponent &component,
                           std::vector<size_t> &detectorIndexes) = 0;
  virtual void registerDetector(const IDetector &detector,
                                std::vector<size_t> &detectorIndexes) = 0;
};
}
}
#endif
