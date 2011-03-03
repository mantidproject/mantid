#ifndef MANTID_CURVE_H
#define MANTID_CURVE_H

#include "../PlotCurve.h"
#include "WorkspaceObserver.h"
#include "boost/shared_ptr.hpp"
#include "MantidAPI/MatrixWorkspace.h"


// Forward definitions
namespace Mantid
{

  namespace API
  {
    class MatrixWorkspace;
    class Workspace;
  }
}

class MantidQwtData;
class Graph;
class MantidUI;

/** 
    This class is for plotting spectra or bins from a Mantid MatrixWorkspace in a 
    QtiPlot's Graph widget.
    
    @author Roman Tolchenov, Tessella plc
    @date 09/09/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/

class MantidCurve:public PlotCurve, public WorkspaceObserver
{
  Q_OBJECT
public:

  /// More complex constructor setting some defaults for the curve
  MantidCurve(const QString& name,const QString& wsName,Graph* g,
              const QString& type,int index,bool err=false);

  /// More complex constructor setting some defaults for the curve
  MantidCurve(const QString& wsName,Graph* g,
              const QString& type,int index,bool err=false);

  /// Copy constructor 
  MantidCurve(const MantidCurve& c);

  ~MantidCurve();

  PlotCurve* clone()const{return new MantidCurve(*this);}

  /// Curve type. Used in the QtiPlot API.
  int rtti() const{return Rtti_PlotUserItem;}

  /// Overrides qwt_plot_curve::setData to make sure only data of MantidQwtData type can  be set
  void setData(const QwtData &data);

  /// Overrides qwt_plot_curve::boundingRect
  QwtDoubleRect boundingRect() const;
  /// Invalidates the bounding rect forcing it to be recalculated
  void invalidateBoundingRect(){m_boundingRect = QwtDoubleRect();}

  /// Return pointer to the data if it of the right type or 0 otherwise
  MantidQwtData* mantidData();
  /// Return pointer to the data if it of the right type or 0 otherwise, const version
  const MantidQwtData* mantidData()const;

  /// Enables/disables drawing of error bars
  void setErrorBars(bool yes=true,bool drawAll = false){m_drawErrorBars = yes;m_drawAllErrorBars = drawAll;}

  virtual void draw(QPainter *p, 
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRect &) const;

  /// Overriden virtual method
  void itemChanged();
  /// saves the mantidcurve details to project file.
  QString saveToString();

  /// The workspace name
  QString workspaceName()const{return m_wsName;}
  /// Returns the workspace index if a spectrum is plotted and -1 if it is a bin.
  int workspaceIndex()const;

private:
  /// Init the curve
  void init(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,Graph* g,
              int index);

  /// Handles delete notification
  void deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
  {
    (void) ws; //Avoid compiler warning
    if (wsName == m_wsName.toStdString())
    {
      observeDelete(false);
      emit removeMe(this);
    }
  }
  /// Handles afterReplace notification
  void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);

  /// Handle an ADS clear notificiation
  void clearADSHandle()
  {
    emit removeMe(this);
  }

signals:

  void resetData(const QString&);

private slots:

  void dataReset(const QString&);

  void axisScaleChanged(int axis, bool toLog);

private:

  /// Make the curve name
  static QString createCurveName(const boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws,
                                 const QString& wsName,int index);
  /// Make a name for a copied curve
  static QString createCopyName(const QString& curveName);
  bool m_drawErrorBars;///< True for drawing error bars
  bool m_drawAllErrorBars; ///< if true and m_drawErrorBars is true draw all error bars (no skipping)
  QString m_wsName;///< Workspace name. If empty the ws isn't in the data service
  /// workspace index
  int  m_index;
  /// The bounding rect used by qwt to set the axes
  mutable QwtDoubleRect m_boundingRect;
};


//=================================================================================================
//=================================================================================================
/**  This class implements QwtData with direct access to a spectrum in a MatrixWorkspace.
 */
class MantidQwtData: public QObject, public QwtData
{
  Q_OBJECT
public:
  /// Constructor
  MantidQwtData(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,int specIndex);

  /// Copy constructor
  MantidQwtData(const MantidQwtData& data);

    //! @return Pointer to a copy (virtual copy constructor)
  virtual QwtData *copy() const {return new MantidQwtData(*this);}

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
  virtual MantidQwtData* copy(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace)const
  {
    return new MantidQwtData(workspace,m_spec);
  }
  /// Returns the error of the i-th data point
  double e(size_t i)const;
  /// Returns the x position of the error bar for the i-th data point (bin)
  double ex(size_t i)const;
  /// Number of error bars to plot
  int esize()const;

  bool isHistogram()const{return m_isHistogram;}

  bool sameWorkspace(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace)const;

  /// Inform the data that it is to be plotted on a log y scale
  void setLogScale(bool on){m_logScale = on;}
  bool logScale()const{return m_logScale;}

private:

  friend class MantidCurve;

  /// Pointer to the Mantid workspace
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_workspace;
  /// Spectrum index in the workspace
  int m_spec;
  /// Copy of the X vector
  const Mantid::MantidVec m_X;

  /// Copy of the Y vector
  const Mantid::MantidVec m_Y;

  /// Copy of the E vector
  const Mantid::MantidVec m_E;

  /// Is the spectrum a histogram?
  bool m_isHistogram;
  /// This field can be set true for a histogram worspace. If it's true x(i) returns (X[i]+X[i+1])/2
  bool m_binCentres;
  /// Indicates that the data is plotted on a log y scale
  bool m_logScale;
  /// temporary storage for a y value
  mutable double m_tmp;

};


#endif // MANTID_CURVE_H
