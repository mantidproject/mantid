#ifndef MANTID_MANTIDWIDGETS_MWVIEW_H_
#define MANTID_MANTIDWIDGETS_MWVIEW_H_

// includes for interface development
#include "DllOption.h"
#include "MantidQtWidgets/Common/MdSettings.h"
#include "ui_MWView.h"
#include <QWidget>
#include <qwt_plot_spectrogram.h>
// includes for workspace handling
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"

namespace Mantid {
namespace API {
class MWDimension;
}
} // namespace Mantid

namespace MantidQt {

namespace API {
class QwtRasterDataMD;
class MdSettings;
} // namespace API

namespace MantidWidgets {
// forward declarations
class ColorBarWidget;
class SafeQwtPlot;

using MWDimension_sptr = boost::shared_ptr<Mantid::API::MWDimension>;
using MWDimension_const_sptr =
    boost::shared_ptr<const Mantid::API::MWDimension>;

/** A 2D viewer for a Matrix Workspace.
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
class EXPORT_OPT_MANTIDQT_LEGACYQWT MWView
    : public QWidget,
      public MantidQt::API::WorkspaceObserver {
  Q_OBJECT

public:
  MWView(QWidget *parent = nullptr);
  ~MWView() override;
  void loadColorMap(QString filename = QString());
  void setWorkspace(Mantid::API::MatrixWorkspace_sptr ws);
  void updateDisplay();
  SafeQwtPlot *getPlot2D();

public slots:
  void colorRangeChangedSlot();
  void loadColorMapSlot();
  void setTransparentZerosSlot(bool transparent);

protected:
  void preDeleteHandle(
      const std::string &workspaceName,
      const boost::shared_ptr<Mantid::API::Workspace> workspace) override;

private:
  void initLayout();
  void loadSettings();
  void saveSettings();
  void checkRangeLimits();
  void findRangeFull();
  void setVectorDimensions();
  void spawnWellcomeWorkspace();
  void showWellcomeWorkspace();

  Ui::MWView m_uiForm;
  /// Spectrogram plot of MWView
  QwtPlotSpectrogram *m_spect;
  /// Data presenter
  API::QwtRasterDataMD *m_data;
  /// File of the last loaded color map.
  QString m_currentColorMapFile;
  /// Md Settings for color maps
  boost::shared_ptr<MantidQt::API::MdSettings> m_mdSettings;
  /// Workspace being shown
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  /// Default workspace shown if no data is loaded
  Mantid::API::MatrixWorkspace_sptr m_wellcomeWorkspace;
  /// Name of default workspace
  const std::string m_wellcomeName;
  /// The calculated range of values in the FULL data set
  QwtDoubleInterval m_colorRangeFull;
  Mantid::API::MDNormalization m_normalization;
  /// Vector of the dimensions to show.
  std::vector<Mantid::Geometry::MDHistoDimension_sptr> m_dimensions;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_MWVIEW_H_ */
