// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VATES_KNOWLEDGE_SERIALIZER_H
#define VATES_KNOWLEDGE_SERIALIZER_H

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <string>

namespace Mantid {
/// Forward Declarations;
namespace Geometry {
class MDImplicitFunction;
}
namespace API {
class IMDWorkspace;
}

namespace VATES {

// The workspace location may or may not be required. This type defines the
// options.
enum LocationPolicy { LocationMandatory, LocationNotRequired };

/**

 This type assists with the generation of well-formed xml meeting the xsd
 scehema. The individual components utilised here may not be able to form
 well-formed
 xml in their own right and therefore do not have a toXMLString method.

 This implementation is based on a builder pattern using the create mechanism
 for xml string generation.

 @author Owen Arnold, Tessella plc
 @date 14/12/2010*/

class DLLExport VatesKnowledgeSerializer {

private:
  boost::shared_ptr<const Mantid::Geometry::MDImplicitFunction> m_spFunction;
  std::string m_wsLocationXML;
  std::string m_wsNameXML;
  std::string m_wsName;
  std::string m_geomXML;

public:
  VatesKnowledgeSerializer();

  /// Set the implicit function to use called.
  void setImplicitFunction(
      boost::shared_ptr<const Mantid::Geometry::MDImplicitFunction> spFunction);

  /// Set the workspace name to apply.
  void setWorkspace(const Mantid::API::IMDWorkspace &workspace);

  /// Set the workspace name to apply.
  void setWorkspaceName(const std::string &wsName);

  /// Set the geometry xml to apply.
  void setGeometryXML(const std::string &geomXML);

  /// Create the xml string correponding to the set values.
  std::string createXMLString() const;

  /// Get the underlying workspace name.
  const std::string &getWorkspaceName() const;

  /// Get the underlying workspace geometry.
  const std::string &getWorkspaceGeometry() const;

  /// Determine if function information is available/set.
  bool hasFunctionInfo() const;

  /// Determine if gemetry information is available/set.
  bool hasGeometryInfo() const;
};
} // namespace VATES
} // namespace Mantid
#endif
