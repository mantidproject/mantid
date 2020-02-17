// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAMPLEENVIRONMENTSPEC_H_
#define MANTID_DATAHANDLING_SAMPLEENVIRONMENTSPEC_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Mantid {
namespace DataHandling {

/**
  Defines the properties of a named SampleEnvironment setup. It is used
  in conjunction with the SampleEnvionmentBuilder to construct given
  configuration of SampleEnvironment. It can be read from XML by a
  SampleEnvironmentParser.
*/
class MANTID_DATAHANDLING_DLL SampleEnvironmentSpec {
public:
  // Convenience typedefs
  using ContainerIndex =
      std::unordered_map<std::string, Geometry::Container_const_sptr>;
  using ComponentList = std::vector<Geometry::IObject_const_sptr>;

  SampleEnvironmentSpec(std::string name);

  /// @return The name of the specification
  inline const std::string &name() const { return m_name; }
  /// @return The number of known cans
  inline size_t ncans() const { return m_cans.size(); }
  /// @return The number of non-can components
  inline size_t ncomponents() const { return m_components.size(); }
  Geometry::Container_const_sptr findContainer(const std::string &id) const;

  Geometry::SampleEnvironment_uptr
  buildEnvironment(const std::string &canID) const;

  void addContainer(const Geometry::Container_const_sptr &can);
  void addComponent(const Geometry::IObject_const_sptr &component);

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

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAMPLEENVIRONMENTSPEC_H_ */
