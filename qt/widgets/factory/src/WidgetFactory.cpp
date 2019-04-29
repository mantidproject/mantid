// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Factory/WidgetFactory.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/SliceViewer/SliceViewer.h"
#include "MantidQtWidgets/SliceViewer/SliceViewerWindow.h"

using namespace MantidQt::SliceViewer;

namespace MantidQt {
namespace Factory {

/// Initialize the instance to NULL
WidgetFactory *WidgetFactory::m_pInstance = nullptr;

//----------------------------------------------------------------------------------------------
/** Private constructor. This is not accessible,
 * use the Instance() method to access the singleton instance
 * instead.
 */
WidgetFactory::WidgetFactory() {
  //    std::cout << "WidgetFactory constructor called\n";
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
WidgetFactory::~WidgetFactory() {
  //    std::cout << "WidgetFactory destructor called\n";
  //    for (size_t i=0; i<m_windows.size(); i++)
  //      delete m_windows[i];
  //    m_windows.clear();
}

//----------------------------------------------------------------------------------------------
/** Retrieve the singleton instance of WidgetFactory */
WidgetFactory *WidgetFactory::Instance() {
  if (!m_pInstance)
    m_pInstance = new WidgetFactory;

  return m_pInstance;
}

//----------------------------------------------------------------------------------------------
/** Create an instance of a SliceViewerWindow:
 * a separate window containing a SliceViewer widget, to do 2D
 * views of multidimensional workspace, as well as a
 * LineViewer widget, to do 1D lines through the 2D slices.
 *
 * @param wsName :: name of the workspace to show
 * @param label :: label for the window title
 * @return the created SliceViewerWindow *
 */
MantidQt::SliceViewer::SliceViewerWindow *
WidgetFactory::createSliceViewerWindow(const QString &wsName,
                                       const QString &label) {
  SliceViewerWindow *window = new SliceViewerWindow(wsName, label);
  QPointer<MantidQt::SliceViewer::SliceViewerWindow> pWindow(window);

  // Save in a list for later use
  m_windows.push_back(pWindow);

  return window;
}

//----------------------------------------------------------------------------------------------
/** Returns a previously-open instance of a SliceViewerWindow.
 *
 * @param wsName :: name of the workspace that was open
 * @param label :: label for the window title
 * @return the previously-created SliceViewerWindow *
 * @throw std::runtime_error if no open windows match the parameters
 */
MantidQt::SliceViewer::SliceViewerWindow *
WidgetFactory::getSliceViewerWindow(const QString &wsName,
                                    const QString &label) {
  for (auto window : m_windows) {
    if (window) {
      // Match the ws name and the label
      if ((window->getSlicer()->getWorkspace()->getName() ==
           wsName.toStdString()) &&
          (window->getLabel() == label))
        return window;
    }
  }
  throw std::runtime_error("No SliceViewer is open with the workspace '" +
                           wsName.toStdString() + "' and label '" +
                           label.toStdString() + "'.");
}

//----------------------------------------------------------------------------------------------
/** Closes every previously-open instance of a SliceViewerWindow.
 */
void WidgetFactory::closeAllSliceViewerWindows() {
  for (auto window : m_windows) {
    if (window)
      // Close with delete
      window->close();
  }
  m_windows.clear();
}

/**
 * Closes one instance
 */
void WidgetFactory::closeSliceViewerWindow(SliceViewerWindow *w) {
  if (w) {
    w->close();
    m_windows.remove(w);
  }
}

//----------------------------------------------------------------------------------------------
/** Create an instance of a bare SliceViewer Widget.
 * This is only capable of doing 2D views, and cannot do line plots
 * since it does not have a LineViewer.
 *
 * Use WidgetFactory::createSliceViewerWindow to create a window combining both.
 *
 * @param wsName :: name of the workspace to show. Optional, blank for no
 *workspace.
 * @return the created SliceViewer *
 */
MantidQt::SliceViewer::SliceViewer *
WidgetFactory::createSliceViewer(const QString &wsName) {
  MantidQt::SliceViewer::SliceViewer *slicer =
      new MantidQt::SliceViewer::SliceViewer();
  // TODO: Save in a list ?
  if (!wsName.isEmpty())
    slicer->setWorkspace(wsName);
  return slicer;
}

} // namespace Factory
} // namespace MantidQt
