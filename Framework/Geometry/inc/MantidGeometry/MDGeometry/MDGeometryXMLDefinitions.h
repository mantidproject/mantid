// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include <string>
namespace Mantid {
namespace Geometry {

/**

 This type contains definitions that will be found in the xml schema for the
 rebinning instructions, but must be used in
 code as part of the peristance/fetching routines. This file provides a single
 location for definitions to aid future maintenance.

 @author Owen Arnold, Tessella plc
 @date 16/12/2010*/

class MANTID_GEOMETRY_DLL
MDGeometryXMLDefinitions{// clang-format off
public :
// XML schema tag definitions for generating xml.
static const std::string workspaceNameXMLTagStart(){ return "<MDWorkspaceName>"; }
static const std::string workspaceNameXMLTagEnd() { return "</MDWorkspaceName>"; }
static const std::string workspaceLocationXMLTagStart() { return "<MDWorkspaceLocation>"; }
static const std::string workspaceLocationXMLTagEnd() { return "</MDWorkspaceLocation>"; }
static const std::string workspaceInstructionXMLTagStart() { return "<MDInstruction>"; }
static const std::string workspaceInstructionXMLTagEnd() { return "</MDInstruction>"; }
/// XML schema tag definitions for finding xml.
static const std::string workspaceNameElementName() { return "MDWorkspaceName"; }
static const std::string functionElementName() { return "Function"; }
static const std::string workspaceLocationElementName() { return "MDWorkspaceLocation"; }
static const std::string workspaceGeometryElementName() { return "DimensionSet"; }
static const std::string workspaceDimensionElementName() { return "Dimension"; }
static const std::string workspaceXDimensionElementName() { return "XDimension"; }
static const std::string workspaceYDimensionElementName() { return "YDimension"; }
static const std::string workspaceZDimensionElementName() { return "ZDimension"; }
static const std::string workspaceTDimensionElementName() { return "TDimension"; }
static const std::string workspaceRefDimensionElementName() { return "RefDimensionId"; }
static const std::string signalName() { return "signal"; }
static const std::string geometryNodeName() { return "geometryNodeName"; }
static const std::string functionNodeName() { return "functionNodeName"; }
static const std::string geometryOperatorInfo() { return "geometryOperatorInfo"; }
static const std::string functionOperatorInfo() { return "functionOperatorInfo"; }
static const std::string RebinnedWSName() { return "RebinnedWS"; }
};
// clang-format on

} // namespace Geometry
} // namespace Mantid
