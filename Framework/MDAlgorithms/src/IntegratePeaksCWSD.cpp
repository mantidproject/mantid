#include "MantidMDAlgorithms/IntegratePeaksCWSD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Utils.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/Progress.h"
#include <fstream>

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksCWSD)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IntegratePeaksCWSD::IntegratePeaksCWSD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IntegratePeaksCWSD::~IntegratePeaksCWSD() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegratePeaksCWSD::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input MDEventWorkspace.");

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace", "",
                                                        Direction::Input),
                  "A PeaksWorkspace containing the peaks to integrate.");

  declareProperty(
      new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                            Direction::Output),
      "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
      "with the peaks' integrated intensities.");

  declareProperty(new WorkspaceProperty<DataObjects::MaskWorkspace>(
                      "MaskWorkspace", "", Direction::Output, PropertyMode::Optional),
                  "Output Masking Workspace");
}

/**
 * @brief IntegratePeaksCWSD::exec
 */
void IntegratePeaksCWSD::exec()
{
  // Process input
  m_inputWS = getProperty("InputWorkspace");
  std::string maskwsname = getPropertyValue("MaskWorkspace");
  if (maskwsname.size() > 0)
  {
    m_maskDets = true;
    m_maskWS = getProperty("MaskWorkspace");
  }
  else
  {
    m_maskDets = false;
  }


} //

void IntegratePeaksCWSD::simplePeakIntegration()
{
  // Go through to get value
  IMDIterator *mditer = m_inputWS->createIterator();
  size_t nextindex = 1;
  bool scancell = true;
  size_t currindex = 0;
  while (scancell) {
    size_t numevent_cell = mditer->getNumEvents();
    for (size_t iev = 0; iev < numevent_cell; ++iev) {
      // Check
      if (currindex >= vec_event_qsample.size())
        throw std::runtime_error("Logic error in event size!");

      float tempx = mditer->getInnerPosition(iev, 0);
      float tempy = mditer->getInnerPosition(iev, 1);
      float tempz = mditer->getInnerPosition(iev, 2);
      signal_t signal = mditer->getInnerSignal(iev);
      detid_t detid = mditer->getInnerDetectorID(iev);

      // FIXME/TODO/NOW - Continue from here!
      throw std::runtime_error("Need to find out how to deal with detid and signal here!");

      Kernel::V3D qsample(tempx, tempy, tempz);
      vec_event_qsample[currindex] = qsample;
      vec_event_signal[currindex] = signal;
      vec_event_det[currindex] = detid;

      ++currindex;
    }

    // Advance to next cell
    if (mditer->next()) {
      // advance to next cell
      mditer->jumpTo(nextindex);
      ++nextindex;
    } else {
      // break the loop
      scancell = false;
    }
  }


}

} // namespace Mantid
} // namespace MDAlgorithms
