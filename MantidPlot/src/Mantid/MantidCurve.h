// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDPLOT_MANTIDCURVE_H
#define MANTIDPLOT_MANTIDCURVE_H

#include "../PlotCurve.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include "MantidQtWidgets/Plotting/Qwt/MantidQwtWorkspaceData.h"

class Graph;
class ErrorBarSettings;

/** Base class for MantidCurve types.
 */
class MantidCurve : public PlotCurve, public MantidQt::API::WorkspaceObserver {
  Q_OBJECT
public:
  /// Constructor
  MantidCurve(const QString &wsName, bool error, bool allerror = false);
  /// Default constructor
  explicit MantidCurve(bool err);
  /// Destructor
  ~MantidCurve() override;
  /// Clone
  MantidCurve *clone(const Graph *g) const override = 0;
  /// Get mantid data
  virtual const MantidQwtWorkspaceData *mantidData() const = 0;
  /// Get mantid data
  virtual MantidQwtWorkspaceData *mantidData() = 0;
  /// Overridden virtual method
  void itemChanged() override;

  /// Returns whether the curve has error bars
  bool hasErrorBars() const { return m_drawErrorBars; }

  /// Returns the error bar settings for this curve (a MantidCurve has only one
  /// set of error bars)
  QList<ErrorBarSettings *> errorBarSettingsList() const override;

  /// Invalidates the bounding rect forcing it to be recalculated
  void invalidateBoundingRect();

  /*-------------------------------------------------------------------------------------
  Public Base/Common methods
  -------------------------------------------------------------------------------------*/

  QwtDoubleRect boundingRect() const override;

  /*-------------------------------------------------------------------------------------
  End Public Base/Common methods
  -------------------------------------------------------------------------------------*/

protected slots:

  void axisScaleChanged(int axis, bool toLog);

protected:
  /*-------------------------------------------------------------------------------------
  Protected Base/Common methods
  -------------------------------------------------------------------------------------*/

  /// Apply the style choice
  void applyStyleChoice(GraphOptions::CurveType style, MultiLayer *ml,
                        int &lineWidth);

  /// Make a name for a copied curve
  static QString createCopyName(const QString &curveName);

  /// Draw the curve.
  void doDraw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap,
              const QRect &rect,
              MantidQwtWorkspaceData const *const data) const;

  /*-------------------------------------------------------------------------------------
  End Protected Base/Common methods
  -------------------------------------------------------------------------------------*/
  bool m_drawErrorBars;    /// Flag indicating that error bars should be drawn.
  bool m_drawAllErrorBars; ///< if true and m_drawErrorBars is true draw all
  /// error bars (no skipping)

  // The error bar settings for this curve. Owned by this class.
  ErrorBarSettings *m_errorSettings;

private:
  /// The bounding rect used by qwt to set the axes
  mutable QwtDoubleRect m_boundingRect;

  // To ensure that all MantidCurves can work with Mantid Workspaces.
  virtual void init(Graph *g, bool distr, GraphOptions::CurveType style,
                    bool multileSpectra = false) = 0;
};

#endif
