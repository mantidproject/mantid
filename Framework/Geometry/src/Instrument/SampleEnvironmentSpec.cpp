// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/SampleEnvironmentSpec.h"

namespace Mantid {
namespace Geometry {

/**
 * Constructor
 * @param name The string name of the specification
 */
SampleEnvironmentSpec::SampleEnvironmentSpec(std::string name)
    : m_name(std::move(name)), m_cans(), m_components() {}

/**
 * Find a can by id string
 * @param id The string id to search for
 * @return A pointer to the retrieved Container instance
 * @throws std::invalid_argument
 */
Container_const_sptr
SampleEnvironmentSpec::findContainer(const std::string &id) const {
  auto indexIter = m_cans.find(id);
  if (indexIter != m_cans.end())
    return indexIter->second;
  else
    throw std::invalid_argument("SampleEnvironmentSpec::find() - Unable to "
                                "find Container matching ID '" +
                                id + "'");
}

/**
 * Build a new SampleEnvironment instance from a given can ID
 * @return A new instance of a SampleEnvironment
 */
SampleEnvironment_uptr
SampleEnvironmentSpec::buildEnvironment(const std::string &canID) const {
  auto env = std::make_unique<SampleEnvironment>(m_name, findContainer(canID));
  for (const auto &component : m_components) {
    env->add(component);
  }
  return env;
}

/**
 * Adds a can definition to the known list
 * @param can A pointer to a Container object
 * @throws std::invalid::argument if the id is empty
 */
void SampleEnvironmentSpec::addContainer(const Container_const_sptr &can) {
  if (can->id().empty()) {
    throw std::invalid_argument(
        "SampleEnvironmentSpec::addContainer() - Container must "
        "have an id field. Empty string found.");
  }
  m_cans.emplace(can->id(), can);
}

/**
 * Add a non-can component to the specification
 * @param component A pointer to a Object
 */
void SampleEnvironmentSpec::addComponent(const IObject_const_sptr &component) {
  m_components.emplace_back(component);
}

} // namespace Geometry
} // namespace Mantid
