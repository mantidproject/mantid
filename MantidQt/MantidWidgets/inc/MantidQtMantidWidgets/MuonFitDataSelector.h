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
  /// Get selected filenames
  QStringList getFilenames() const override;
  /// Get selected start time
  double getStartTime() const override;
  /// Get selected end time
  double getEndTime() const override;
  /// Get names of chosen groups
  QStringList getChosenGroups() const override;
  /// Set chosen group
  void setChosenGroup(const QString &group) override;
  /// Get selected periods
  QStringList getPeriodSelections() const override;
  /// Set selected period
  void setChosenPeriod(const QString &period) override;
  /// Get type of fit
  IMuonFitDataSelector::FitType getFitType() const override;
  /// Get instrument name
  QString getInstrumentName() const override;
  /// Get selected run numbers
  QString getRuns() const override;
  /// Get label for simultaneous fit
  QString getSimultaneousFitLabel() const override;
  /// Set label for simultaneous fit
  void setSimultaneousFitLabel(const QString &label) override;
  /// Get index of selected dataset
  int getDatasetIndex() const override;
  /// Set names of datasets for selection
  void setDatasetNames(const QStringList &datasetNames) override;
  /// Get name of selected dataset
  QString getDatasetName() const override;
  /// Ask user whether to overwrite label or not
  bool askUserWhetherToOverwrite() override;

public slots:
  /// Set number of periods in data
  void setNumPeriods(size_t numPeriods) override;
  /// Set starting run number and instrument
  void setWorkspaceDetails(const QString &runNumbers,
                           const QString &instName) override;
  /// Set names of available groups
  void setAvailableGroups(const QStringList &groupNames) override;
  /// Set start time for fit
  void setStartTime(double start) override;
  /// Set end time for fit
  void setEndTime(double end) override;
  /// Set start time without sending a signal
  void setStartTimeQuietly(double start) override;
  /// Set end time without sending a signal
  void setEndTimeQuietly(double end) override;
  /// Called when user changes runs
  void userChangedRuns();
  /// Called when period combination box checked/unchecked
  void periodCombinationStateChanged(int state);
  /// Called when fit type changed
  void fitTypeChanged(bool state);
  /// Called when group/period box selection changes
  void checkForMultiGroupPeriodSelection();

signals:
  /// Edited the start or end fields
  void dataPropertiesChanged();
  /// Changed the groups selection
  void selectedGroupsChanged();
  /// Changed the periods selection
  void selectedPeriodsChanged();
  /// Changed the workspace
  void workspaceChanged();
  /// Simultaneous fit label changed
  void simulLabelChanged();
  /// Dataset index changed
  void datasetIndexChanged(int index);

private:
  /// Add a checkbox to Groups section
  void addGroupCheckbox(const QString &name);
  /// Clear all checkboxes from Groups section
  void clearGroupCheckboxes();
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
  /// Check/uncheck "Combination" box and enable/disable text boxes
  void setPeriodCombination(bool on);
  /// Set busy cursor and disable input
  void setBusyState();
  /// Member - user interface
  Ui::MuonFitDataSelector m_ui;
  /// Map of group names to checkboxes
  QMap<QString, QCheckBox *> m_groupBoxes;
  /// Map of period names to checkboxes
  QMap<QString, QCheckBox *> m_periodBoxes;

private slots:
  /// Set normal cursor and enable input
  void unsetBusyState();
  /// Change dataset to previous one
  void setPreviousDataset();
  /// Change dataset to next one
  void setNextDataset();
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MUONFITDATASELECTOR_H_ */