// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveCalFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include <cmath>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Types::Core::DateAndTime;

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveCalFile)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveCalFile::init() {
  declareProperty(std::make_unique<WorkspaceProperty<GroupingWorkspace>>("GroupingWorkspace", "", Direction::Input,
                                                                         PropertyMode::Optional),
                  "Optional: An GroupingWorkspace workspace giving the grouping info.");

  declareProperty(std::make_unique<WorkspaceProperty<OffsetsWorkspace>>("OffsetsWorkspace", "", Direction::Input,
                                                                        PropertyMode::Optional),
                  "Optional: An OffsetsWorkspace workspace giving the detector calibration "
                  "values.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MaskWorkspace>>("MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional: An Workspace workspace giving which detectors are masked.");

  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save, ".cal"),
                  "Path to the .cal file that will be created.");

  auto offsetprecision = std::make_shared<BoundedValidator<int>>();
  offsetprecision->setLower(7);
  offsetprecision->setUpper(11);
  declareProperty("OffsetPrecision", 7, offsetprecision, "Precision of offsets (between 7 and 11 decimal).");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveCalFile::exec() {
  GroupingWorkspace_sptr groupWS = getProperty("GroupingWorkspace");
  OffsetsWorkspace_sptr offsetsWS = getProperty("OffsetsWorkspace");
  MaskWorkspace_sptr maskWS = getProperty("MaskWorkspace");
  std::string Filename = getPropertyValue("Filename");
  m_precision = getProperty("OffsetPrecision");

  // Do the saving
  SaveCalFile::saveCalFile(Filename, groupWS, offsetsWS, maskWS);
}

//-----------------------------------------------------------------------
/** Reads the calibration file.
 *
 * @param calFileName :: path to the old .cal file
 * @param groupWS :: optional, GroupingWorkspace to save. Will be 0 if not
 *specified.
 * @param offsetsWS :: optional, OffsetsWorkspace to save. Will be 0.0 if not
 *specified.
 * @param maskWS :: optional, masking-type workspace to save. Will be 1
 *(selected) if not specified.
 */
void SaveCalFile::saveCalFile(const std::string &calFileName, const GroupingWorkspace_sptr &groupWS,
                              const OffsetsWorkspace_sptr &offsetsWS, const MaskWorkspace_sptr &maskWS) {
  Instrument_const_sptr inst;

  bool doGroup = false;
  if (groupWS) {
    doGroup = true;
    inst = groupWS->getInstrument();
  }

  bool doOffsets = false;
  if (offsetsWS) {
    doOffsets = true;
    inst = offsetsWS->getInstrument();
  }

  bool doMask = false;
  if (maskWS) {
    doMask = true;
    inst = maskWS->getInstrument();
    if (!inst)
      g_log.warning() << "Mask workspace " << maskWS->getName() << " has no instrument associated with."
                      << "\n";
  }

  g_log.information() << "Status: doGroup = " << doGroup << " doOffsets = " << doOffsets << " doMask = " << doMask
                      << "\n";

  if (!inst)
    throw std::invalid_argument("You must give at least one of the grouping, "
                                "offsets or masking workspaces.");

  // Header of the file
  std::ofstream fout(calFileName.c_str());
  fout << "# Calibration file for instrument " << inst->getName() << " written on "
       << DateAndTime::getCurrentTime().toISO8601String() << ".\n";
  fout << "# Format: number    UDET         offset    select    group\n";

  // Get all the detectors
  detid2det_map allDetectors;
  inst->getDetectors(allDetectors);
  int64_t number = 0;

  detid2det_map::const_iterator it;
  for (it = allDetectors.begin(); it != allDetectors.end(); ++it) {
    detid_t detectorID = it->first;
    // Geometry::IDetector_const_sptr det = it->second;

    // Find the offset, if any
    double offset = 0.0;
    if (doOffsets)
      offset = offsetsWS->getValue(detectorID, 0.0);

    // Find the group, if any
    int64_t group = 1;
    if (doGroup)
      group = static_cast<int64_t>(groupWS->getValue(detectorID, 0.0));

    // Find the selection, if any
    int selected = 1;
    if (doMask && (maskWS->isMasked(detectorID)))
      selected = 0;

    // if(group > 0)
    fout << std::fixed << std::setw(9) << number << std::fixed << std::setw(15) << detectorID << std::fixed
         << std::setprecision(m_precision) << std::setw(15) << offset << std::fixed << std::setw(8) << selected
         << std::fixed << std::setw(8) << group << "\n";

    number++;
  }
}

} // namespace Mantid::DataHandling
