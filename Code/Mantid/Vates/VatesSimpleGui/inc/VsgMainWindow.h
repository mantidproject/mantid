#ifndef VSGMAINWINDOW_H_
#define VSGMAINWINDOW_H_

#include <QtGui/QMainWindow>
#include <QPointer>
#include "ui_VsgMainWindow.h"

class ViewBase;

class pqPipelineSource;

class QHBoxLayout;
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
class VsgMainWindow : public QMainWindow, public Ui::mpMainWindow
{
    Q_OBJECT

public:
    /**
     * Default constructor.
     * @param parent the parent widget for the main window
     */
    VsgMainWindow(QWidget *parent = 0);
    /// Default destructor.
    virtual ~VsgMainWindow();

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
  /// Signal to enable the threeslice view mode button.
  void enableThreeSliceViewButton();
  /// Signal to enable the multislice view mode button.
  void enableMultiSliceViewButton();

private:
    Q_DISABLE_COPY(VsgMainWindow);
    ViewBase *currentView; ///< Holder for the current view
    ViewBase *hiddenView; ///< Holder for the view that is being switched from
    QPointer<pqPipelineSource> originSource; ///< Holder for the current source
    QHBoxLayout *viewLayout; ///< Layout manager for the view widget

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
    ViewBase *setMainViewWidget(QWidget *container, ModeControlWidget::Views v);
    /// Helper function to swap current and hidden view pointers.
    void swapViews();
};

#endif // VSGMAINWINDOW_H_
