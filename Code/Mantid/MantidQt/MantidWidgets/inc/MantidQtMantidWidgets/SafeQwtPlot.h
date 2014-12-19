#ifndef MANTID_MANTIDWIDGETS_SAFEQWTPLOT_H_
#define MANTID_MANTIDWIDGETS_SAFEQWTPLOT_H_

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include <qwt_plot.h>
#include <qpainter.h>
#include "qwt_text.h"
#include "MantidAPI/Workspace.h"
#include "WidgetDllOption.h"

namespace MantidQt
{
namespace MantidWidgets
{

  /** A version of QwtPlot that adds a layer of thread safety.
   *
   * Each SafeQwtPlot has a workspace associated with it.
   * Before drawing, it acquires a ReadLock to prevent
   * an algorithm from modifying the underlying workspace while it is
   * drawing.
   *
   * If no workspace is set, no drawing occurs (silently).
    
    @date 2012-01-24

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS SafeQwtPlot : public QwtPlot
  {
    Q_OBJECT

  public:
    explicit SafeQwtPlot(QWidget * parent = NULL);
    explicit SafeQwtPlot(const QwtText &title, QWidget *p = NULL);

    virtual ~SafeQwtPlot();
    
    virtual void drawCanvas(QPainter * painter);

    void setWorkspace(Mantid::API::Workspace_sptr ws);

  private:
    /// Workspace being read-locked
    Mantid::API::Workspace_sptr m_ws;
  };


} // namespace MantidWidgets
} // namespace MantidQt

#endif  /* MANTID_MANTIDWIDGETS_SAFEQWTPLOT_H_ */
