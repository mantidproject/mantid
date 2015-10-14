//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/FindDetectorsInShape.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FindDetectorsInShape)

using namespace Kernel;
using namespace API;
using namespace Geometry;

/// (Empty) Constructor
FindDetectorsInShape::FindDetectorsInShape() {}

/// Destructor
FindDetectorsInShape::~FindDetectorsInShape() {}

void FindDetectorsInShape::init() {
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("Workspace", "", Direction::Input),
      "Name of the input workspace");
  declareProperty("ShapeXML", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The XML definition of the shape");
  declareProperty(
      "IncludeMonitors", false,
      "Whether monitors should be included if they are contained in the\n"
      "shape (default false)");
  declareProperty("DetectorList", std::vector<int>(),
                  "The list of detector ids included within the shape",
                  Direction::Output);
}

void FindDetectorsInShape::exec() {
  // Get the input workspace
  const MatrixWorkspace_const_sptr WS = getProperty("Workspace");

  bool includeMonitors = getProperty("IncludeMonitors");

  std::string shapeXML = getProperty("ShapeXML");

  // convert into a Geometry object
  Geometry::ShapeFactory sFactory;
  boost::shared_ptr<Geometry::Object> shape_sptr =
      sFactory.createShape(shapeXML);

  // get the instrument out of the workspace
  Instrument_const_sptr instrument_sptr = WS->getInstrument();

  // To get all the detector ID's
  detid2det_map allDetectors;
  instrument_sptr->getDetectors(allDetectors);

  std::vector<int> foundDets;

  // progress
  detid2det_map::size_type objCmptCount = allDetectors.size();
  int iprogress_step = static_cast<int>(objCmptCount / 100);
  if (iprogress_step == 0)
    iprogress_step = 1;
  int iprogress = 0;

  // Now go through all
  detid2det_map::iterator it;
  detid2det_map::const_iterator it_end = allDetectors.end();
  for (it = allDetectors.begin(); it != it_end; ++it) {
    Geometry::IDetector_const_sptr det = it->second;

    // attempt to dynamic cast up to an IDetector
    boost::shared_ptr<const Geometry::IDetector> detector_sptr =
        boost::dynamic_pointer_cast<const Geometry::IDetector>(it->second);

    if (detector_sptr) {
      if ((includeMonitors) || (!detector_sptr->isMonitor())) {
        // check if the centre of this item is within the user defined shape
        if (shape_sptr->isValid(detector_sptr->getPos())) {
          // shape encloses this objectComponent
          g_log.debug() << "Detector contained in shape "
                        << detector_sptr->getID() << std::endl;
          foundDets.push_back(detector_sptr->getID());
        }
      }
    }

    iprogress++;
    if (iprogress % iprogress_step == 0) {
      progress(static_cast<double>(iprogress) /
               static_cast<double>(objCmptCount));
      interruption_point();
    }
  }
  setProperty("DetectorList", foundDets);
}

} // namespace DataHandling
} // namespace Mantid
