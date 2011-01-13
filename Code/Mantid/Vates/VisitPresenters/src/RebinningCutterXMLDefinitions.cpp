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

//Not strictly an xml definition. Consider moving.
const std::string XMLDefinitions::metaDataId="1";
}
}
