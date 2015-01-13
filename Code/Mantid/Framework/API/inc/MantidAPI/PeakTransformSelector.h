#ifndef MANTID_API_PEAKTRANSFORMSELECTOR_H_
#define MANTID_API_PEAKTRANSFORMSELECTOR_H_

#include "MantidAPI/PeakTransformFactory.h"
#include <set>

namespace Mantid {
namespace API {
/**
@class PeakTransformSelector
Used to choose an appropriate PeakTransformFactory
*/
class DLLExport PeakTransformSelector {
public:
  /// Constructor
  PeakTransformSelector();
  /// Destructor
  ~PeakTransformSelector();
  /// Register a candidate factory
  void registerCandidate(PeakTransformFactory_sptr candidate);
  /// Make choice
  PeakTransformFactory_sptr makeChoice(const std::string labelX,
                                       const std::string labelY) const;
  /// Make default choice
  PeakTransformFactory_sptr makeDefaultChoice() const;
  /// Has a factory capable of the requested transform.
  bool hasFactoryForTransform(const std::string labelX,
                              const std::string labelY) const;
  /// Get the number of registered factories
  size_t numberRegistered() const;

private:
  /// Disabled copy constructor
  PeakTransformSelector(const PeakTransformSelector &);
  /// Disabled assigment operator
  PeakTransformSelector &operator=(const PeakTransformSelector &);
  /// Collection of candidate factories.
  typedef std::set<PeakTransformFactory_sptr> Factories;
  Factories m_candidateFactories;
};
}
}

#endif
