// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MD_CURVE_H
#define MANTID_MD_CURVE_H

#include "MantidAPI/IMDWorkspace.h"
#include "MantidCurve.h"
#include "MantidQtWidgets/Plotting/Qwt/MantidQwtIMDWorkspaceData.h"
#include <boost/shared_ptr.hpp>

// Forward definitions
class MantidUI;

/**
    This class is for plotting IMDWorkspaces

    @date 17/11/2011
*/

class MantidMDCurve : public MantidCurve {
  Q_OBJECT
public:
  /// More complex constructor setting some defaults for the curve
  MantidMDCurve(const QString &wsName, Graph *g, bool err = false,
                bool distr = false,
                GraphOptions::CurveType style = GraphOptions::HorizontalSteps);

  /// Copy constructor
  MantidMDCurve(const MantidMDCurve &c);

  ~MantidMDCurve() override;

  MantidMDCurve &operator=(const MantidMDCurve &rhs) = delete;

  MantidMDCurve *clone(const Graph *) const override;

  /// Curve type. Used in the QtiPlot API.
  int rtti() const override { return Rtti_PlotUserItem; }

  /// Used for waterfall plots: updates the data curves with an offset
  // void loadData();

  /// Overrides qwt_plot_curve::setData to make sure only data of
  /// QwtWorkspaceSpectrumData type can  be set
  void setData(const QwtData &data);

  /// Overrides qwt_plot_curve::boundingRect
  QwtDoubleRect boundingRect() const override;

  /// Return pointer to the data if it of the right type or 0 otherwise
  MantidQwtIMDWorkspaceData *mantidData() override;

  /// Return pointer to the data if it of the right type or 0 otherwise, const
  /// version
  const MantidQwtIMDWorkspaceData *mantidData() const override;

  /// Enables/disables drawing of error bars
  void setErrorBars(bool yes = true, bool drawAll = false) {
    m_drawErrorBars = yes;
    m_drawAllErrorBars = drawAll;
  }

  void draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QRect &) const override;

  /// saves the MantidMatrixCurve details to project file.
  QString saveToString();

  /// The workspace name
  QString workspaceName() const { return m_wsName; }

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

  /// Handle an ADS clear notification
  void clearADSHandle() override { emit removeMe(this); }

signals:

  void resetData(const QString &);

private slots:

  void dataReset(const QString &);

private:
  QString
      m_wsName; ///< Workspace name. If empty the ws isn't in the data service
};

#endif // MANTID_CURVE_H
