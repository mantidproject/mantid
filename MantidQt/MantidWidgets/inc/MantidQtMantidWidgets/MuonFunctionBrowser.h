#ifndef MANTID_MANTIDWIDGETS_MUONFUNCTIONBROWSER_H_
#define MANTID_MANTIDWIDGETS_MUONFUNCTIONBROWSER_H_

#include "WidgetDllOption.h"
#include "MantidQtMantidWidgets/FunctionBrowser.h"

namespace MantidQt {
namespace MantidWidgets {

/** MuonFunctionBrowser : Subclasses FunctionBrowser for muon-specific use

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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MuonFunctionBrowser
    : public FunctionBrowser {
  Q_OBJECT

public:
  /// Constructor
  MuonFunctionBrowser(QWidget *parent = nullptr, bool multi = false);
  /// Destructor
  virtual ~MuonFunctionBrowser() override;

protected:
  /// Ask user for function and return it
  QString getUserFunctionFromDialog() override;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MUONFUNCTIONBROWSER_H_ */