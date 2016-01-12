#ifndef MANTIDGROUPPLOTGENERATOR_H_
#define MANTIDGROUPPLOTGENERATOR_H_

#include "MantidSurfacePlotDialog.h"
#include "MantidMatrix.h"
#include "Graph3D.h"
#include "MantidAPI/NumericAxis.h"

/**
* This utility class generates a surface or contour plot from a group of
* workspaces. 
*/
class MantidGroupPlotGenerator {
public:
  /// Constructor
  explicit MantidGroupPlotGenerator(MantidUI *mantidUI);

  /// Plots a surface from the given workspace group
  void
  plotSurface(const Mantid::API::WorkspaceGroup_const_sptr &wsGroup,
              const MantidSurfacePlotDialog::UserInputSurface &options) const;

  /// Plots a contour plot from the given workspace group
  void
  plotContour(const Mantid::API::WorkspaceGroup_const_sptr &wsGroup,
              const MantidSurfacePlotDialog::UserInputSurface &options) const;

  /// Tests if WorkspaceGroup contains only MatrixWorkspaces
  static bool groupIsAllMatrixWorkspaces(
      const Mantid::API::WorkspaceGroup_const_sptr &wsGroup);

  /// Validates the given options and returns an error string
  static std::string
  validatePlotOptions(MantidSurfacePlotDialog::UserInputSurface &options,
                      int nWorkspaces);

private:
  /// Type of graph to plot
  enum class Type { Surface, Contour };

  /// Plots a graph from the given workspace group
  void plot(Type graphType,
            const Mantid::API::WorkspaceGroup_const_sptr &wsGroup,
            const MantidSurfacePlotDialog::UserInputSurface &options) const;

  /// Creates a single workspace to plot from
  const Mantid::API::MatrixWorkspace_sptr createWorkspaceForGroupPlot(
      boost::shared_ptr<const Mantid::API::WorkspaceGroup> wsGroup,
      const MantidSurfacePlotDialog::UserInputSurface &options,
      QString *xAxisTitle) const;

  /// Returns a single log value from the given workspace
  double
  getSingleLogValue(int wsIndex,
                    const Mantid::API::MatrixWorkspace_const_sptr &matrixWS,
                    const QString &logName) const;

  /// Returns a single log value from supplied custom log
  double getSingleLogValue(int wsIndex, const std::set<double> &values) const;

  /// Converts histogram to point data, if not already
  void convertHistoToPoints(Mantid::API::MatrixWorkspace_sptr ws) const;

  /// Converts point to histogram data, if not already
  void convertPointsToHisto(Mantid::API::MatrixWorkspace_sptr ws) const;

  /// Converts X data to correct (point/histo) format for the graph type
  void convertXData(Mantid::API::MatrixWorkspace_sptr ws, Type graphType) const;

  /// Pointer to the Mantid UI
  MantidUI * const m_mantidUI;
};


#endif
