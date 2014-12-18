#include "MantidAPI/PeakTransformSelector.h"
#include <stdexcept>

namespace Mantid {
namespace API {
/// Constructor
PeakTransformSelector::PeakTransformSelector() {}

/**
Register a peak transform factory as a candidate.
@param candidate : candidate peak transform factory
*/
void
PeakTransformSelector::registerCandidate(PeakTransformFactory_sptr candidate) {
  m_candidateFactories.insert(candidate);
}

/**
@return the number of registered candidates.
*/
size_t PeakTransformSelector::numberRegistered() const {
  return m_candidateFactories.size();
}

/**
Make a choice for the peak transform factory, but use the default labels known
to each factory.
@return selected factory
*/
PeakTransformFactory_sptr PeakTransformSelector::makeDefaultChoice() const {
  if (numberRegistered() == 0) {
    throw std::runtime_error("Nothing registered.");
  }

  PeakTransformFactory_sptr selected;
  bool found = false;
  for (auto it = m_candidateFactories.begin(); it != m_candidateFactories.end();
       ++it) {
    try {
      PeakTransformFactory_sptr temp = (*it);
      temp->createDefaultTransform();
      selected = temp;
      found = true;
    } catch (PeakTransformException &) {
    }
  }
  if (!found) {
    throw std::invalid_argument(
        "PeakTransformSelector could not find a suitable transform");
  }
  return selected;
}

/**
Make a choice for the peak transform factory.
@param labelX: X-label to use in determining selection.
@param labelY: Y-label to use in determining selection.
@return selected factory
*/
PeakTransformFactory_sptr
PeakTransformSelector::makeChoice(const std::string labelX,
                                  const std::string labelY) const {
  if (labelX.empty()) {
    throw std::invalid_argument("labelX is empty");
  }
  if (labelY.empty()) {
    throw std::invalid_argument("labelY is empty");
  }
  if (numberRegistered() == 0) {
    throw std::runtime_error("Nothing registered.");
  }

  PeakTransformFactory_sptr selected;
  bool found = false;
  for (auto it = m_candidateFactories.begin(); it != m_candidateFactories.end();
       ++it) {
    try {
      PeakTransformFactory_sptr temp = (*it);
      temp->createTransform(labelX, labelY);
      selected = temp;
      found = true;
    } catch (PeakTransformException &) {
    }
  }
  if (!found) {
    std::stringstream ss;
    ss << "PeakTransformSelector could not find a suitable transform for "
          "labelX " << labelX << " labelY " << labelY;
    throw std::invalid_argument(ss.str());
  }
  return selected;
}

/**
Can any of the registered peak transform factories peform the requested
transformation.
@param labelX: X-label to use in determining selection.
@param labelY: Y-label to use in determining selection.
@return TRUE only if such a factory is available.
*/
bool
PeakTransformSelector::hasFactoryForTransform(const std::string labelX,
                                              const std::string labelY) const {
  bool hasFactoryForTransform = true;
  try {
    this->makeChoice(labelX, labelY);
  } catch (std::invalid_argument &) {
    hasFactoryForTransform = false;
  }
  return hasFactoryForTransform;
}

/// Destructor
PeakTransformSelector::~PeakTransformSelector() {}
}
}
