#include "MantidMDAlgorithms/ConvertCWSDMDtoHKL.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

DECLARE_ALGORITHM(ConvertCWSDMDtoHKL)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertCWSDMDtoHKL::ConvertCWSDMDtoHKL() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertCWSDMDtoHKL::~ConvertCWSDMDtoHKL() {}

void ConvertCWSDMDtoHKL::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "Name of the input MDEventWorkspace that stores detectors "
                  "counts from a constant-wave powder diffraction experiment.");

  Mantid::Kernel::IPropertyManager::declareProperty(
      new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace", "",
                                               Direction::Output),
      "Name of the output MDEventWorkspace in HKL-space.");
}

/**
  */
void ConvertCWSDMDtoHKL::exec() {
  // 1. Check the units of the MDEvents
  // 2. Export all the events to text file
  // 3. Get a UB matrix
  // 4. Refer to IndexPeak to calculate H,K,L of each MDEvent
}

} // namespace MDAlgorithms
} // namespace Mantid
