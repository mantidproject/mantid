#ifndef MANTIDQTAPI_MANTIDQWTIMDWORKSPACEDATA_H
#define MANTIDQTAPI_MANTIDQWTIMDWORKSPACEDATA_H

#include <boost/shared_ptr.hpp>
#include "MantidAPI/IMDWorkspace.h"
#include <QObject>
#include "MantidQtAPI/MantidQwtWorkspaceData.h"
#include "MantidKernel/VMD.h"

class MantidQwtIMDWorkspaceData:  public QObject, public MantidQwtWorkspaceData
{
  Q_OBJECT
public:
  MantidQwtIMDWorkspaceData(Mantid::API::IMDWorkspace_const_sptr workspace, const bool logScale,
      Mantid::Kernel::VMD start = Mantid::Kernel::VMD(), Mantid::Kernel::VMD end = Mantid::Kernel::VMD(),
      Mantid::API::MDNormalization normalize = Mantid::API::NoNormalization,
      bool isDistribution = false);

  MantidQwtIMDWorkspaceData(const MantidQwtIMDWorkspaceData& data);

  virtual QwtData *copy() const;
  virtual MantidQwtIMDWorkspaceData* copy(Mantid::API::IMDWorkspace_sptr workspace) const;

  virtual size_t size() const;
  virtual double x(size_t i) const;
  virtual double y(size_t i) const;

  double e(size_t i)const;
  double ex(size_t i)const;
  size_t esize()const;

  bool sameWorkspace(Mantid::API::IMDWorkspace_sptr workspace)const;

  /// Inform the data that it is to be plotted on a log y scale
  void setLogScale(bool on);
  bool logScale()const{return m_logScale;}
  void saveLowestPositiveValue(const double v);
  bool setAsDistribution(bool on = true);

  void applyOffsets(const double xOffset, const double yOffset);

private:

  void cacheLinePlot();

  friend class MantidMatrixCurve;

  /// Pointer to the Mantid workspace
  Mantid::API::IMDWorkspace_const_sptr m_workspace;
  /// Indicates that the data is plotted on a log y scale
  bool m_logScale;
  /// lowest positive y value
  mutable double m_minPositive;

  /// Start point of the line in the workspace
  Mantid::Kernel::VMD m_start;

  /// End point of the line in the workspace
  Mantid::Kernel::VMD m_end;

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

  /// Optional coordinate transformation to go from distance on line to another coordinate
  Mantid::API::CoordTransform * m_transform;

  /// If m_transform is specified, X (on the line) = this dimension index in the output coordinates
  size_t m_dimensionIndex;
};
#endif
