#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATAPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATAPRESENTER_H_

#include "IAddWorkspaceDialog.h"
#include "IndirectDataTablePresenter.h"
#include "IndirectFitDataView.h"
#include "IndirectFittingModel.h"

#include "MantidAPI/MatrixWorkspace.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
  A presenter.
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
class DLLExport IndirectFitDataPresenter : public QObject {
  Q_OBJECT
public:
  IndirectFitDataPresenter(IndirectFittingModel *model,
                           IndirectFitDataView *view);

  IndirectFitDataPresenter(
      IndirectFittingModel *model, IndirectFitDataView *view,
      std::unique_ptr<IndirectDataTablePresenter> tablePresenter);

  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);

  void setStartX(double startX, std::size_t dataIndex, int spectrumIndex);
  void setEndX(double endX, std::size_t dataIndex, int spectrumIndex);
  void setExclude(const std::string &exclude, std::size_t dataIndex,
                  int spectrumIndex);

  void loadSettings(const QSettings &settings);
  UserInputValidator &validate(UserInputValidator &validator);

public slots:
  void updateSpectraInTable(std::size_t dataIndex);

protected slots:
  void setModelWorkspace(const QString &name);
  void setModelFromSingleData();
  void setModelFromMultipleData();
  void showAddWorkspaceDialog();

signals:
  void singleSampleLoaded();
  void singleResolutionLoaded();
  void dataAdded();
  void dataRemoved();
  void dataChanged();
  void startXChanged(double, std::size_t, std::size_t);
  void endXChanged(double, std::size_t, std::size_t);
  void excludeRegionChanged(const std::string &, std::size_t, std::size_t);
  void multipleDataViewSelected();
  void singleDataViewSelected();
  void requestedAddWorkspaceDialog();

protected:
  IndirectFitDataView const *getView() const;
  void addData(IAddWorkspaceDialog const *dialog);
  virtual void addDataToModel(IAddWorkspaceDialog const *dialog);
  void setSingleModelData(const std::string &name);
  virtual void addModelData(const std::string &name);
  void setResolutionHidden(bool hide);
  void displayWarning(const std::string &warning);
  virtual void dialogExecuted(IAddWorkspaceDialog const *dialog,
                              QDialog::DialogCode result);

private:
  virtual std::unique_ptr<IAddWorkspaceDialog>
  getAddWorkspaceDialog(QWidget *parent) const;

  IndirectFittingModel *m_model;
  PrivateFittingData m_singleData;
  PrivateFittingData m_multipleData;
  IndirectFitDataView *m_view;
  std::unique_ptr<IndirectDataTablePresenter> m_tablePresenter;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITDATAPRESENTER_H_ */
