// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECPARSER_H_
#define MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECPARSER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/SampleEnvironmentSpec.h"
#include "MantidKernel/Material.h"
#include <iosfwd>
#include <unordered_map>

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace Geometry {

/**
  Read an XML definition of a SampleEnvironmentSpec and produce a new
  SampleEnvironmentSpec object.
*/
class MANTID_GEOMETRY_DLL SampleEnvironmentSpecParser {
public:
  static constexpr const char *ROOT_TAG = "environmentspec";

public:
  SampleEnvironmentSpec_uptr parse(const std::string &name, std::istream &istr);
  SampleEnvironmentSpec_uptr parse(const std::string &name,
                                   Poco::XML::Element *element);

private:
  // Convenience definitions
  using MaterialsIndex = std::unordered_map<std::string, Kernel::Material>;

  // Methods
  void validateRootElement(Poco::XML::Element *element) const;
  void parseMaterials(Poco::XML::Element *element);
  void parseAndAddComponents(SampleEnvironmentSpec *spec,
                             Poco::XML::Element *element) const;
  void parseAndAddContainers(SampleEnvironmentSpec *spec,
                             Poco::XML::Element *element) const;
  Container_const_sptr parseContainer(Poco::XML::Element *element) const;
  boost::shared_ptr<IObject> parseComponent(Poco::XML::Element *element) const;

  // Members
  MaterialsIndex m_materials;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SAMPLEENVIRONMENTSPECPARSER_H_ */
