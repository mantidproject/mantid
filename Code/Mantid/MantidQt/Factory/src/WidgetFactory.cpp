#include "MantidQtFactory/WidgetFactory.h"
#include "MantidKernel/System.h"
#include "MantidQtSliceViewer/SliceViewerWindow.h"
#include "MantidQtSliceViewer/SliceViewer.h"

using namespace MantidQt::SliceViewer;

namespace MantidQt
{
namespace Factory
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  WidgetFactoryImpl::WidgetFactoryImpl()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  WidgetFactoryImpl::~WidgetFactoryImpl()
  {
    for (size_t i=0; i<m_windows.size(); i++)
      delete m_windows[i];
    m_windows.clear();
  }


  //----------------------------------------------------------------------------------------------
  /** Create an instance of a SliceViewerWindow:
   * a separate window containing a SliceViewer widget, to do 2D
   * views of multidimensional workspace, as well as a
   * LineViewer widget, to do 1D lines through the 2D slices.
   *
   * @param wsName :: name of the workspace to show
   * @param app :: QApplication parent
   * @param label :: label for the window title
   * @return the created SliceViewerWindow *
   */
  MantidQt::SliceViewer::SliceViewerWindow * WidgetFactoryImpl::createSliceViewerWindow(const QString& wsName, const QString& label)
  {
    SliceViewerWindow * window = new SliceViewerWindow(wsName, label);
    //TODO: Save in a list
    m_windows.push_back(window);
    return window;
  }

  //----------------------------------------------------------------------------------------------
  /** Create an instance of a bare SliceViewer Widget.
   * This is only capable of doing 2D views, and cannot do line plots
   * since it does not have a LineViewer.
   *
   * Use WidgetFactory::createSliceViewerWindow to create a window combining both.
   *
   * @param wsName :: name of the workspace to show. Optional, blank for no workspace.
   * @return the created SliceViewer *
   */
  MantidQt::SliceViewer::SliceViewer* WidgetFactoryImpl::createSliceViewer(const QString& wsName)
  {
    MantidQt::SliceViewer::SliceViewer * slicer = new MantidQt::SliceViewer::SliceViewer();
    //TODO: Save in a list
    if (!wsName.isEmpty())
      slicer->setWorkspace(wsName);
    return slicer;
  }



} // namespace Mantid
} // namespace Factory
