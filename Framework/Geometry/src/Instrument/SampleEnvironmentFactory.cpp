#include "MantidGeometry/Instrument/SampleEnvironmentFactory.h"

namespace Mantid {
namespace Geometry {

//------------------------------------------------------------------------------
// Anonyomous
//------------------------------------------------------------------------------
namespace {

// Typedef for cache
using SampleEnvironmentSpecCache =
    std::unordered_map<std::string, SampleEnvironmentSpec_uptr>;

/**
 * If it doesn't exist create the static cache, otherwise return a reference to
 * it
 * @return A reference to the static cache
 */
SampleEnvironmentSpecCache &retrieveSpecCache() {
  static SampleEnvironmentSpecCache cache;
  return cache;
}
}

//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------
/**
 * Create a new SampleEnvironment instance from the given specification and can.
 * @param specFinder A reference to an object used to retrieve a
 * SampleEnvironmentSpec
 * @param specName The name of a specification
 * @param canName The name of a can within the spec
 * @return A new instance of the given environment
 */
SampleEnvironment_uptr
SampleEnvironmentFactory::create(const ISampleEnvironmentSpecFinder &specFinder,
                                 const std::string &specName,
                                 const std::string &canName) {
  auto &specCache = retrieveSpecCache();
  SampleEnvironmentSpec *spec(nullptr);
  auto iter = specCache.find(specName);
  if (iter != specCache.end()) {
    spec = iter->second.get();
  } else {
    auto specUPtr = specFinder.find(specName);
    spec = specUPtr.get();
    specCache.insert(std::make_pair(specName, std::move(specUPtr)));
  }
  return spec->buildEnvironment(canName);
}

} // namespace Geometry
} // namespace Mantid
