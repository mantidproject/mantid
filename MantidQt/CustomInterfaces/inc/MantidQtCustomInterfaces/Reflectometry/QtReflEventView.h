#ifndef MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_

#include "MantidQtCustomInterfaces/Reflectometry/IReflEventView.h"
#include "ui_ReflEventWidget.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

// Forward decs
class IReflEventPresenter;

/** QtReflEventView : Provides an interface for the "Event Handling" widget in
the ISIS Reflectometry interface.

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
class QtReflEventView : public QWidget, public IReflEventView {
  Q_OBJECT
public:
  /// Constructor
  QtReflEventView(QWidget *parent = 0);
  /// Destructor
  ~QtReflEventView() override;
  /// Returns the presenter managing this view
  IReflEventPresenter *getPresenter() const override;

  /// Returns time-slicing values
  std::string getTimeSlicingValues() const override;
  /// Returns time-slicing type
  std::string getTimeSlicingType() const override;

public slots:
  /// Enable / disable slicing option entry fields
  void toggleSlicingOptions() const;

private:
  /// Initialise the interface
  void initLayout();

  /// The widget
  Ui::ReflEventWidget m_ui;
  /// The presenter
  std::unique_ptr<IReflEventPresenter> m_presenter;

  /// Current slice type
  mutable std::string m_sliceType;

  /// List of radio buttons
  std::vector<QRadioButton *> m_buttonList;
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLEVENTVIEW_H_ */
