// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------
// Includes
//----------------------------------
#include "ISlitCalculator.h"
#include "MantidAPI/InstrumentDataService.h"

#include "DllOption.h"
#include "ui_SlitCalculator.h"
#include <QDialog>

namespace MantidQt {
namespace MantidWidgets {
/** SlitCalculator : A calculator for Reflectometry instrument slits
 */
class EXPORT_OPT_MANTIDQT_COMMON SlitCalculator : public QDialog, public ISlitCalculator {
  Q_OBJECT
public:
  SlitCalculator(QWidget *parent);
  ~SlitCalculator() override;
  void setCurrentInstrumentName(std::string instrumentName) override;
  void processInstrumentHasBeenChanged() override;
  void show() override;

protected:
  Ui::SlitCalculator ui;

private:
  Mantid::Geometry::Instrument_const_sptr instrument;
  std::string currentInstrumentName;
  void setupSlitCalculatorWithInstrumentValues(const Mantid::Geometry::Instrument_const_sptr & /*instrument*/);
  std::string getCurrentInstrumentName();
  Mantid::Geometry::Instrument_const_sptr getInstrument();
  void setInstrument(const std::string &instrumentName);
private slots:
  void on_recalculate_triggered();
};
} // namespace MantidWidgets
} // namespace MantidQt
