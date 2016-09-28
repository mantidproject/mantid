#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_

#include "ui_TomoToolConfigAstra.h"
#include "ui_TomoToolConfigCustom.h"
#include "ui_TomoToolConfigSavu.h"
#include "ui_TomoToolConfigTomoPy.h"

#include <QDialog>

namespace MantidQt {
namespace CustomInterfaces {

/**
Third party tool configuration dialog(s) for the tomographic reconstruction
GUI.

Copyright &copy; 2014,2015 ISIS Rutherford Appleton Laboratory, NScD
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




class TomoToolConfigDialogBase : public QDialog {
  Q_OBJECT

public:
	TomoToolConfigDialogBase(QWidget *parent = 0);

	virtual void setUpDialog() = 0;
	virtual int execute() {
		return this->exec();
	};

	static TomoToolConfigDialogBase *fromString(const std::string &toolName);


private:
  void initLayout();

private slots:
  void okClicked();
  void cancelClicked();

private:
  QLabel *labelRun, *labelOpt;
  QLineEdit *editRun, *editOpt;
  QHBoxLayout *hRun, *hOpt, *hBut;
  QGridLayout *layout;
  QPushButton *okButton, *cancelButton;
};
}
}

#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_
