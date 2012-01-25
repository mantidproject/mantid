//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/PairedGroupAlgorithm.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace API
  {
    PairedGroupAlgorithm::PairedGroupAlgorithm() : API::Algorithm(), m_progress(NULL) {}

    PairedGroupAlgorithm::~PairedGroupAlgorithm()
    {
      if (m_progress) delete m_progress;
    }



  } // namespace API
} // namespace Mantid
