#ifndef MANTID_MANTIDWIDGETS_MUONFITDATASELECTOR_H_
#define MANTID_MANTIDWIDGETS_MUONFITDATASELECTOR_H_

#include "DllOption.h"
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include "MantidQtWidgets/Common/IMuonFitDataSelector.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Common/MuonFitPropertyBrowser.h"
#include "ui_MuonFitDataSelector.h"

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
class EXPORT_OPT_MANTIDQT_COMMON MuonFitDataSelector
    : public MantidQt::API::MantidWidget,
      public IMuonFitDataSelector {
  Q_OBJECT

public:
  /// Basic constructor
  explicit MuonFitDataSelector(QWidget *parent);
  /// Constructor with more options
  MuonFitDataSelector(QWidget *parent, int runNumber, const QString &instName);
  //, size_t numPeriods, const QStringList &groups);
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
  /// Set chosen group/period
  void setGroupsSelected(QStringList groups) { m_chosenGroups = groups; };
  void setPeriodsSelected(QStringList periods) { m_chosenPeriods = periods; };
  /// Get selected periods
  QStringList getPeriodSelections() const override;
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
  /// Set starting run number, instrument and (optionally) file path
  void setWorkspaceDetails(const QString &runNumbers, const QString &instName,
                           const boost::optional<QString> &filePath) override;
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
  /// Called when fit type changed
  void fitTypeChanged(bool state);
  /// Called when group/period box selection changes
  void checkForMultiGroupPeriodSelection();
  void updateNormalizationFromDropDown(int);
signals:
  /// Edited the start or end fields
  void dataPropertiesChanged();
  /// Changed the workspace
  void workspaceChanged();
  /// Simultaneous fit label changed
  void simulLabelChanged();
  /// Dataset index changed
  void datasetIndexChanged(int index);
  void nameChanged(QString name);

private:
  /// Set default values in some input controls
  void setDefaultValues();
  /// Set up connections for signals/slots
  void setUpConnections();
  /// Set type for fit
  void setFitType(IMuonFitDataSelector::FitType type);
  /// Set busy cursor and disable input
  void setBusyState();
  /// Member - user interface
  Ui::MuonFitDataSelector m_ui;
  double m_startX;
  double m_endX;
  QStringList m_chosenGroups;
  QStringList m_chosenPeriods;
  bool m_multiFit;

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
