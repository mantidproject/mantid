// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*
 *
 * SetCrystalLocation.cpp
 *
 *  Created on: Dec 12, 2018
 *      Author: Brendan Sullivan
 */
#include "MantidCrystal/SetCrystalLocation.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"

#include <cstdarg>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Geometry::IndexingUtils;
using Mantid::Geometry::Instrument_const_sptr;
using namespace Mantid::Geometry;

namespace Mantid {

namespace Crystal {

DECLARE_ALGORITHM(SetCrystalLocation)

void SetCrystalLocation::init() {
  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Original event workspace");
  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output event workspace with a modified sample position");
  declareProperty("NewX", 0.0, "New Absolute X position of crystal.");
  declareProperty("NewY", 0.0, "New Absolute Y position of crystal.");
  declareProperty("NewZ", 0.0, "New Absolute Z position of crystal.");
}

// simple algorithm that changes the sample position of the input
// event workspace.
void SetCrystalLocation::exec() {
  EventWorkspace_sptr events = getProperty("InputWorkspace");
  EventWorkspace_sptr outEvents = getProperty("OutputWorkspace");
  const double newX = getProperty("NewX");
  const double newY = getProperty("NewY");
  const double newZ = getProperty("NewZ");
  V3D newSamplePos = V3D(newX, newY, newZ);
  if (events != outEvents) {
    outEvents = events->clone();
  }

  auto &componentInfo = outEvents->mutableComponentInfo();
  CalibrationHelpers::adjustUpSampleAndSourcePositions(
      componentInfo.l1(), newSamplePos, componentInfo);

  setProperty("OutputWorkspace", outEvents);
} // exec

} // namespace Crystal

} // namespace Mantid
