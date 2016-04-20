#ifndef MANTID_MANTIDWIDGETS_MUONFITDATASELECTOR_H_
#define MANTID_MANTIDWIDGETS_MUONFITDATASELECTOR_H_

#include "ui_MuonFitDataSelector.h"
#include "WidgetDllOption.h"
#include "MantidQtMantidWidgets/IMuonFitDataSelector.h"
#include "MantidQtAPI/MantidWidget.h"

namespace MantidQt {
namespace MantidWidgets {

/** MuonFitDataSelector : Selects runs, groups, periods for fit

  Widget to select data to fit for MuonAnalysis

  Implements IMuonFitDataSelector

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
    : public MantidQt::API::MantidWidget,
      public IMuonFitDataSelector {
  Q_OBJECT
public:
  /// Basic constructor
  explicit MuonFitDataSelector(QWidget *parent);
  /// Constructor with more options
  MuonFitDataSelector(QWidget *parent, int runNumber, const QString &instName,
                      size_t numPeriods, const QStringList &groups);
  // --- MantidWidget methods ---
  /// Get user input through a common interface
  QVariant getUserInput() const override;
  /// Set user input through a common interface
  void setUserInput(const QVariant &value) override;
  // --- IMuonFitDataSelector methods
  /// Get selected run numbers
  QStringList getRuns() const override;
  /// Get selected workspace index
  unsigned int getWorkspaceIndex() const override;
  /// Get selected start time
  double getStartTime() const override;
  /// Get selected end time
  double getEndTime() const override;
  /// Get names of chosen groups
  QStringList getChosenGroups() const override;
  /// Get selected periods
  QStringList getPeriodSelections() const override;
  /// Get type of fit
  IMuonFitDataSelector::FitType getFitType() const override;

public slots:
  /// Set number of periods in data
  void setNumPeriods(size_t numPeriods) override;
  /// Set starting run number and instrument
  void setWorkspaceDetails(const QString &runNumbers,
                           const QString &instName) override;
  /// Set names of available groups
  void setAvailableGroups(const QStringList &groupNames) override;
  /// Set selected workspace index
  void setWorkspaceIndex(unsigned int index) override;
  /// Set start time for fit
  void setStartTime(double start) override;
  /// Set end time for fit
  void setEndTime(double end) override;
  /// Set start time without sending a signal
  void setStartTimeQuietly(double start) override;
  /// Set end time without sending a signal
  void setEndTimeQuietly(double end) override;

signals:
  /// Edited the runs, ws index, start or end fields
  void workspacePropertiesChanged();
  /// Changed the groups selection
  void selectedGroupsChanged();
  /// Changed the periods selection
  void selectedPeriodsChanged();

private:
  /// Add a checkbox to Groups section
  void addGroupCheckbox(const QString &name);
  /// Clear all checkboxes from Groups section
  void clearGroupCheckboxes();
  /// Set selection status of a checkbox
  void setGroupSelected(const QString &name, bool selected);
  /// Set visibility of "Periods" section
  void setPeriodVisibility(bool visible);
  /// Set default values in some input controls
  void setDefaultValues();
  /// Set up validators for input
  void setUpValidators();
  /// Set up connections for signals/slots
  void setUpConnections();
  /// Set type for fit
  void setFitType(IMuonFitDataSelector::FitType type);
  /// Member - user interface
  Ui::MuonFitDataSelector m_ui;
  /// Map of group names to checkboxes
  QMap<QString, QCheckBox *> m_groupBoxes;
  /// Map of period names to checkboxes
  QMap<QString, QCheckBox *> m_periodBoxes;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MUONFITDATASELECTOR_H_ */