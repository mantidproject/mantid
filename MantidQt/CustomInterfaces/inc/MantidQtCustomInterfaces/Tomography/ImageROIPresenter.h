#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGEROIPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGEROIPRESENTER_H_

#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidQtCustomInterfaces/Tomography/IImageROIPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImageROIView.h"
#include "MantidQtCustomInterfaces/Tomography/ImageStackPreParams.h"
#include "MantidQtCustomInterfaces/Tomography/StackOfImagesDirs.h"

#include <boost/scoped_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {

/**
Presenter for the image center of rotation (and other parameters)
selection widget. In principle, in a strict MVP setup, signals from
the model should always be handled through this presenter and never go
directly to the view, and viceversa.

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
class DLLExport ImageROIPresenter : public IImageROIPresenter {

public:
  /// Default constructor - normally used from the concrete view
  ImageROIPresenter(IImageROIView *view);
  ~ImageROIPresenter() override;

  void notify(IImageROIPresenter::Notification notif) override;

protected:
  void initialize();

  /// clean shut down of model, view, etc.
  void cleanup();

  void processInit();
  void processBrowseImg();
  void processNewStack();
  void processUpdateImgIndex();
  void processSelectCoR();
  void processSelectROI();
  void processSelectNormalization();
  void processFinishedCoR();
  void processFinishedROI();
  void processFinishedNormalization();
  void processResetCoR();
  void processResetROI();
  void processResetNormalization();
  void processShutDown();

private:
  StackOfImagesDirs checkInputStack(const std::string &path);

  /// loads a list of images from a stack, from their individual paths
  Mantid::API::WorkspaceGroup_sptr
  loadFITSStack(const std::vector<std::string> &imgs);

  void loadFITSImage(const std::string &path, const std::string &wsName);

  std::string
  filterImagePathsForFITSStack(const std::vector<std::string> &paths);

  /// whether to show (potentially too many and too annoying) warning pop-ups or
  /// messages
  static bool g_warnIfUnexpectedFileExtensions;

  /// path to the image stack being visualized
  std::string m_stackPath;

  /// Associated view for this presenter (MVP pattern)
  IImageROIView *const m_view;

  /// Associated model for this presenter (MVP pattern). This is just
  /// a set of coordinates
  const boost::scoped_ptr<ImageStackPreParams> m_model;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGEROIPRESENTER_H_
