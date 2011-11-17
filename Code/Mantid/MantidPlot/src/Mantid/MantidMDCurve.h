#ifndef MANTID_MD_CURVE_H
#define MANTID_MD_CURVE_H

#include "MantidCurve.h"
#include <boost/shared_ptr.hpp>
#include "MantidAPI/IMDWorkspace.h"


// Forward definitions

class MantidQwtIMDWorkspaceData;
class MantidUI;

/** 
    This class is for plotting IMDWorkspaces

    @date 17/11/2011

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

class MantidMDCurve : public MantidCurve
{
  Q_OBJECT
public:

  /// More complex constructor setting some defaults for the curve
  MantidMDCurve(const QString& wsName,Graph* g,
              bool err=false,bool distr = false, Graph::CurveType style = Graph::Unspecified);

  /// Copy constructor 
  MantidMDCurve(const MantidMDCurve& c);

  ~MantidMDCurve();

  MantidMDCurve* clone(const Graph*)const;

  /// Curve type. Used in the QtiPlot API.
  int rtti() const{return Rtti_PlotUserItem;}

  /// Used for waterfall plots: updates the data curves with an offset
  //void loadData();

  /// Overrides qwt_plot_curve::setData to make sure only data of MantidQwtMatrixWorkspaceData type can  be set
  void setData(const QwtData &data);

  /// Overrides qwt_plot_curve::boundingRect
  QwtDoubleRect boundingRect() const;
  
  /// Invalidates the bounding rect forcing it to be recalculated
  void invalidateBoundingRect(){m_boundingRect = QwtDoubleRect();}

  /// Return pointer to the data if it of the right type or 0 otherwise
  MantidQwtIMDWorkspaceData* mantidData();

  /// Return pointer to the data if it of the right type or 0 otherwise, const version
  const MantidQwtIMDWorkspaceData* mantidData()const;

  /// Enables/disables drawing of error bars
  void setErrorBars(bool yes=true,bool drawAll = false){m_drawErrorBars = yes;m_drawAllErrorBars = drawAll;}

  /// Returns whether the curve has error bars
  bool hasErrorBars() const;

  virtual void draw(QPainter *p, 
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRect &) const;

  /// Overriden virtual method
  void itemChanged();
 
  /// saves the MantidMatrixCurve details to project file.
  QString saveToString();

  /// The workspace name
  QString workspaceName()const{return m_wsName;}

private:

  using PlotCurve::draw; // Avoid Intel compiler warning

  /// Init the curve
  void init(Graph* g, bool distr, Graph::CurveType style);

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

  /// Make a name for a copied curve
  static QString createCopyName(const QString& curveName);
  bool m_drawErrorBars;///< True for drawing error bars
  bool m_drawAllErrorBars; ///< if true and m_drawErrorBars is true draw all error bars (no skipping)
  QString m_wsName;///< Workspace name. If empty the ws isn't in the data service
  /// The bounding rect used by qwt to set the axes
  mutable QwtDoubleRect m_boundingRect;
};

#endif // MANTID_CURVE_H
