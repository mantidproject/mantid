#ifndef MANTIDGROUPPLOTGENERATOR_H_
#define MANTIDGROUPPLOTGENERATOR_H_

#include "Graph3D.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidMatrix.h"
#include <MantidQtMantidWidgets/MantidSurfacePlotDialog.h>

/**
* This utility class generates a surface or contour plot from a group of
* workspaces.
*/
class MantidGroupPlotGenerator {
public:
  /// Constructor
  explicit MantidGroupPlotGenerator(
      MantidQt::MantidWidgets::MantidDisplayBase *mantidUI);

  /// Plots a surface from the given workspace group
  void plotSurface(
      const Mantid::API::WorkspaceGroup_const_sptr &wsGroup,
      const MantidQt::MantidWidgets::MantidSurfacePlotDialog::UserInputSurface &
          options) const;

  /// Plots a contour plot from the given workspace group
  void plotContour(
      const Mantid::API::WorkspaceGroup_const_sptr &wsGroup,
      const MantidQt::MantidWidgets::MantidSurfacePlotDialog::UserInputSurface &
          options) const;

  /// Tests if WorkspaceGroup contains only MatrixWorkspaces
  static bool groupIsAllMatrixWorkspaces(
      const Mantid::API::WorkspaceGroup_const_sptr &wsGroup);

  /// Validates the given options and returns an error string
  static std::string validatePlotOptions(
      MantidQt::MantidWidgets::MantidSurfacePlotDialog::UserInputSurface &
          options,
      int nWorkspaces);

  /// Tests if WorkspaceGroup contents all have same X for given spectrum
  static bool
  groupContentsHaveSameX(const Mantid::API::WorkspaceGroup_const_sptr &wsGroup,
                         const size_t index);

private:
  /// Type of graph to plot
  enum class Type { Surface, Contour };

  /// Plots a graph from the given workspace group
  void plot(
      Type graphType, const Mantid::API::WorkspaceGroup_const_sptr &wsGroup,
      const MantidQt::MantidWidgets::MantidSurfacePlotDialog::UserInputSurface &
          options) const;

  /// Creates a single workspace to plot from
  const Mantid::API::MatrixWorkspace_sptr createWorkspaceForGroupPlot(
      Type graphType,
      boost::shared_ptr<const Mantid::API::WorkspaceGroup> wsGroup,
      const MantidQt::MantidWidgets::MantidSurfacePlotDialog::UserInputSurface &
          options) const;

  /// Returns a single log value from the given workspace
  double
  getSingleLogValue(int wsIndex,
                    const Mantid::API::MatrixWorkspace_const_sptr &matrixWS,
                    const QString &logName) const;

  /// Returns a single log value from supplied custom log
  double getSingleLogValue(int wsIndex, const std::set<double> &values) const;

  /// Get X axis title
  QString getXAxisTitle(
      const boost::shared_ptr<const Mantid::API::WorkspaceGroup> wsGroup) const;

  /// Validate chosen workspaces/spectra
  void validateWorkspaceChoices(
      const boost::shared_ptr<const Mantid::API::WorkspaceGroup> wsGroup,
      const size_t spectrum) const;

  /// Pointer to the Mantid UI
  MantidQt::MantidWidgets::MantidDisplayBase *const m_mantidUI;
};

#endif
