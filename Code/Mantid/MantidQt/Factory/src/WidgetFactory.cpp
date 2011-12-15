#include "MantidQtFactory/WidgetFactory.h"
#include "MantidKernel/System.h"
#include "MantidQtSliceViewer/SliceViewerWindow.h"

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
    return window;
  }


} // namespace Mantid
} // namespace Factory
