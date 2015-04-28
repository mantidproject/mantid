#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_

#include "ui_TomoReconstruction.h"
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

Copyright &copy; 2014,205 ISIS Rutherford Appleton Laboratory, NScD
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

class TomoToolConfigTomoPy : public QDialog {
  Q_OBJECT

public:
  TomoToolConfigTomoPy(QWidget *parent = 0);
};

class TomoToolConfigSavu : public QMainWindow {
  Q_OBJECT
public:
  TomoToolConfigSavu(QWidget *parent = 0);

private:
  void initLayout();
};

class TomoToolConfigAstra : public QDialog {
  Q_OBJECT
public:
  TomoToolConfigAstra(QWidget *parent = 0);

private:
  void initLayout();
};

class TomoToolConfigCustom : public QDialog {
  Q_OBJECT
public:
  TomoToolConfigCustom(QWidget *parent = 0);

private:
  void initLayout();
};

class TomoToolConfigDialog : public QDialog {
  Q_OBJECT

public:
  TomoToolConfigDialog(QWidget *parent = 0);

private:
  void initLayout();

private slots:
  void okClicked();
  void cancelClicked();

private:
  QLabel *labelRun, *labelOpt;
  QLineEdit *editRun, *editOpt;
  QHBoxLayout *hRun, *hOpt;
  QGridLayout *layout;
  QPushButton *okButton, *cancelButton;
};
}
}

#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_
