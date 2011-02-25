#include "MantidDataHandling/LoadDaveGrp.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FileProperty.h"
#include <vector>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadDaveGrp)

void LoadDaveGrp::init()
{
  std::vector<std::string> exts;
  exts.push_back(".grp");

  this->declareProperty(new API::FileProperty("Filename", "",
      API::FileProperty::Load, exts), "A Dave grouped Ascii file");
  this->declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "",
      Kernel::Direction::Output),
      "The name of the workspace that will be created.");
}

void LoadDaveGrp::exec()
{
  const int nGroups = 1;
  const int xLength = 1;
  const int yLength = 1;

  // Create workspace
  API::MatrixWorkspace_sptr outputWorkspace = \
      boost::dynamic_pointer_cast<API::MatrixWorkspace>\
      (API::WorkspaceFactory::Instance().create("Workspace2D", nGroups,
          xLength, yLength));

  this->setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace DataHandling
} // namespace Mantid
