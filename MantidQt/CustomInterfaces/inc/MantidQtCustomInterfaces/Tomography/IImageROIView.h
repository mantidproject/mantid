#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGEROIVIEW_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGEROIVIEW_H_

#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Widget to handle the selection of the center of rotation, region of
interest, region for normalization, etc. from an image or stack of
images. This is the abstract base class / interface for the view of
this widget (in the sense of the MVP pattern).  The name ImageROI
refers to the Center-of-Rotation, which is the most basic parameter
that users can select via this widget. This class is Qt-free. Qt
specific functionality and dependencies are added in a class derived
from this.

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
class IImageROIView {

public:
  IImageROIView(){};
  virtual ~IImageROIView(){};

  // Selection states
  enum SelectionState {
    SelectNone, ///< Init, or after any reset
    SelectCoR,
    SelectROIFirst,
    SelectROISecond,
    SelectNormAreaFirst,
    SelectNormAreaSecond
  };

  /**
   * Sets the user selection. This should guarantee that all widgets
   * are updated (including spin boxes, image, slider through the
   * image stack, etc.
   *
   * @param params all user-modifyable parameters (coordinates for the
   * CoR, ROI and area for normalization).
   *
   */
  virtual void setParams(ImageStackPreParams &params) = 0;

  /**
   * Provides the current user selection.
   *
   * @return parameters as set/edited by the user.
   */
  virtual ImageStackPreParams userSelection() const = 0;

  /**
   * The current selection state. For example: nothin/initialized,
   * selecting CoR, selecting second corner of the normalization area,
   * selecting first corner of the ROI.
   *
   * @return current state
   */
  virtual SelectionState selectionState() const = 0;

  /**
   * Update to a new state (for example select CoR).
   *
   * @param state new state we're transitioning into.
   */
  virtual void changeSelectionState(const SelectionState &state) = 0;

  /**
   * Display a special case of stack of images: individual image, from
   * a path to a recognized directory structure (sample/dark/white) or
   * image format. Here recognized format means something that is
   * supported natively by the widgets library, in practice
   * Qt. Normally you can expect that .tiff and .png images are
   * supported.
   *
   * @param path path to the stack (directory) or individual image file.
   */
  virtual void showStack(const std::string &path) = 0;

  /**
   * Display a stack of images (or individual image as a particular
   * case), from a workspace group containing matrix workspaces. It
   * assumes that the workspace contains an image in the form in which
   * LoadFITS loads FITS images (or spectrum per row, all of them with
   * the same number of data points (columns)).
   *
   * @param wsg Workspace group where every workspace is a FITS or
   * similar image that has been loaded with LoadFITS or similar
   * algorithm. This holds the sample images.
   *
   * @param wsgFlats Workspace group with flat (open beam) images.
   *
   * @param wsgDarks Workspace group with dark images.
   */
  virtual void showStack(const Mantid::API::WorkspaceGroup_sptr &wsg,
                         const Mantid::API::WorkspaceGroup_sptr &wsgFlats,
                         const Mantid::API::WorkspaceGroup_sptr &wsgDarks) = 0;

  /**
   * Get the stack of (sample) images currently being displayed (it
   * has been shown using showStack()), as a workspace group.
   *
   * @return workspace group containing the individual images, which
   * can be empty if no stack has been loaded.
   */
  virtual const Mantid::API::WorkspaceGroup_sptr stackSamples() const = 0;

  /**
   * Normally one image (projection for tomography stacks) will be
   * shown on a 2D display. Show there a particular projection from a
   * stack contained in a workspace group.
   *
   * @param wsg workspace holding a stack of images
   *
   * @param idx index (in the group) of the image to show
   *
   */
  virtual void showProjection(const Mantid::API::WorkspaceGroup_sptr &wsg,
                              size_t idx) = 0;

  /**
   * Display a warning to the user (for example as a pop-up window).
   *
   * @param warn warning title, should be short and would normally be
   * shown as the title of the window or a big banner.
   *
   * @param description longer, free form description of the issue.
   */
  virtual void userWarning(const std::string &warn,
                           const std::string &description) = 0;

  /**
   * To enable/disable all actions in the view. Useful when it is
   * necessary to prevent the user from requesting actions. For
   * example, during a lenghty execution of a process, drawing, etc.
   *
   * @param enable whether to enable or disable actions.
   */
  virtual void enableActions(bool enable) = 0;

  /**
   * Display an error message (for example as a pop-up window).
   *
   * @param err Error title, should be short and would normally be
   * shown as the title of the window or a big banner.
   *
   * @param description longer, free form description of the issue.
   */
  virtual void userError(const std::string &err,
                         const std::string &description) = 0;

  /**
   * The images of the type selected by the user (samples/flats/darks).
   *
   * @return a workspace group with the images of the current type
   */
  virtual Mantid::API::WorkspaceGroup_sptr currentImageTypeStack() const = 0;

  /**
   * Sets the type of image (sample/flat/dark).
   *
   * @param wsg workspace group that contains the type of image
   */
  virtual void updateImageType(const Mantid::API::WorkspaceGroup_sptr wsg) = 0;

  /**
   * The index of the image currently shown (from the current stack if there's
   * any).
   *
   * @return index from 0 to the total number of images in the
   * stack-1, as used for example when indexing workspaces in
   * workspacegroups
   */
  virtual size_t currentImgIndex() const = 0;

  /**
   * Display now this image (idx) from the stack.
   *
   * @param idx index of the image to display.
   */
  virtual void updateImgWithIndex(size_t idx) = 0;

  /**
   * Start to play/animate the stack currently displayed.
   */
  virtual void playStart() = 0;

  /**
   * Stop playing/animating the stack currently displayed, and goes
   * back to the default status.
   */
  virtual void playStop() = 0;

  /**
   * The rotation angle selected.
   *
   * @return angle in degrees.
   */
  virtual float currentRotationAngle() const = 0;

  /**
   * Modify the rotation angle selection and update the image display
   * to match the new rotation.
   *
   * @param angle rotation angle in degrees
   */
  virtual void updateRotationAngle(float angle) = 0;

  /**
   * Get the path/location of a stack of images (or single image as a
   * particular case) that the user is requesting to display.  The
   * path would be expected to point to a recognized directory
   * structure (sample/dark/white) or image file (as a particular
   * case).
   *
   * @return location (can be a directory, file, etc.) that needs to
   * be figured out elsewhere.
   */
  virtual std::string askImgOrStackPath() = 0;

  /**
   * Save settings (normally when closing this widget).
   */
  virtual void saveSettings() const = 0;

  /**
   * Forget the current center-of-rotation selection and set to
   * default.
   */
  virtual void resetCoR() = 0;

  /**
   * Forget the current region-of-interest selection and set to
   * default (all).
   */
  virtual void resetROI() = 0;

  /**
   * Forget the current selection of region-for-normalization and set
   * to default (none).
   */
  virtual void resetNormArea() = 0;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IIMAGEROIVIEW_H_
