// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTSPEC_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTSPEC_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Mantid {
namespace Geometry {

/**
  Defines the properties of a named SampleEnvironment setup. It is used
  in conjunction with the SampleEnvionmentBuilder to construct given
  configuration of SampleEnvironment. It can be read from XML by a
  SampleEnvironmentParser.
*/
class MANTID_GEOMETRY_DLL SampleEnvironmentSpec {
public:
  // Convenience typedefs
  using ContainerIndex = std::unordered_map<std::string, Container_const_sptr>;
  using ComponentList = std::vector<IObject_const_sptr>;

  SampleEnvironmentSpec(std::string name);

  /// @return The name of the specification
  inline const std::string &name() const { return m_name; }
  /// @return The number of known cans
  inline size_t ncans() const { return m_cans.size(); }
  /// @return The number of non-can components
  inline size_t ncomponents() const { return m_components.size(); }
  Container_const_sptr findContainer(const std::string &id) const;

  SampleEnvironment_uptr buildEnvironment(const std::string &canID) const;

  void addContainer(const Container_const_sptr &can);
  void addComponent(const IObject_const_sptr &component);

private:
  std::string m_name;
  ContainerIndex m_cans;
  ComponentList m_components;
};

/// unique_ptr to a SampleEnvironmentSpec
using SampleEnvironmentSpec_uptr = std::unique_ptr<SampleEnvironmentSpec>;
/// unique_ptr to a const SampleEnvironmentSpec
using SampleEnvironmentSpec_const_uptr =
    std::unique_ptr<const SampleEnvironmentSpec>;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SAMPLEENVIRONMENTSPEC_H_ */
