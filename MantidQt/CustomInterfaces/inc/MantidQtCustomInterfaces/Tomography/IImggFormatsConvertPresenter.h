#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMGGFORMATSCONVERTPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMGGFORMATSCONVERTPRESENTER_H_

namespace MantidQt {
namespace CustomInterfaces {

/**
Presenter for the widget to convert images and stacks of images
between different formats.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
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
class IImggFormatsConvertPresenter {

public:
  IImggFormatsConvertPresenter(){};
  virtual ~IImggFormatsConvertPresenter(){};

  /// These are user actions, triggered from the (passive) view, that need
  /// handling by the presenter
  enum Notification {
    Init,    ///< interface is initing (reload settings, set defaults, etc.)
    Convert, ///< User starts a conversion
    ShutDown ///< The widget is being closed/destroyed
  };

  /**
   * Notifications sent through the presenter when an (smart) action
   * is required to respond to user requests or something changes in
   * the view. This plays the role of signals emitted by the view to
   * this presenter.
   *
   * @param notif Type of notification to process.
   */
  virtual void notify(IImggFormatsConvertPresenter::Notification notif) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMGGFORMATSCONVERTPRESENTER_H_
