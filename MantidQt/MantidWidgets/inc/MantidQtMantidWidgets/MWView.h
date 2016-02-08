#ifndef MANTID_MANTIDWIDGETS_MWVIEW_H_
#define MANTID_MANTIDWIDGETS_MWVIEW_H_

// includes for interface development
#include <QWidget>
#include <qwt_plot_spectrogram.h>
#include "ui_MWView.h"
#include "MantidQtAPI/MdSettings.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"
// includes for workspace handling
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/IMDWorkspace.h"

namespace Mantid{
namespace API{
class MWDimension;
}
}

namespace MantidQt {

namespace API{
class QwtRasterDataMD;
class MdSettings;
}

namespace MantidWidgets {
class ColorBarWidget;

/** A viewer for a Matrix Workspace.
 *
 * Before drawing, it acquires a ReadLock to prevent
 * an algorithm from modifying the underlying workspace while it is
 * drawing.
 *
 * If no workspace is set, no drawing occurs (silently).

  @date 2016-02-05

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MWView : public QWidget {
  Q_OBJECT

public:
  MWView(QWidget *parent = 0);
  ~MWView();
  void loadColorMap(QString filename = QString() );
  void setWorkspace(Mantid::API::MatrixWorkspace_sptr ws);

signals:
  /// Signal emitted when someone uses setWorkspace() on MWView
  void workspaceChanged();

public slots:
  void colorRangeChangedSlot();
  void loadColorMapSlot();
  void setTransparentZerosSlot(bool transparent);

private:
  void initLayout();
  void loadSettings();
  void saveSettings();
  void updateDisplay();
  void checkRangeLimits();

  Ui::MWView m_uiForm;
  /// Spectrogram plot of MWView
  QwtPlotSpectrogram *m_spect;
  /// Data presenter
  API::QwtRasterDataMD *m_data;
  /// File of the last loaded color map.
  QString m_currentColorMapFile;
  /// Md Settings for color maps
  boost::shared_ptr<MantidQt::API::MdSettings>  m_mdSettings;
  /// Workspace being shown
  Mantid::API::MatrixWorkspace_sptr m_workspace;

};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MWVIEW_H_ */
