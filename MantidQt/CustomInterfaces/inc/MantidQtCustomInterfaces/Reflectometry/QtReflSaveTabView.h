#ifndef MANTID_CUSTOMINTERFACES_QTREFLSAVETABVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLSAVETABVIEW_H_

#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabView.h"
#include <memory>

#include "ui_ReflSaveTabWidget.h"

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflSaveTabPresenter;

/** QtReflSaveTabView : Provides an interface for the "Save ASCII" tab in the
Reflectometry (Polref) interface.

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
class MANTIDQT_CUSTOMINTERFACES_DLL QtReflSaveTabView
    : public QWidget,
      public IReflSaveTabView {
  Q_OBJECT
public:
  /// Constructor
  QtReflSaveTabView(QWidget *parent = 0);
  /// Destructor
  ~QtReflSaveTabView() override;

  /// Returns the save path
  std::string getSavePath() const override;
  /// Returns the prefix
  std::string getPrefix() const override;
  /// Returns the filter
  std::string getFilter() const override;
  /// Returns the reg exp check
  bool getRegExpCheck() const override;
  /// Returns the list of workspaces
  std::string getListOfWorkspaces() const override;
  /// Returns the list of parameters
  std::string getListOfParameters() const override;
  /// Returns the spectra list
  std::string getSpectraList() const override;
  /// Returns the file format
  std::string getFileFormat() const override;
  /// Returns the title check
  bool getTitleCheck() const override;
  /// Returns the Q resolution check
  bool getQResolutionCheck() const override;
  /// Returns the separator type
  std::string getSeparator() const override;

private:
  /// Initialize the interface
  void initLayout();
  /// The presenter
  std::unique_ptr<IReflSaveTabPresenter> m_presenter;
  /// The widget
  Ui::ReflSaveTabWidget m_ui;

};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLSAVETABVIEW_H_ */
