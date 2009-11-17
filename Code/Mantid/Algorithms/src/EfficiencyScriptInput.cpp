#include "MantidAlgorithms/EfficiencyScriptInput.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/RebinParamsValidator.h"

namespace Mantid
{
namespace Algorithms
{
// Register the class into the algorithm factory
DECLARE_ALGORITHM(EfficiencyScriptInput)

using namespace Kernel;
using namespace API;
using namespace Geometry;


void EfficiencyScriptInput::init()
{
  std::vector<std::string> raw;
  raw.push_back("raw");
  declareProperty(new FileProperty("RawFile","", FileProperty::Load, raw), "");
  declareProperty(new ArrayProperty<double>("BinBoundaries", new RebinParamsValidator));
  std::vector<std::string> map;
  raw.push_back("map");
  declareProperty(new FileProperty("MapFile","", FileProperty::Load, map), "");
  declareProperty(new FileProperty("OutFile","", FileProperty::Save), "");
}

void EfficiencyScriptInput::exec()
{
}

} // namespace Algorithm
} // namespace Mantid
