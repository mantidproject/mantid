/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This collection of functions MAY ONLY be used in packages above MDAlgorithms
 *********************************************************************************/
#include "MantidTestHelpers/MDAlgorithmsTestHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

namespace Mantid {
using namespace API;
using namespace DataObjects;

namespace MDAlgorithms {

/** Set of helper methods for testing MDEventWorkspace things
 *
 * */
namespace MDAlgorithmsTestHelper {

/** Make a (optionally) file backed MDEventWorkspace with nEvents fake data
 *points
 * the points are randomly distributed within the box (nEvents>0) or
 *homoheneously and regularly spread through the box (nEvents<0)
 *
 * @param wsName :: name of the workspace in ADS
 * @param fileBacked :: true for file-backed
 * @param numEvents :: number of events in the target workspace distributed
 *randomly if numEvents>0 or regularly & homogeneously if numEvents<0
 * @param coord :: Required coordinate system
 * @return MDEW sptr
 */
DataObjects::MDEventWorkspace3Lean::sptr
makeFileBackedMDEW(const std::string &wsName, bool fileBacked, long numEvents,
                   Kernel::SpecialCoordinateSystem coord) {
  // ---------- Make a file-backed MDEventWorkspace -----------------------
  std::string snEvents = boost::lexical_cast<std::string>(numEvents);
  MDEventWorkspace3Lean::sptr ws1 =
      MDEventsTestHelper::makeMDEW<3>(10, 0.0, 10.0, 0);
  ws1->getBoxController()->setSplitThreshold(100);
  ws1->setCoordinateSystem(coord);
  Mantid::API::AnalysisDataService::Instance().addOrReplace(
      wsName, boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(ws1));
  FrameworkManager::Instance().exec("FakeMDEventData", 6, "InputWorkspace",
                                    wsName.c_str(), "UniformParams",
                                    snEvents.c_str(), "RandomizeSignal", "1");
  if (fileBacked) {
    std::string filename = wsName + ".nxs";
    auto saver = FrameworkManager::Instance().exec(
        "SaveMD", 4, "InputWorkspace", wsName.c_str(), "Filename",
        filename.c_str());
    FrameworkManager::Instance().exec(
        "LoadMD", 8, "OutputWorkspace", wsName.c_str(), "Filename",
        saver->getPropertyValue("Filename").c_str(), "FileBackEnd", "1",
        "Memory", "0");
  }
  return boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(
      Mantid::API::AnalysisDataService::Instance().retrieve(wsName));
}

} // namespace
}
}
