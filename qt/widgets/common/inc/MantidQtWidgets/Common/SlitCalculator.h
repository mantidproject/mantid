// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_SLITCALCULATOR_H
#define MANTID_MANTIDWIDGETS_SLITCALCULATOR_H

//----------------------------------
// Includes
//----------------------------------
#include "MantidAPI/InstrumentDataService.h"

#include "DllOption.h"
#include "ui_SlitCalculator.h"
#include <QDialog>

namespace MantidQt {
namespace MantidWidgets {
/** SlitCalculator : A calculator for Reflectometry instrument slits
 */
class EXPORT_OPT_MANTIDQT_COMMON SlitCalculator : public QDialog {
  Q_OBJECT
public:
  SlitCalculator(QWidget *parent);
  ~SlitCalculator() override;
  void setCurrentInstrumentName(std::string instrumentName);
  void processInstrumentHasBeenChanged();

protected:
  Ui::SlitCalculator ui;

private:
  Mantid::Geometry::Instrument_const_sptr instrument;
  std::string currentInstrumentName;
  void setupSlitCalculatorWithInstrumentValues(
      Mantid::Geometry::Instrument_const_sptr /*instrument*/);
  std::string getCurrentInstrumentName();
  Mantid::Geometry::Instrument_const_sptr getInstrument();
  void setInstrument(std::string instrumentName);
private slots:
  void on_recalculate_triggered();
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_SLITCALCULATOR_H */
