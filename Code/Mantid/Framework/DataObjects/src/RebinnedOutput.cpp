#include "MantidDataObjects/RebinnedOutput.h"

#include "MantidAPI/WorkspaceFactory.h"

//using namespace Mantid::Kernel;
//using namespace Mantid::API;

namespace Mantid
{
namespace DataObjects
{
  DECLARE_WORKSPACE(RebinnedOutput)

  // Get a reference to the logger
  Kernel::Logger& RebinnedOutput::g_log = Kernel::Logger::get("RebinnedOutput");

  RebinnedOutput::RebinnedOutput() : Workspace2D()
  {
  }
    
  RebinnedOutput::~RebinnedOutput()
  {
    // Clear out the memory
    for (std::size_t i = 0; i < this->fracArea.size(); i++)
    {
      delete this->fracArea[i];
    }
  }
  
  /**
   * Gets the name of the workspace type.
   * @return Standard string name
   */
  const std::string RebinnedOutput::id() const
  {
    return "RebinnedOutput";
  }

  /**
   * Sets the size of the workspace and initializes arrays to zero
   * @param NVectors :: The number of vectors/histograms/detectors in the workspace
   * @param XLength :: The number of X data points/bin boundaries in each vector (must all be the same)
   * @param YLength :: The number of data/error points in each vector (must all be the same)
   */
  void RebinnedOutput::init(const std::size_t &NVectors,
                            const std::size_t &XLength,
                            const std::size_t &YLength)
  {
    Workspace2D::init(NVectors, XLength, YLength);
    std::size_t nHist = this->getNumberHistograms();
    this->fracArea.resize(nHist);
  }


} // namespace Mantid
} // namespace DataObjects
