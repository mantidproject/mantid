#ifndef MANTIDQTAPI_MANTIDQWTMATRIXWORKSPACEDATA_H
#define MANTIDQTAPI_MANTIDQWTMATRIXWORKSPACEDATA_H

#include <boost/shared_ptr.hpp>
#include "MantidAPI/MatrixWorkspace.h"
#include <QObject>
#include "MantidQtAPI/MantidQwtWorkspaceData.h"
#include "DllOption.h"

//=================================================================================================
//=================================================================================================
/**  This class implements QwtData with direct access to a spectrum in a MatrixWorkspace.
 */
class EXPORT_OPT_MANTIDQT_API MantidQwtMatrixWorkspaceData : public MantidQwtWorkspaceData
{
public:
  MantidQwtMatrixWorkspaceData(Mantid::API::MatrixWorkspace_const_sptr workspace, int specIndex, const bool logScale, bool distr = false);
  MantidQwtMatrixWorkspaceData(const MantidQwtMatrixWorkspaceData& data);
  MantidQwtMatrixWorkspaceData& operator=(const MantidQwtMatrixWorkspaceData &);

    //! @return Pointer to a copy (virtual copy constructor)
  virtual QwtData *copy() const {return new MantidQwtMatrixWorkspaceData(*this);}

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
  virtual MantidQwtMatrixWorkspaceData* copy(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace)const
  {
    return new MantidQwtMatrixWorkspaceData(workspace,m_spec, m_logScale);
  }
  /// Returns the error of the i-th data point
  double e(size_t i)const;
  /// Returns the x position of the error bar for the i-th data point (bin)
  double ex(size_t i)const;
  /// Number of error bars to plot
  size_t esize()const;

  double getYMin() const;
  double getYMax() const;

  bool isHistogram()const{return m_isHistogram;}

  /// Inform the data that it is to be plotted on a log y scale
  void setLogScale(bool on);
  bool logScale()const{return m_logScale;}
  void saveLowestPositiveValue(const double v);
  bool setAsDistribution(bool on = true);

private:

  friend class MantidMatrixCurve;

  /// Pointer to the Mantid workspace
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_workspace;
  /// Spectrum index in the workspace
  int m_spec;
  /// Copy of the X vector
  Mantid::MantidVec m_X;
  /// Copy of the Y vector
  Mantid::MantidVec m_Y;
  /// Copy of the E vector
  Mantid::MantidVec m_E;

  /// Is the spectrum a histogram?
  bool m_isHistogram;
  /// This field can be set true for a histogram workspace. If it's true x(i) returns (X[i]+X[i+1])/2
  bool m_binCentres;
  /// Indicates that the data is plotted on a log y scale
  bool m_logScale;
  /// lowest positive y value
  mutable double m_minPositive;
  /// Is plotting as distribution
  bool m_isDistribution;

};
#endif
