#ifndef MANTID_CUSTOMINTERFACES_QTDATAPROCESSOROPTIONSDIALOG_H
#define MANTID_CUSTOMINTERFACES_QTDATAPROCESSOROPTIONSDIALOG_H

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithmView.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorPresenter.h"

#include <QDialog>

#include "ui_DataProcessorOptionsDialog.h"

namespace MantidQt {
namespace CustomInterfaces {

/** QtDataProcessorOptionsDialog : Provides a dialog for setting Reflectometry
UI
options.

Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport QtDataProcessorOptionsDialog : public QDialog {
  Q_OBJECT
public:
  QtDataProcessorOptionsDialog(
      DataProcessorAlgorithmView *view,
      boost::shared_ptr<DataProcessorPresenter> presenter);
  ~QtDataProcessorOptionsDialog() override;

protected:
  void initLayout();
  void initBindings();
protected slots:
  void saveOptions();
  void loadOptions();

protected:
  // the interface
  Ui::DataProcessorOptionsDialog ui;
  // the presenter
  boost::shared_ptr<DataProcessorPresenter> m_presenter;
  // maps option names to widget names
  std::map<std::string, QString> m_bindings;
};

} // CustomInterfaces
} // MantidQt

#endif /* MANTID_CUSTOMINTERFACES_QTDATAPROCESSOROPTIONSDIALOG_H */
