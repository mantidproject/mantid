// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VATESXMLDEFINITIONS_H_
#define VATESXMLDEFINITIONS_H_

#include "MantidKernel/System.h"
#include <string>
namespace Mantid {
namespace VATES {

/**

 This type contains definitions that will be found in the xml schema of VATES,
 but must be used in
 code as part of the peristance/fetching routines. This file provides a single
 location for definitions to aid future maintenance.

 @author Owen Arnold, Tessella plc
 @date 16/12/2010*/

// TODO: these definitions may be more appropriate in API/Geometry where they
// can also be used as part of the parsing.

class DLLExport XMLDefinitions {
public:
  ////XML schema tag definitions for generating xml.
  // static const std::string workspaceNameXMLTagStart()
  // {
  //   return "<MDWorkspaceName>";
  // }
  // static const std::string workspaceNameXMLTagEnd()
  // {
  //   return "</MDWorkspaceName>";
  // }
  // static const std::string workspaceLocationXMLTagStart()
  // {
  //   return "<MDWorkspaceLocation>";
  // }
  // static const std::string workspaceLocationXMLTagEnd()
  // {
  //   return "</MDWorkspaceLocation>";
  // }
  // static const std::string workspaceInstructionXMLTagStart()
  // {
  //   return "<MDInstruction>";
  // }
  // static const std::string workspaceInstructionXMLTagEnd()
  // {
  //   return "</MDInstruction>";
  // }
  // ///XML schema tag definitions for finding xml.
  // static const std::string workspaceNameElementName()
  // {
  //   return "MDWorkspaceName";
  // }
  // static const std::string functionElementName()
  // {
  //   return "Function";
  // }
  // static const std::string workspaceLocationElementName()
  // {
  //   return "MDWorkspaceLocation";
  // }
  // static const std::string workspaceGeometryElementName()
  // {
  //   return "DimensionSet";
  // }
  // static const std::string workspaceDimensionElementName()
  // {
  //   return "Dimension";
  // }
  // static const std::string workspaceXDimensionElementName()
  // {
  //   return "XDimension";
  // }
  // static const std::string workspaceYDimensionElementName()
  // {
  //   return "YDimension";
  // }
  // static const std::string workspaceZDimensionElementName()
  // {
  //   return "ZDimension";
  // }
  // static const std::string workspaceTDimensionElementName()
  // {
  //   return "TDimension";
  // }
  // static const std::string workspaceRefDimensionElementName()
  // {
  //   return "RefDimensionId";
  // }
  /// An id for recognising specific vtkFieldData objects on inbound and
  /// outbound datasets.
  static const std::string metaDataId() { return "VATES_Metadata"; }
  static const std::string signalName() { return "signal"; }
  static const std::string geometryNodeName() { return "geometryNodeName"; }
  static const std::string functionNodeName() { return "functionNodeName"; }
  static const std::string geometryOperatorInfo() {
    return "geometryOperatorInfo";
  }
  static const std::string functionOperatorInfo() {
    return "functionOperatorInfo";
  }
  static const std::string RebinnedWSName() { return "RebinnedWS"; }
};
} // namespace VATES
} // namespace Mantid
#endif
