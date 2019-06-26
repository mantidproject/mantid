// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/FindDetectorsInShape.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FindDetectorsInShape)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void FindDetectorsInShape::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "Workspace", "", Direction::Input),
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
  const MatrixWorkspace_const_sptr WS = getProperty("Workspace");
  bool includeMonitors = getProperty("IncludeMonitors");
  std::string shapeXML = getProperty("ShapeXML");

  // convert into a Geometry object
  Geometry::ShapeFactory sFactory;
  auto shape_sptr = sFactory.createShape(shapeXML);

  const auto &detectorInfo = WS->detectorInfo();
  const auto &detIDs = detectorInfo.detectorIDs();

  std::vector<int> foundDets;

  // progress
  detid2det_map::size_type objCmptCount = detectorInfo.size();
  int iprogress_step = static_cast<int>(objCmptCount / 100);
  if (iprogress_step == 0)
    iprogress_step = 1;
  int iprogress = 0;

  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if ((includeMonitors) || (!detectorInfo.isMonitor(i))) {
      // check if the centre of this item is within the user defined shape
      if (shape_sptr->isValid(detectorInfo.position(i))) {
        // shape encloses this objectComponent
        g_log.debug() << "Detector contained in shape " << detIDs[i] << '\n';
        foundDets.push_back(detIDs[i]);
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
