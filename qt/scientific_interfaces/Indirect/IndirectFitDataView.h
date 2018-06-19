#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTFITDATAVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTFITDATAVIEW_H_

#include "ui_IndirectFitDataView.h"

#include "../General/UserInputValidator.h"

#include <QTabWidget>

/**
  Copyright &copy; 2015-2016 ISIS Rutherford Appleton Laboratory, NScD
  Oak Ridge National Laboratory & European Spallation Source
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
namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitDataView : public QTabWidget {
  Q_OBJECT
public:
  IndirectFitDataView(QWidget *parent);
  ~IndirectFitDataView() override;

  QTableWidget *getDataTable() const;
  bool isMultipleDataTabSelected() const;
  bool isResolutionHidden() const;
  void setResolutionHidden(bool hide);
  void disableMultipleDataTab();

  std::string getSelectedSample() const;
  std::string getSelectedResolution() const;

  QStringList getSampleWSSuffices() const;
  QStringList getSampleFBSuffices() const;
  QStringList getResolutionWSSuffices() const;
  QStringList getResolutionFBSuffices() const;

  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);

  void readSettings(const QSettings &settings);
  UserInputValidator &validate(UserInputValidator &validator);

public slots:
  void displayWarning(const std::string &warning);

signals:
  void sampleLoaded(const QString &);
  void resolutionLoaded(const QString &);
  void addClicked();
  void removeClicked();
  void multipleDataViewSelected();
  void singleDataViewSelected();

protected slots:
  void emitViewSelected(int index);

private:
  UserInputValidator &validateMultipleData(UserInputValidator &validator);
  UserInputValidator &validateSingleData(UserInputValidator &validator);

  std::unique_ptr<Ui::IndirectFitDataForm> m_dataForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTFITDATAVIEW_H_ */
