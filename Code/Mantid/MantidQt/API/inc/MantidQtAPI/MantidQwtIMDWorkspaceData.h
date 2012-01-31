#ifndef MANTIDQTAPI_MANTIDQWTIMDWORKSPACEDATA_H
#define MANTIDQTAPI_MANTIDQWTIMDWORKSPACEDATA_H

#include <boost/shared_ptr.hpp>
#include "MantidAPI/IMDWorkspace.h"
#include <QObject>
#include "MantidQtAPI/MantidQwtWorkspaceData.h"

class MantidQwtIMDWorkspaceData:  public QObject, public MantidQwtWorkspaceData
{
  Q_OBJECT
public:
  /// Constructor
  MantidQwtIMDWorkspaceData(Mantid::API::IMDWorkspace_const_sptr workspace, const bool logScale, bool distr = false);

  /// Copy constructor
  MantidQwtIMDWorkspaceData(const MantidQwtIMDWorkspaceData& data);

    //! @return Pointer to a copy (virtual copy constructor)
  virtual QwtData *copy() const {return new MantidQwtIMDWorkspaceData(*this);}

  //! @return Size of the data set
  virtual size_t size() const;

  /**
  Return the x value of data point i
  @param i :: Index
  @return x X value of data point i
  */
  virtual double x(size_t i) const;
  /**
  Return the y value of data point i
  @param i :: Index
  @return y Y value of data point i
  */
  virtual double y(size_t i) const;

  /// Return a new data object of the same type but with a new workspace
  virtual MantidQwtIMDWorkspaceData* copy(Mantid::API::IMDWorkspace_sptr workspace)const
  {
    return new MantidQwtIMDWorkspaceData(workspace, m_logScale);
  }
  /// Returns the error of the i-th data point
  double e(size_t i)const;
  /// Returns the x position of the error bar for the i-th data point (bin)
  double ex(size_t i)const;
  /// Number of error bars to plot
  size_t esize()const;

  bool sameWorkspace(Mantid::API::IMDWorkspace_sptr workspace)const;

  /// Inform the data that it is to be plotted on a log y scale
  void setLogScale(bool on);
  bool logScale()const{return m_logScale;}
  void saveLowestPositiveValue(const double v);
  bool setAsDistribution(bool on = true);

  void applyOffsets(const double xOffset, const double yOffset);

private:

  friend class MantidMatrixCurve;

  /// Pointer to the Mantid workspace
  Mantid::API::IMDWorkspace_const_sptr m_workspace;
  /// Indicates that the data is plotted on a log y scale
  bool m_logScale;
  /// lowest positive y value
  mutable double m_minPositive;
  /// Is plotting as distribution
  bool m_isDistribution;

};
#endif
