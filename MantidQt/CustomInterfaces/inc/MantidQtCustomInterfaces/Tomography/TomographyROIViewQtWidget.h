#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYROIVIEWQTWIDGET_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYROIVIEWQTWIDGET_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyROIPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/ITomographyROIView.h"

#include "ui_ImageSelectCoRAndRegions.h"

#include <boost/scoped_ptr.hpp>

#include <QTimer>
// forward declarations for Qt
class QWidget;
class QPixmap;

namespace Mantid {
namespace Kernel {
class V2D;
}
}

namespace MantidQt {
namespace CustomInterfaces {

/**
Qt-based view of the widget to handle the selection of the center of
rotation, region of interest, region for normalization, etc. from an
image or stack of images. Provides a concrete view for the graphical
interface for tomography functionality in Mantid. This view is
Qt-based and it is probably the only one that will be implemented in a
foreseeable horizon. The interface of this class is given by
ITomographyROIView so that it fits in the MVP (Model-View-Presenter) design
of the TomographyROI widget.

Copyright &copy; 2015-2016 ISIS Rutherford Appleton Laboratory, NScD
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
class MANTIDQT_CUSTOMINTERFACES_DLL TomographyROIViewQtWidget
    : public QWidget,
      public ITomographyROIView {
  Q_OBJECT

public:
  TomographyROIViewQtWidget(QWidget *parent = 0);
  ~TomographyROIViewQtWidget() override;

  void setParams(ImageStackPreParams &params) override;

  ImageStackPreParams userSelection() const override;

  SelectionState selectionState() const override { return m_selectionState; }

  void changeSelectionState(const SelectionState &state) override;

  /// show a stack of images given the path to the files
  void showStack(const std::string &path) override;

  /// show a (new) stack of images that have been loaded into groups of
  /// workspaces (samples, flats, darks)
  void showStack(const Mantid::API::WorkspaceGroup_sptr &wsg,
                 const Mantid::API::WorkspaceGroup_sptr &wsgFlats,
                 const Mantid::API::WorkspaceGroup_sptr &wsgDarks) override;

  const Mantid::API::WorkspaceGroup_sptr stackSamples() const override {
    return m_stackSamples;
  }

  void showProjection(const Mantid::API::WorkspaceGroup_sptr &wsg,
                      size_t idx) override;

  void userWarning(const std::string &warn,
                   const std::string &description) override;

  void userError(const std::string &err,
                 const std::string &description) override;

  void enableActions(bool enable) override;

  size_t currentImgIndex() const override;

  void updateImgWithIndex(size_t idx) override;

  void playStart() override;

  void playStop() override;

  float currentRotationAngle() const override;

  void updateRotationAngle(float angle) override;

  Mantid::API::WorkspaceGroup_sptr currentImageTypeStack() const override;

  void updateImageType(const Mantid::API::WorkspaceGroup_sptr wsg) override;

  std::string askImagePath(const std::string &windowTitle) override;

  std::string askColorMapFile() override;

  void updateColorMap(const std::string &filename) override;

  void saveSettings() const override;
  void imageOrStackLoaded(const std::string &path) override;

signals:
  void findCORClickedSignal(const std::string &exec,
                            const std::vector<std::string> &args);

  void imageOrStackLoadedSignal(const std::string &path);

public slots:
  void readCoRFromProcessOutput(const QString &str);

protected:
  void initLayout();
  void showImg();

  void resetCoR() override;
  void resetROI() override;
  void resetNormArea() override;

  void resetWidgetsOnNewStack();

  /// update coordinates from mouse event
  void mouseUpdateCoR(const int x, const int y);
  void mouseUpdateROICornersStartSelection(const int x, const int y);
  void mouseUpdateROICornerContinuedSelection(const int x, const int y);
  void mouseFinishROI(const int x, const int y);
  void mouseUpdateNormAreaCornersStartSelection(const int x, const int y);
  void mouseUpdateNormAreaCornerContinuedSelection(const int x, const int y);
  void mouseFinishNormArea(const int x, const int y);

private slots:
  void browseImageClicked();
  void browseStackClicked();

  void rotationUpdated(int idx);
  void autoCoRToolChanged(int idx);

  void imageTypeUpdated(int idx);

  void loadColorMapRequest();
  void colorRangeChanged();

  void corClicked();
  void corResetClicked();
  void roiClicked();
  void roiResetClicked();
  void normAreaClicked();
  void normAreaResetClicked();

  void playClicked();
  void updatePlay();

  void updateFromImagesSlider(int current);

  void valueUpdatedCoR(int v);
  void valueUpdatedROI(int v);
  void valueUpdatedNormArea(int v);
  void findCORClicked();

private:
  void setupConnections();

  void readSettings();

  /// enable types of images (sample, flat, dark)
  /// depending on their
  /// availability
  void enableImageTypes(bool enableSamples, bool enableFlats, bool enableDarks);

  /// enable/disable the groups with spin boxes for the
  /// center and corners
  void enableParamWidgets(bool enable);

  // widget closing
  void closeEvent(QCloseEvent *ev) override;

  /// initialize values to defaults and set max/min for
  /// the spin boxes
  void initParamWidgets(size_t maxWidth, size_t maxHeight);

  /// Set coordinates in the widgets from a params object
  void setParamWidgets(ImageStackPreParams &params);

  // shows the image in a widget
  void showProjectionImage(const Mantid::API::WorkspaceGroup_sptr &wsg,
                           size_t idx, float rotationAngle = 0);

  void getCheckedDimensions(const Mantid::API::MatrixWorkspace_sptr ws,
                            size_t &width, size_t &height);

  void checkNewProjectionImage(const Mantid::API::WorkspaceGroup_sptr &wsg,
                               size_t idx, size_t &width, size_t &height,
                               Mantid::API::MatrixWorkspace_sptr &imgWS,
                               std::string &imgName);

  void getPixelMinMax(Mantid::API::MatrixWorkspace_sptr ws, double &min,
                      double &max);

  QPixmap transferWSImageToQPixmap(const Mantid::API::MatrixWorkspace_sptr ws,
                                   size_t width, size_t height,
                                   float rotationAngle);

  void grabCoRFromWidgets();
  void grabROIFromWidgets();
  void grabNormAreaFromWidgets();

  void grabCoRFromMousePoint(int x, int y);
  void grabROICorner1FromMousePoint(int x, int y);
  void grabROICorner2FromMousePoint(int x, int y);
  void grabNormAreaCorner1FromMousePoint(int x, int y);
  void grabNormAreaCorner2FromMousePoint(int x, int y);

  /// repaint the image with new positions of points and
  /// rectangles
  void refreshImage();
  void refreshCoR();
  void refreshROI();
  void refreshNormArea();

  /// draw a cross/target symbol
  void drawCenterCrossSymbol(QPainter &painter,
                             const Mantid::Kernel::V2D &center) const;

  /// draw a rectangle/box to highlight the ROI: region of
  /// interest
  void drawBoxROI(QPainter &painter, const Mantid::Kernel::V2D &first,
                  const Mantid::Kernel::V2D &second) const;

  /// draw a rectangle/box to highlight the normalization
  /// ("air") region
  void drawBoxNormalizationRegion(QPainter &painter,
                                  const Mantid::Kernel::V2D &first,
                                  const Mantid::Kernel::V2D &second) const;

  bool eventFilter(QObject *obj, QEvent *event) override;

  struct RectangleXY {
    int right;
    int top;
  } m_startOfRectangle;

  void updateValuesForSpinBoxes(
      const int x, const int y,
      const struct TomographyROIViewQtWidget::RectangleXY startPositions,
      QSpinBox *spinLeft, QSpinBox *spinTop, QSpinBox *spinRight,
      QSpinBox *spinBottom);

  Ui::ImageSelectCoRAndRegions m_ui;

  Mantid::API::WorkspaceGroup_sptr m_stackSamples, m_stackFlats, m_stackDarks;

  /// this holds the base image on top of which rectangles
  /// and other
  /// objects are drawn
  boost::scoped_ptr<QPixmap> m_basePixmap;

  /// current color map used to visualize the image/slice
  std::string m_colorMapFilename;

  /// persistent settings
  static const std::string m_settingsGroup;

  /// parameters currently set by the user
  ImageStackPreParams m_params;

  /// Timer for the "play" function
  std::unique_ptr<QTimer> m_playTimer;

  /// max image size for the current stack
  int m_imgWidth, m_imgHeight;

  /// are we picking the CoR, or the first point of the
  /// ROI, etc.
  SelectionState m_selectionState;

  // presenter as in the model-view-presenter
  boost::scoped_ptr<ITomographyROIPresenter> m_presenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYROIVIEWQTWIDGET_H_
