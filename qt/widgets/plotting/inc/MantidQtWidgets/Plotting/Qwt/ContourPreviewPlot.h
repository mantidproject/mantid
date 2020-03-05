// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_CONTOURPREVIEWPLOT_H_
#define MANTID_MANTIDWIDGETS_CONTOURPREVIEWPLOT_H_

#include "ui_ContourPreviewPlot.h"

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidQtWidgets/Common/MdSettings.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <qwt_plot_spectrogram.h>

#include <QSettings>
#include <QWidget>

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
class ColorBarWidget;
class SafeQwtPlot;

using MWDimension_sptr = boost::shared_ptr<Mantid::API::MWDimension>;
using MWDimension_const_sptr =
    boost::shared_ptr<Mantid::API::MWDimension const>;
using DimensionRange = std::pair<Mantid::coord_t, Mantid::coord_t>;

class EXPORT_OPT_MANTIDQT_PLOTTING ContourPreviewPlot
    : public QWidget,
      public MantidQt::API::WorkspaceObserver {
  Q_OBJECT

public:
  ContourPreviewPlot(QWidget *parent = nullptr);
  ~ContourPreviewPlot() override;

  Mantid::API::MatrixWorkspace_sptr getActiveWorkspace() const;
  void setWorkspace(Mantid::API::MatrixWorkspace_sptr const workspace);
  SafeQwtPlot *getPlot2D();

  void setPlotVisible(bool const &visible);

  bool isPlotVisible() const;

  void setXAxisLabel(QString const &label);
  void setYAxisLabel(QString const &label);

  std::tuple<double, double> getAxisRange(AxisID axisID) const;

protected:
  void preDeleteHandle(
      std::string const &workspaceName,
      boost::shared_ptr<Mantid::API::Workspace> const workspace) override;

private slots:
  void handleSetTransparentZeros(bool const &transparent);

private:
  void setupPlot();
  void loadSettings();
  void saveSettings();

  void updateDisplay();

  void checkRangeLimits() const;
  void checkForInfiniteLimits(DimensionRange const &range,
                              std::size_t const &index,
                              std::ostringstream &message) const;
  DimensionRange dimensionRange(std::size_t const &index) const;
  Mantid::coord_t dimensionMinimum(std::size_t const &index) const;
  Mantid::coord_t dimensionMaximum(std::size_t const &index) const;
  void findFullRange();

  void setVectorDimensions();
  void clearPlot();

  Ui::ContourPreviewPlot m_uiForm;
  /// Spectrogram plot of ContourPreviewPlot
  std::unique_ptr<QwtPlotSpectrogram> m_spectrogram;
  /// Data presenter
  std::unique_ptr<API::QwtRasterDataMD> m_data;
  /// Md Settings for colour maps
  boost::shared_ptr<MantidQt::API::MdSettings> m_mdSettings;
  /// Workspace being shown
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  /// The calculated range of values in the full data set
  QwtDoubleInterval m_colourRangeFull;
  Mantid::API::MDNormalization m_normalization;
  /// Vector of the dimensions to show.
  std::vector<Mantid::Geometry::MDHistoDimension_sptr> m_dimensions;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_CONTOURPREVIEWPLOT_H_ */
