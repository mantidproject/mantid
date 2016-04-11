#ifndef MANTID_MANTIDWIDGETS_MUONFITDATASELECTOR_H_
#define MANTID_MANTIDWIDGETS_MUONFITDATASELECTOR_H_

#include "ui_MuonFitDataSelector.h"
#include "WidgetDllOption.h"
#include "MantidQtAPI/MantidWidget.h"

namespace MantidQt {
namespace MantidWidgets {

/** MuonFitDataSelector : Selects runs, groups, periods for fit

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MuonFitDataSelector
    : public MantidQt::API::MantidWidget {
  Q_OBJECT
public:
  /// Constructor
  MuonFitDataSelector(QWidget *parent, int runNumber, const QString &instName,
                      size_t numPeriods, const QStringList &groups);
  /// Get user input through a common interface
  QVariant getUserInput() const override;
  /// Set user input through a common interface
  void setUserInput(const QVariant &value) override;
  /// Set starting run number and instrument
  void setWorkspaceDetails(int runNumber, const QString &instName);
  /// Set groups available for user to choose
  void setGroupingOptions(const QStringList &groups);
  /// Set number of periods in data
  void setNumPeriods(size_t numPeriods);

private:
  /// Set default values in some input controls
  void setDefaultValues();
  /// Set up validators for input
  void setUpValidators();
  /// get workspace index
  unsigned int getWorkspaceIndex() const;
  /// get start time
  double getStartTime() const;
  /// get end time
  double getEndTime() const;
  /// get run filenames
  QStringList getRuns() const;
  /// Member - user interface
  Ui::MuonFitDataSelector m_ui;
  /// Starting run number
  int m_startingRun;
  /// Instrument name
  QString m_instName;
  /// Number of periods
  size_t m_numPeriods;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MUONFITDATASELECTOR_H_ */