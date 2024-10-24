// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/SampleEnvironmentSpec.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/Material.h"
#include <iosfwd>
#include <unordered_map>

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace DataHandling {

/**
  Read an XML definition of a SampleEnvironmentSpec and produce a new
  SampleEnvironmentSpec object.
*/
class MANTID_DATAHANDLING_DLL SampleEnvironmentSpecParser {
public:
  static constexpr const char *ROOT_TAG = "environmentspec";

public:
  SampleEnvironmentSpec_uptr parse(const std::string &name, const std::string &filename, std::istream &istr);
  SampleEnvironmentSpec_uptr parse(const std::string &name, Poco::XML::Element *element);

private:
  // Convenience definitions
  using MaterialsIndex = std::unordered_map<std::string, Kernel::Material>;

  // Methods
  void validateRootElement(Poco::XML::Element *element) const;
  void parseMaterials(Poco::XML::Element *element);
  void parseAndAddComponents(SampleEnvironmentSpec *spec, Poco::XML::Element *element) const;
  void loadFullSpecification(SampleEnvironmentSpec *spec, Poco::XML::Element *element);
  void parseAndAddContainers(SampleEnvironmentSpec *spec, Poco::XML::Element *element) const;
  Geometry::Container_const_sptr parseContainer(Poco::XML::Element *element) const;
  std::shared_ptr<Geometry::IObject> parseComponent(Poco::XML::Element *element) const;
  std::shared_ptr<Geometry::MeshObject> loadMeshFromSTL(Poco::XML::Element *stlFileElement) const;
  void LoadOptionalDoubleFromXML(Poco::XML::Element *componentElement, const std::string &elementName,
                                 double &targetVariable) const;
  std::vector<double> parseTranslationVector(const std::string &translationVectorStr) const;
  std::string findFile(const std::string &filename) const;
  // Members
  MaterialsIndex m_materials;
  std::string m_filepath;
};

} // namespace DataHandling
} // namespace Mantid
