#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFITDATAPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFITDATAPRESENTER_H_

#include "ConvFitModel.h"
#include "IndirectFitDataPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class ConvFitAddWorkspaceDialog;

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
class DLLExport ConvFitDataPresenter : public IndirectFitDataPresenter {
  Q_OBJECT
public:
  ConvFitDataPresenter(ConvFitModel *model, IndirectFitDataView *view);

private slots:
  void setModelResolution(const QString &name);

protected:
  void addModelData(const std::string &name) override;

private:
  void addDataToModel(IAddWorkspaceDialog const *dialog) override;
  std::unique_ptr<IAddWorkspaceDialog>
  getAddWorkspaceDialog(QWidget *parent) const override;
  void addWorkspace(ConvFitAddWorkspaceDialog const *dialog,
                    IndirectFittingModel *model);

  ConvFitModel *m_convModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFITDATAPRESENTER_H_ */
