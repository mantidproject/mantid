#ifndef MPMAINWINDOW_H
#define MPMAINWINDOW_H

#include <QtGui/QMainWindow>
#include "ui_MpMainWindow.h"
#include <QPointer>

class IView;

class pqPipelineSource;
/**
 *
  This class represents the main level program.

  @author Michael Reuter
  @date 24/05/2011

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class mpMainWindow : public QMainWindow, public Ui::mpMainWindow
{
    Q_OBJECT

public:
    /**
     * Default constructor.
     * @param parent the parent widget for the main window
     */
    mpMainWindow(QWidget *parent = 0);
    /// Default destructor.
    virtual ~mpMainWindow();

protected slots:
    /**
     * Load and render data from the given source.
     * @param source a ParaView compatible source
     */
    void onDataLoaded(pqPipelineSource *source);
    /**
     * Execute the logic for switching views on the main level window.
     * @param v the view mode to switch to
     */
    void switchViews(ModeControlWidget::Views v);

signals:
  /// Signal to disable all view modes but standard.
	void disableViewModes();
	/// Signal to enable the other view mode buttons.
	void enableModeButtons();

private:
    Q_DISABLE_COPY(mpMainWindow);
    QPointer<pqPipelineSource> originSource; ///< Holder for the current source
    IView *currentView; ///< Holder for the current view
    IView *hiddenView; ///< Holder for the view that is being switched from

    /// Disable communication with the proxy tab widget.
    void removeProxyTabWidgetConnections();
    /// Set the signals/slots for the main program components based on the view.
    void setMainWindowComponentsForView();
    /**
     * Create the requested view on the main window.
     * @param container the UI widget to associate the view mode with
     * @param v the view mode to set on the main window
     * @return the requested view
     */
    IView *setMainViewWidget(QWidget *container, ModeControlWidget::Views v);
    /// Helper function to swap current and hidden view pointers.
    void swapViews();
};

#endif // MPMAINWINDOW_H
