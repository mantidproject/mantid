// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_FACTORY_WIDGETFACTORY_H_
#define MANTID_FACTORY_WIDGETFACTORY_H_

#include "DllOption.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/SliceViewer/SliceViewer.h"
#include "MantidQtWidgets/SliceViewer/SliceViewerWindow.h"
#include <QPointer>
#include <QVector>
#include <Qsci/qscilexer.h>

namespace MantidQt {
namespace SliceViewer {
class SliceViewerWindow;
}
namespace Factory {

/** Factory class that handles the creation
 * of MantidQt widgets such as the SliceViewer.
 * This allows both C++ and Python to create these widgets
 * and both to handle them in a uniform way.

  @date 2011-12-15
*/
class EXPORT_OPT_MANTIDQT_FACTORY WidgetFactory : public QObject {
  Q_OBJECT
public:
  static WidgetFactory *Instance();
  ~WidgetFactory() override;

  MantidQt::SliceViewer::SliceViewerWindow *
  createSliceViewerWindow(const QString &wsName, const QString &label);
  MantidQt::SliceViewer::SliceViewerWindow *
  getSliceViewerWindow(const QString &wsName, const QString &label);
  void closeAllSliceViewerWindows();
  void closeSliceViewerWindow(SliceViewer::SliceViewerWindow *w);

  MantidQt::SliceViewer::SliceViewer *createSliceViewer(const QString &wsName);

private:
  WidgetFactory();

protected:
  /// List of the open SliceViewerWindows
  std::list<QPointer<MantidQt::SliceViewer::SliceViewerWindow>> m_windows;
  /// Singleton instance
  static WidgetFactory *m_pInstance;
};

} // namespace Factory
} // namespace MantidQt

#endif /* MANTID_FACTORY_WIDGETFACTORY_H_ */
