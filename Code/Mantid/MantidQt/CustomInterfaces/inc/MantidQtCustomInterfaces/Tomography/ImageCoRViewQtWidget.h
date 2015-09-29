#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGECORVIEWQTWIDGET_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGECORVIEWQTWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/Tomography/IImageCoRPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImageCoRView.h"

#include "ui_ImageSelectCoRAndRegions.h"

#include <boost/scoped_ptr.hpp>

// forward declarations for Qt
class QWidget;

namespace MantidQt {
namespace CustomInterfaces {

/**
Qt-based view of the widget to handle the selection of the center of
rotation, region of interest, region for normalization, etc. from an
image or stack of images. Provides a concrete view for the graphical
interface for tomography functionality in Mantid. This view is
Qt-based and it is probably the only one that will be implemented in a
foreseeable horizon. The interface of this class is given by
IImageCoRView so that it fits in the MVP (Model-View-Presenter) design
of the ImageCoR widget.

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
class DLLExport ImageCoRViewQtWidget : public QWidget, public IImageCoRView {
  Q_OBJECT

public:
  ImageCoRViewQtWidget(QWidget *parent = 0);
  virtual ~ImageCoRViewQtWidget(){};

  void initParams(ImageStackPreParams &params);

  ImageStackPreParams userSelection() const;

  virtual std::string stackPath() const { return m_stackPath; };

  /// show a stack of images given the path to the files
  void showStack(const std::string &path);

  /// show a stack of images that have been loaded into a group of workspaces
  void showStack(const Mantid::API::WorkspaceGroup_sptr &ws);

  void userWarning(const std::string &warn, const std::string &description);

  void userError(const std::string &err, const std::string &description);

  std::string askImgOrStackPath();

  void saveSettings() const;

protected:
  void initLayout();
  void showImg();

private slots:
  void browseImgClicked();

  void corClicked();
  void corResetClicked();
  void roiClicked();
  void roiResetClicked();
  void normAreaClicked();
  void normAreaResetClicked();

 private:
  void setupConnections();

  void readSettings();

  // widget closing
  virtual void closeEvent(QCloseEvent *ev);

  Ui::ImageSelectCoRAndRegions m_ui;

  /// persistent settings
  static const std::string m_settingsGroup;

  /// parameters currently set by the user
  ImageStackPreParams m_params;

  /// path to the image stack being visualized
  std::string m_stackPath;

  // presenter as in the model-view-presenter
  boost::scoped_ptr<IImageCoRPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGECORVIEWQTWIDGET_H_
