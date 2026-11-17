// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SampleEnvironmentSpec.h"

namespace Mantid::DataHandling {

/**
 * Constructor
 * @param name The string name of the specification
 */
SampleEnvironmentSpec::SampleEnvironmentSpec(std::string name) : m_name(std::move(name)), m_cans(), m_components() {}

/**
 * Find a can by id string
 * @param id The string id to search for
 * @return A pointer to the retrieved Container instance
 * @throws std::invalid_argument
 */
Geometry::Container_const_sptr SampleEnvironmentSpec::findContainer(const std::string &id) const {
  // if there's only one can and an id wasn't specified then return it
  if ((m_cans.size() == 1) && (id.empty())) {
    return m_cans.begin()->second;
  } else {

    auto indexIter = m_cans.find(id);
    if (indexIter != m_cans.end())
      return indexIter->second;
    else
      throw std::invalid_argument("SampleEnvironmentSpec::find() - Unable to "
                                  "find Container matching ID '" +
                                  id + "'");
  }
}

/**
 * Build a new SampleEnvironment instance from a given can ID
 * @return A new instance of a SampleEnvironment
 */
Geometry::SampleEnvironment_uptr SampleEnvironmentSpec::buildEnvironment(const std::string &canID) const {
  const auto can = findContainer(canID);
  auto env = std::make_unique<Geometry::SampleEnvironment>(m_name, can, m_canOnGoniometer.at(can->id()));
  for (size_t i = 0; i < m_components.size(); ++i) {
    env->add(m_components[i], m_componentOnGoniometer[i]);
  }
  return env;
}

/**
 * Adds a can definition to the known list
 * @param can A pointer to a Container object
 * @param onGoniometer Whether it turns with the sample when a goniometer is set
 * @throws std::invalid::argument if the id is empty
 */
void SampleEnvironmentSpec::addContainer(const Geometry::Container_const_sptr &can, const bool onGoniometer) {
  if (can->id().empty()) {
    throw std::invalid_argument("SampleEnvironmentSpec::addContainer() - Container must "
                                "have an id field. Empty string found.");
  }
  m_cans.emplace(can->id(), can);
  m_canOnGoniometer.emplace(can->id(), onGoniometer);
}

/**
 * Add a non-can component to the specification
 * @param component A pointer to a Object
 * @param onGoniometer Whether it turns with the sample when a goniometer is set
 */
void SampleEnvironmentSpec::addComponent(const Geometry::IObject_const_sptr &component, const bool onGoniometer) {
  m_components.emplace_back(component);
  m_componentOnGoniometer.emplace_back(onGoniometer);
}

} // namespace Mantid::DataHandling
