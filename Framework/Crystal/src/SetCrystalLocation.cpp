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
 *      Author: Brendan
 */
#include "MantidCrystal/SetCrystalLocation.h"

#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/CalibrationHelpers.h"
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

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

class OrEnabledWhenProperties : public Kernel::IPropertySettings {
public:
  OrEnabledWhenProperties(const std::string &prop1Name,
                          ePropertyCriterion prop1Crit,
                          const std::string &prop1Value,
                          const std::string &prop2Name,
                          ePropertyCriterion prop2Crit,
                          const std::string &prop2Value)
      : IPropertySettings(), propName1(prop1Name), propName2(prop2Name),
        Criteria1(prop1Crit), Criteria2(prop2Crit), value1(prop1Value),
        value2(prop2Value)

  {
    Prop1 = new Kernel::EnabledWhenProperty(propName1, Criteria1, value1);
    Prop2 = new Kernel::EnabledWhenProperty(propName2, Criteria2, value2);
  }
  ~OrEnabledWhenProperties() override // responsible for deleting all supplied
                                      // EnabledWhenProperites
  {
    delete Prop1;
    delete Prop2;
  }

  IPropertySettings *clone() const override {
    return new OrEnabledWhenProperties(propName1, Criteria1, value1, propName2,
                                       Criteria2, value2);
  }

  bool isEnabled(const IPropertyManager *algo) const override {
    return Prop1->isEnabled(algo) && Prop2->isEnabled(algo);
  }

private:
  std::string propName1, propName2;
  ePropertyCriterion Criteria1, Criteria2;
  std::string value1, value2;
  Kernel::EnabledWhenProperty *Prop1, *Prop2;
};

void SetCrystalLocation::init() {
  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "EventWorkspace", "", Direction::Input),
                  "Original event workspace");
  declareProperty(
      make_unique<WorkspaceProperty<EventWorkspace>>("ModifiedEventWorkspace",
                                                     "", Direction::Output),
      "Output event workspace with a modified sample position");
  declareProperty("NewX", 0.0, "New Absolute X position of crystal.");
  declareProperty("NewY", 0.0, "New Absolute Y position of crystal.");
  declareProperty("NewZ", 0.0, "New Absolute Z position of crystal.");
}

/**
 * Execute algorithm. Steps:
 * a) Get property values
 * b) Set up data for call to PeakHKLErrors fitting function
 * c) execute and get results
 * d) Convert results to output information
 *
 */
void SetCrystalLocation::exec() {
  EventWorkspace_sptr events = getProperty("EventWorkspace");
  EventWorkspace_sptr outEvents = getProperty("ModifiedEventWorkspace");
  double newX = getProperty("NewX");
  double newY = getProperty("NewY");
  double newZ = getProperty("NewZ");
  V3D newSamplePos = V3D(newX, newY, newZ);
  if (events != outEvents) {
    outEvents = events->clone();
  }

  auto &componentInfo = outEvents->mutableComponentInfo();
  CalibrationHelpers::adjustUpSampleAndSourcePositions(
      componentInfo.l1(), newSamplePos, componentInfo);
 

} // exec

} // namespace Crystal

} // namespace Mantid
