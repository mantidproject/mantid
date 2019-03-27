// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTAPI_MANTIDQWTIMDWORKSPACEDATA_H
#define MANTIDQTAPI_MANTIDQWTIMDWORKSPACEDATA_H

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/VMD.h"
#include "MantidQtWidgets/LegacyQwt/DllOption.h"
#include "MantidQtWidgets/LegacyQwt/MantidQwtWorkspaceData.h"
#include <boost/weak_ptr.hpp>

/** This class is used to plot MDWorkspace line plots.
 * For example, in the LineViewer and via the "Plot MD" menu on workspaces.
 *
 * It implements the QwtData interface.
 *
 */
class EXPORT_OPT_MANTIDQT_LEGACYQWT MantidQwtIMDWorkspaceData
    : public MantidQwtWorkspaceData {
public:
  /// For PlotAxisChoice, auto-determine it
  static const int PlotAuto = -2;
  /// For PlotAxisChoice, distance from start of line
  static const int PlotDistance = -1;

  MantidQwtIMDWorkspaceData(
      Mantid::API::IMDWorkspace_const_sptr workspace, const bool logScale,
      Mantid::Kernel::VMD start = Mantid::Kernel::VMD(),
      Mantid::Kernel::VMD end = Mantid::Kernel::VMD(),
      Mantid::API::MDNormalization normalize = Mantid::API::NoNormalization,
      bool isDistribution = false);

  MantidQwtIMDWorkspaceData(const MantidQwtIMDWorkspaceData &data);
  MantidQwtIMDWorkspaceData &operator=(const MantidQwtIMDWorkspaceData & /*data*/);
  ~MantidQwtIMDWorkspaceData() override;

  QwtData *copy() const override;
  virtual MantidQwtIMDWorkspaceData *
  copy(Mantid::API::IMDWorkspace_sptr workspace) const;

  size_t size() const override;
  size_t esize() const override;

  void setPreviewMode(bool preview);
  void setPlotAxisChoice(int choice);
  void setNormalization(Mantid::API::MDNormalization choice);

  QString getXAxisLabel() const override;
  QString getYAxisLabel() const override;
  int currentPlotXAxis() const;

  bool setAsDistribution(bool on = true);

protected:
  double getX(size_t i) const override;
  double getY(size_t i) const override;
  double getE(size_t i) const override;
  double getEX(size_t i) const override;

private:
  void copyData(const MantidQwtIMDWorkspaceData &data);

  void cacheLinePlot();
  void calculateMinMax();
  void choosePlotAxis();

  friend class MantidMatrixCurve;

  /// Pointer to the Mantid workspace being displayed
  Mantid::API::IMDWorkspace_const_sptr m_workspace;

  /// Are we in preview mode?
  bool m_preview;

  /// Start point of the line in the workspace
  Mantid::Kernel::VMD m_start;

  /// End point of the line in the workspace
  Mantid::Kernel::VMD m_end;

  /// Direction from start to end, normalized to unity
  Mantid::Kernel::VMD m_dir;

  /// Cached vector of positions along the line (from the start)
  std::vector<Mantid::coord_t> m_lineX;

  /// Cached vector of signal (normalized)
  std::vector<Mantid::signal_t> m_Y;

  /// Cached vector of error (normalized)
  std::vector<Mantid::signal_t> m_E;

  /// Method of normalization of the signal
  Mantid::API::MDNormalization m_normalization;

  /// Is plotting as distribution
  bool m_isDistribution;

  /// Original workspace (for purposes of showing alternative coordinates)
  boost::weak_ptr<const Mantid::API::IMDWorkspace> m_originalWorkspace;

  /// Optional coordinate transformation to go from distance on line to another
  /// coordinate
  Mantid::API::CoordTransform *m_transform;

  /// Choice of which X axis to plot.
  int m_plotAxis;

  /// Current choice, in the case of auto-determined.
  /// This will correspond to -1 (distance)
  /// or the index into the original workspace dimensions
  int m_currentPlotAxis;
};
#endif
