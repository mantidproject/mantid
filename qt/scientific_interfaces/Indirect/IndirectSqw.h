#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTSQW_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTSQW_H_

#include "IndirectDataReductionTab.h"

#include "MantidKernel/System.h"
#include "ui_IndirectSqw.h"

namespace MantidQt {
namespace CustomInterfaces {
/** IndirectSqw

  @author Dan Nixon
  @date 23/07/2014

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport IndirectSqw : public IndirectDataReductionTab {
  Q_OBJECT

public:
  IndirectSqw(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~IndirectSqw() override;

  void setup() override;
  void run() override;
  bool validate() override;

private slots:
  void plotContour();
  void sqwAlgDone(bool error);
  void plotClicked();
  void saveClicked();

private:
  Ui::IndirectSqw m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECTSQW_H_
