#ifndef MANTID_MATRIX_CURVE_H
#define MANTID_MATRIX_CURVE_H

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurve.h"
#include "MantidKernel/Unit.h"
#include <boost/shared_ptr.hpp>

// Forward definitions
class MantidUI;

/**
    This class is for plotting spectra or bins from a Mantid MatrixWorkspace in
   a
    QtiPlot's Graph widget.

    @author Roman Tolchenov, Tessella plc
    @date 09/09/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MantidMatrixCurve : public MantidCurve {
  Q_OBJECT
public:
  /// Indicates whether the curve index is treated as a row or a column
  enum IndexDir {
    Spectrum, // index is treated as a row
    Bin       // index is treated as a column
  };

  /// More complex constructor setting some defaults for the curve
  MantidMatrixCurve(const QString &name, const QString &wsName, Graph *g,
                    int index, IndexDir indexType, bool err = false,
                    bool distr = false,
                    GraphOptions::CurveType style = GraphOptions::Unspecified,
                    bool multipleSpectra = false);

  /// More complex constructor setting some defaults for the curve
  MantidMatrixCurve(const QString &wsName, Graph *g, int index,
                    IndexDir indexType, bool err = false, bool distr = false,
                    GraphOptions::CurveType style = GraphOptions::Unspecified);

  /// Copy constructor
  MantidMatrixCurve(const MantidMatrixCurve &c);

  ~MantidMatrixCurve() override;

  MantidMatrixCurve *clone(const Graph *) const override;

  /// Curve type. Used in the QtiPlot API.
  int rtti() const override { return Rtti_PlotUserItem; }

  /// Used for waterfall plots: updates the data curves with an offset
  void loadData();

  /// Overrides qwt_plot_curve::setData to make sure only data of
  /// QwtWorkspaceSpectrumData type can  be set
  void setData(const QwtData &data);

  /// Overrides qwt_plot_curve::boundingRect
  QwtDoubleRect boundingRect() const override;

  /// Return pointer to the data if it of the right type or 0 otherwise
  MantidQwtMatrixWorkspaceData *mantidData() override;
  /// Return pointer to the data if it of the right type or 0 otherwise, const
  /// version
  const MantidQwtMatrixWorkspaceData *mantidData() const override;

  /// Enables/disables drawing of error bars
  void setErrorBars(bool yes = true, bool drawAll = false) {
    m_drawErrorBars = yes;
    m_drawAllErrorBars = drawAll;
  }

  /// Enables/disables drawing as distribution, ie dividing each y-value by the
  /// bin width.
  bool setDrawAsDistribution(bool on = true);

  /// Returns whether the curve is plotted as a distribution
  bool isDistribution() const;

  /// Returns true if the curve data comes for a histgoram workspace
  bool isHistogramData() const;

  /// Returns whether the can be normalized, i.e whether the workspace data is
  /// already divided by the width
  bool isNormalizable() const;

  void draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QRect &) const override;

  /// Overriden virtual method
  void itemChanged() override;

  /// saves the MantidMatrixCurve details to project file.
  QString saveToString();

  /// The workspace name
  QString workspaceName() const { return m_wsName; }
  /// Returns the workspace index if a spectrum is plotted and -1 if it is a
  /// bin.
  int workspaceIndex() const;
  /// Return the x units
  Mantid::Kernel::Unit_sptr xUnits() const { return m_xUnits; }
  /// Return the y units
  Mantid::Kernel::Unit_sptr yUnits() const { return m_yUnits; }

private:
  using PlotCurve::draw; // Avoid Intel compiler warning

  /// Init the curve
  void init(Graph *g, bool distr, GraphOptions::CurveType style,
            bool multipleSpectra = false) override;

  /// Handles delete notification
  void postDeleteHandle(const std::string &wsName) override {
    if (wsName == m_wsName.toStdString()) {
      observePostDelete(false);
      emit removeMe(this);
    }
  }
  /// Handles afterReplace notification
  void afterReplaceHandle(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> ws) override;

  /// Handle an ADS clear notificiation
  void clearADSHandle() override { emit removeMe(this); }

signals:

  void resetData(const QString &);

private slots:

  void dataReset(const QString &);

private:
  /// Make the curve name
  QString createCurveName(
      const QString &prefix,
      const boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws);

  QString
      m_wsName; ///< Workspace name. If empty the ws isn't in the data service
  /// index
  int m_index;
  /// Is the index a spectrum or bin index
  IndexDir m_indexType;
  /// x units
  Mantid::Kernel::Unit_sptr m_xUnits;
  /// y units
  Mantid::Kernel::Unit_sptr m_yUnits;
};

#endif // MANTID_CURVE_H
