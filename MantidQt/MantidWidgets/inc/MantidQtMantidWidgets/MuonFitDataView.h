#ifndef MANTID_MANTIDWIDGETS_MUONFITDATAVIEW_H_
#define MANTID_MANTIDWIDGETS_MUONFITDATAVIEW_H_

#include "ui_MuonFitDataView.h"
#include "WidgetDllOption.h"
#include "MantidQtMantidWidgets/IMuonFitDataView.h"
#include "MantidQtAPI/MantidWidget.h"
#include "MantidQtMantidWidgets/MuonFitDataPresenter.h"

namespace MantidQt {
namespace MantidWidgets {

/** MuonFitDataView : Selects runs, groups, periods for fit

  This is the lightweight view for the widget. All the work is done by
  the presenter MuonFitDataPresenter.

  Implements IMuonFitDataView

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MuonFitDataView
    : public MantidQt::API::MantidWidget,
      public IMuonFitDataView {
  Q_OBJECT
public:
  /// Constructor
  MuonFitDataView(QWidget *parent, int runNumber, const QString &instName,
                  size_t numPeriods, const QStringList &groups);
  // --- MantidWidget methods ---
  /// Get user input through a common interface
  QVariant getUserInput() const override;
  /// Set user input through a common interface
  void setUserInput(const QVariant &value) override;
  // --- end ---
  // --- IMuonFitDataView methods
  QStringList getRuns() const override;
  unsigned int getWorkspaceIndex() const override;
  void setWorkspaceIndex(unsigned int index) override;
  double getStartTime() const override;
  void setStartTime(double start) override;
  double getEndTime() const override;
  void setEndTime(double end) override;
  void setPeriodVisibility(bool visible) override;
  void addGroupCheckbox(const QString &name) override;
  void clearGroupCheckboxes() override;
  bool isGroupSelected(const QString &name) const override;
  void setGroupSelected(const QString &name, bool selected) override;
  void setNumPeriods(size_t numPeriods) override;
  QStringList getPeriodSelections() const override;
  // --- end ---
  /// Set starting run number and instrument
  void setWorkspaceDetails(int runNumber, const QString &instName);

private:
  /// Set default values in some input controls
  void setDefaultValues();
  /// Set up validators for input
  void setUpValidators();
  /// Member - user interface
  Ui::MuonFitDataView m_ui;
  /// Presenter
  std::unique_ptr<MuonFitDataPresenter> m_presenter;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MUONFITDATAVIEW_H_ */