#ifndef  SPECTRUM_VIEW_H
#define  SPECTRUM_VIEW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include <QtGui>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidQtSpectrumViewer/GraphDisplay.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/DllOptionSV.h"

/**
    @class SpectrumView

    This is the QMainWindow for the SpectrumView data viewer.  Data is
    displayed in an SpectrumView, by constructing the SpectrumView object and
    specifying a particular data source.

    @author Dennis Mikkelson
    @date   2012-04-03

    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories

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

    Code Documentation is available at
                 <http://doxygen.mantidproject.org>
 */

namespace Ui
{
class SpectrumViewer; // Forward declaration of UI file
}

namespace MantidQt
{
namespace SpectrumView
{

// Forward declarations
class EModeHandler;
class RangeHandler;
class SliderHandler;
class SpectrumDisplay;
class SVConnections;
class MatrixWSDataSource;

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER SpectrumView : public QMainWindow, public MantidQt::API::WorkspaceObserver
{
  Q_OBJECT

public:
  /// Construct a SpectrumView to display data from the specified data source
  SpectrumView( QWidget * parent = 0 );

  ~SpectrumView();
  void renderWorkspace(Mantid::API::MatrixWorkspace_const_sptr wksp);

protected slots:
  void closeWindow();
  void updateWorkspace();

protected:
  virtual void resizeEvent(QResizeEvent * event);
  void preDeleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);
  void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);

private:
  void init(SpectrumDataSource* dataSource);
  void updateHandlers(SpectrumDataSource* dataSource);

  GraphDisplay*    m_hGraph;
  GraphDisplay*    m_vGraph;

  MatrixWSDataSource *m_dataSource;

  /* Keep void pointers to the following objects, to avoid having to */
  /* include ui_SpectrumView.h, which disappears by the time MantidPlot is */
  /* being built.  We need the pointers so we can delete them in the */
  /* destructor. */
  Ui::SpectrumViewer *m_ui;              // Ui_SpectrumViewer*
  SliderHandler      *m_sliderHandler;   // SliderHandler*
  RangeHandler       *m_rangeHandler;    // RangeHandler*
  SpectrumDisplay    *m_spectrumDisplay; // SpectrumDisplay*
  SVConnections      *m_svConnections;   // SVConnections*
  EModeHandler       *m_emodeHandler;    // EModeHandler*

signals:
  void needToClose();
  void needToUpdate();
};

} // namespace SpectrumView
} // namespace MantidQt

#endif   // SPECTRUM_VIEW_H
