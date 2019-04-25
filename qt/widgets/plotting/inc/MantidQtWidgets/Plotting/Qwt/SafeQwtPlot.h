// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_SAFEQWTPLOT_H_
#define MANTID_MANTIDWIDGETS_SAFEQWTPLOT_H_

#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "qwt_text.h"
#include <QPainter>
#include <qwt_plot.h>

namespace MantidQt {
namespace MantidWidgets {

/** A version of QwtPlot that adds a layer of thread safety.
 *
 * Each SafeQwtPlot has a workspace associated with it.
 * Before drawing, it acquires a ReadLock to prevent
 * an algorithm from modifying the underlying workspace while it is
 * drawing.
 *
 * If no workspace is set, no drawing occurs (silently).

  @date 2012-01-24
*/
class EXPORT_OPT_MANTIDQT_PLOTTING SafeQwtPlot : public QwtPlot {
  Q_OBJECT

public:
  explicit SafeQwtPlot(QWidget *parent = nullptr);
  explicit SafeQwtPlot(const QwtText &title, QWidget *p = nullptr);

  ~SafeQwtPlot() override;

  void drawCanvas(QPainter *painter) override;

  void setWorkspace(Mantid::API::Workspace_sptr ws);

private:
  /// Workspace being read-locked
  Mantid::API::Workspace_sptr m_ws;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_SAFEQWTPLOT_H_ */
