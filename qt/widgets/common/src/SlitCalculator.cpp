#include "MantidQtWidgets/Common/SlitCalculator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidKernel/Strings.h"

#include <cmath>

namespace MantidQt {
namespace MantidWidgets {
SlitCalculator::SlitCalculator(QWidget *parent) {
  Q_UNUSED(parent);
  ui.setupUi(this);
  if (currentInstrumentName == "") {
    // set up the initial instrument if there is not one associated
    // with slit calculator
    currentInstrumentName = "INTER";
    setInstrument(currentInstrumentName);
  }
  on_recalculate_triggered();
}
void SlitCalculator::processInstrumentHasBeenChanged() {
  // used in refl main window to indicate that slitCalculator fields
  // need to update because another instrument has been selected.
  on_recalculate_triggered();
}
SlitCalculator::~SlitCalculator() {}
void SlitCalculator::setInstrument(std::string instrumentName) {
  // we want to get the most up-to-date definition, so we use the current
  // date/time
  auto date =
      Mantid::Types::Core::DateAndTime::getCurrentTime().toISO8601String();
  // find the full path to the definition file
  auto filename =
      Mantid::API::ExperimentInfo::getInstrumentFilename(instrumentName, date);
  // parse the XML that we have found for the definition
  Mantid::Geometry::InstrumentDefinitionParser parser =
      Mantid::Geometry::InstrumentDefinitionParser(
          filename, instrumentName,
          Mantid::Kernel::Strings::loadFile(filename));
  // retrieving the mangled name of the instrument
  std::string instrumentNameMangled = parser.getMangledName();
  // See if we have a definition already in the InstrumentDataService
  if (Mantid::API::InstrumentDataService::Instance().doesExist(
          instrumentNameMangled)) {
    // If it does, set the associated instrument to the one we have found.
    this->instrument = Mantid::API::InstrumentDataService::Instance().retrieve(
        instrumentNameMangled);
  } else {
    // We set the instrument from XML that we have found.
    Mantid::API::Progress prog;
    this->instrument = parser.parseXML(&prog);
  }
  setupSlitCalculatorWithInstrumentValues(instrument);
}

void SlitCalculator::setupSlitCalculatorWithInstrumentValues(
    Mantid::Geometry::Instrument_const_sptr instrument) {
  // fetch the components that we need for values from IDF
  auto slit1Component = instrument->getComponentByName("slit1");
  auto slit2Component = instrument->getComponentByName("slit2");
  auto sampleComponent = instrument->getComponentByName("some-surface-holder");
  // check that they have been fetched from the IDF
  if (slit1Component.get() != nullptr && slit2Component.get() != nullptr &&
      sampleComponent.get() != nullptr) {
    // convert from meters to millimeters
    const double s1s2 = 1e3 * (slit1Component->getDistance(*slit2Component));
    // set value in field of slitCalculator
    ui.spinSlit1Slit2->setValue(s1s2);
    // convert from meters to millimeters
    const double s2sa = 1e3 * (slit2Component->getDistance(*sampleComponent));
    // set value in field of slitCalculator
    ui.spinSlit2Sample->setValue(s2sa);
  } else {
    // the parameters slit1, slit2 and sample-holder where not found
    // set the values in SlitCalculator up so that it is obvious
    // we did not retrieve them from any IDF.
    ui.spinSlit1Slit2->setValue(0.0);
    ui.spinSlit2Sample->setValue(0.0);
  }
}
Mantid::Geometry::Instrument_const_sptr SlitCalculator::getInstrument() {
  return instrument;
}
void SlitCalculator::setCurrentInstrumentName(std::string instrumentName) {
  this->currentInstrumentName = instrumentName;
}
std::string SlitCalculator::getCurrentInstrumentName() {
  return currentInstrumentName;
}
void SlitCalculator::on_recalculate_triggered() {
  const auto currentInstrument = getInstrument();
  if (currentInstrument->getName() != currentInstrumentName) {
    setInstrument(currentInstrumentName);
  }
  // Gather input
  const double s1s2 = ui.spinSlit1Slit2->value();
  const double s2sa = ui.spinSlit2Sample->value();
  const double res = ui.spinResolution->value();
  const double footprint = ui.spinFootprint->value();
  const double angle = ui.spinAngle->value();

  // Calculate values
  Mantid::API::IAlgorithm_sptr algSlit =
      Mantid::API::AlgorithmManager::Instance().create("CalculateSlits");
  algSlit->initialize();
  algSlit->setChild(true);
  algSlit->setProperty("Slit1Slit2", s1s2);
  algSlit->setProperty("Slit2SA", s2sa);
  algSlit->setProperty("Resolution", res);
  algSlit->setProperty("Footprint", footprint);
  algSlit->setProperty("Angle", angle);
  algSlit->execute();

  const double s1 = algSlit->getProperty("Slit1");
  const double s2 = algSlit->getProperty("Slit2");

  // Update output
  ui.slit1Text->setText(QString::number(s1, 'f', 3));
  ui.slit2Text->setText(QString::number(s2, 'f', 3));
}
} // namespace MantidWidgets
} // namespace MantidQt
