#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_

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
class TomoToolConfigDialogBase {
public:
  TomoToolConfigDialogBase(QWidget *parent = 0);
  virtual ~TomoToolConfigDialogBase() {}

  static TomoToolConfigDialogBase *
  getCorrectDialogForToolFromString(const std::string &toolName);

  virtual void setUpDialog() = 0;
  virtual int execute() = 0;

};
}
}

#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGDIALOG_H_
