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
  std::vector<std::string> raw(1, std::string("raw"));
  declareProperty(new FileProperty("RawFile","", FileProperty::Load, raw),"");
  declareProperty(new ArrayProperty<double>("BinBoundaries", new RebinParamsValidator));
  declareProperty(new FileProperty("WhiteBeamVan", "", FileProperty::Load, raw), "");
  std::vector<std::string> mask(1, "msk");
  declareProperty(new FileProperty("DetectorMask","", FileProperty::NoExistLoad, mask), "");
  std::vector<std::string> map(1, "map");
  declareProperty(new FileProperty("MapFile","", FileProperty::NoExistLoad, map), "");
  declareProperty(new FileProperty("OutFile","", FileProperty::Save), "");
}

void EfficiencyScriptInput::exec()
{
}

} // namespace Algorithm
} // namespace Mantid
