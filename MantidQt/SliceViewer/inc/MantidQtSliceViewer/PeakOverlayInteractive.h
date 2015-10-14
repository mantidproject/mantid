#ifndef MANTID_SLICEVIEWER_PEAKOVERLAYINTERACTIVE_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAYINTERACTIVE_H_

#include "DllOption.h"
#include "MantidQtSliceViewer/PeakOverlayView.h"
#include <QtGui/QWidget>
#include <QtGui/QCursor>

// Forward dec
class QwtPlot;
class QRect;

namespace MantidQt {

namespace MantidWidgets {
// Forward dec
class InputController;
}

namespace SliceViewer {
// Forward dec
class PeaksPresenter;

/** Widget base class for representing peaks. Contains common code used by
 Interactive/Editable peak overlay widgets.

 Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlayInteractive : public QWidget, public PeakOverlayView {
  Q_OBJECT

public:
  /// Constructor
  PeakOverlayInteractive(PeaksPresenter *const peaksPresenter, QwtPlot *plot,
                        const int plotXIndex, const int plotYIndex, QWidget *parent);
  /// Destructor
  virtual ~PeakOverlayInteractive();

  /// Enter peak deletion mode.
  void peakDeletionMode();
  /// Enter peak addition mode
  void peakAdditionMode();
  /// Enter display mode
  void peakDisplayMode();

  QSize sizeHint() const;
  QSize size() const;
  int height() const;
  int width() const;

protected:
  /// Owning presenter
  PeaksPresenter *m_presenter;
  /// QwtPlot containing this
  QwtPlot *m_plot;
  /// Plot x index
  const int m_plotXIndex;
  /// Plot y index
  const int m_plotYIndex;

private:
  /// Input controller.
  MantidQt::MantidWidgets::InputController *m_tool;

  void mousePressEvent(QMouseEvent* e);
  void mouseMoveEvent(QMouseEvent* e);
  void mouseReleaseEvent(QMouseEvent* e);
  void wheelEvent(QWheelEvent* e);
  void keyPressEvent(QKeyEvent* e);
  void enterEvent(QEvent *e);
  void leaveEvent(QEvent *e);

  void paintEvent(QPaintEvent *event);

  // Call do paint on sub-classes
  virtual void doPaintPeaks(QPaintEvent *event) = 0;

  void captureMouseEvents(bool capture);

private slots:

  void erasePeaks(const QRect &rect);
  void addPeakAt(int coordX, int coordY);


};


}
}

#endif //MANTID_SLICEVIEWER_PEAKOVERLAYINTERACTIVE_H_
