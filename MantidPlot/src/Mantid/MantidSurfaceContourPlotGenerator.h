// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDSURFACECONTOURPLOTGENERATOR_H_
#define MANTIDSURFACECONTOURPLOTGENERATOR_H_

#include "Graph3D.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidMatrix.h"
#include "MantidQtWidgets/Common/MantidDisplayBase.h"
#include "MantidQtWidgets/Common/MantidWSIndexDialog.h"

/**
 * This utility class generates a surface or contour plot from a group of
 * workspaces.
 */
class MantidSurfaceContourPlotGenerator {
public:
  /// Constructor
  explicit MantidSurfaceContourPlotGenerator(
      MantidQt::MantidWidgets::MantidDisplayBase *mantidUI);

  /// Plots a surface from the given workspace group
  void plotSurface(bool accepted, int plotIndex, const QString &axisName,
                   const QString &logName,
                   const std::set<double> &customLogValues,
                   const std::vector<Mantid::API::MatrixWorkspace_const_sptr>
                       &workspaces) const;

  /// Plots a contour plot from the given workspace group
  void plotContour(bool accepted, int plotIndex, const QString &axisName,
                   const QString &logName,
                   const std::set<double> &customLogValues,
                   const std::vector<Mantid::API::MatrixWorkspace_const_sptr>
                       &workspaces) const;

  /// Validates the given options and returns an error string
  static std::string validatePlotOptions(
      MantidQt::MantidWidgets::MantidWSIndexWidget::UserInputAdvanced &options,
      int nWorkspaces);

  /// Tests if WorkspaceGroup contents all have same X for given spectrum
  static bool groupContentsHaveSameX(
      const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces,
      const size_t index);

private:
  /// Type of graph to plot
  enum class Type { Surface, Contour };

  /// Plots a graph from the given workspace group
  void
  plot(Type graphType, bool accepted, int plotIndex, const QString &axisName,
       const QString &logName, const std::set<double> &customLogValues,
       const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces)
      const;

  /// Creates a single workspace to plot from
  const Mantid::API::MatrixWorkspace_sptr createWorkspaceForGroupPlot(
      Type graphType,
      const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces,
      int plotIndex, const QString &logName,
      const std::set<double> &customLogValues) const;

  /// Returns a single log value from the given workspace
  double
  getSingleLogValue(size_t wsIndex,
                    const Mantid::API::MatrixWorkspace_const_sptr &matrixWS,
                    const QString &logName) const;

  /// Returns a single log value from supplied custom log
  double getSingleLogValue(size_t wsIndex,
                           const std::set<double> &values) const;

  /// Get X axis title
  QString getXAxisTitle(
      const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces)
      const;

  /// Validate chosen workspaces/spectra
  void validateWorkspaceChoices(
      const std::vector<Mantid::API::MatrixWorkspace_const_sptr> &workspaces,
      const size_t spectrum) const;

  /// Pointer to the Mantid UI
  MantidQt::MantidWidgets::MantidDisplayBase *const m_mantidUI;
};

#endif
