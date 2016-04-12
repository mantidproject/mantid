#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGEROIPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGEROIPRESENTER_H_

namespace MantidQt {
namespace CustomInterfaces {

/**
Presenter for the widget that handles the selection of the center of
rotation, region of interest, region for normalization, etc. from an
image or stack of images. This is the abstract base class / interface
for the presenter (in the sense of the MVP pattern).  The name
ImageROI refers to the Center-of-Rotation, which is the most basic
parameter that users can select via this widget. This class is
QtGUI-free as it uses the interface of the view. The model is simply
the ImageStackPreParams class which holds coordinates selected by the
user.

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD
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
class IImageROIPresenter {

public:
  IImageROIPresenter(){};
  virtual ~IImageROIPresenter(){};

  /// These are user actions, triggered from the (passive) view, that need
  /// handling by the presenter
  enum Notification {
    Init,                  ///< interface is initing (set, defaults, etc.)
    BrowseImgOrStack,      ///< User browses for an image file or stack
    NewImgOrStack,         ///< A new image or stack needs to be loaded
    ChangeImageType,       ///< Change between sample/flat/dark images
    ChangeRotation,        ///< Modify the rotation in the images displayed
    UpdateImgIndex,        ///< Sliding/scrolling through the stack
    PlayStartStop,         ///< User starts or stops the "play images"
    SelectCoR,             ///< Start picking of the center of rotation
    SelectROI,             ///< Start selection of the region of interest
    SelectNormalization,   ///< Start selection of the normalization region
    FinishedCoR,           ///< A CoR has been picked
    FinishedROI,           ///< The ROI is selected
    FinishedNormalization, ///< The normalization region is selected
    ResetCoR,              ///< Reset CoR to default/none/middle
    ResetROI,              ///< Reset ROI to default/empty
    ResetNormalization,    ///< Reet the normalization region to default/empty
    ShutDown               ///< The widget is being closed/destroyed
  };

  /**
   * Notifications sent through the presenter when something changes
   * in the view. This plays the role of signals emitted by the view
   * to this presenter.
   *
   * @param notif Type of notification to process.
   */
  virtual void notify(IImageROIPresenter::Notification notif) = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGEROIPRESENTER_H_
