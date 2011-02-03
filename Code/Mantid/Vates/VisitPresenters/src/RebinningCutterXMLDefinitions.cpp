#include <MantidVisitPresenters/RebinningCutterXMLDefinitions.h>

namespace Mantid
{
namespace VATES
{

const std::string XMLDefinitions::workspaceNameXMLTagStart = "<MDWorkspaceName>";
const std::string XMLDefinitions::workspaceNameXMLTagEnd = "</MDWorkspaceName>";
const std::string XMLDefinitions::workspaceLocationXMLTagStart = "<MDWorkspaceLocation>";
const std::string XMLDefinitions::workspaceLocationXMLTagEnd = "</MDWorkspaceLocation>";
const std::string XMLDefinitions::workspaceInstructionXMLTagStart = "<MDInstruction>";
const std::string XMLDefinitions::workspaceInstructionXMLTagEnd = "</MDInstruction>";

const std::string XMLDefinitions::workspaceNameElementName = "MDWorkspaceName";
const std::string XMLDefinitions::functionElementName = "Function";
const std::string XMLDefinitions::workspaceLocationElementName = "MDWorkspaceLocation";
const std::string XMLDefinitions::workspaceGeometryElementName = "DimensionSet";

//Not strictly an xml definition. Consider moving. The actual values assigned to the following are not important other that that they are consistent.
const std::string XMLDefinitions::metaDataId="1";
const std::string XMLDefinitions::signalName="signal";
const std::string XMLDefinitions::geometryNodeName="geometryNodeName";
const std::string XMLDefinitions::functionNodeName="functionNodeName";
const std::string XMLDefinitions::geometryOperatorInfo="geometryOperatorInfo";
const std::string XMLDefinitions::functionOperatorInfo="functionOperatorInfo";

}
}
