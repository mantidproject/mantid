#ifndef MANTID_CURVE_H
#define MANTID_CURVE_H

#include "../PlotCurve.h"
#include "WorkspaceObserver.h"
#include "boost/shared_ptr.hpp"


// Forward definitions
namespace Mantid
{
  typedef std::vector<double> MantidVec;
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

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratories

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

  /// Return pointer to the data if it of the right type or 0 otherwise
  MantidQwtData* mantidData();
  /// Return pointer to the data if it of the right type or 0 otherwise, const version
  const MantidQwtData* mantidData()const;

  /// Enables/disables drawing of error bars
  void setErrorBars(bool yes=true){m_drawErrorBars = yes;}

  virtual void draw(QPainter *p, 
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRect &) const;

  /// Overriden virtual method
  void itemChanged();

private:
  /// Init the curve
  void init(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,Graph* g,
              const QString& type,int index);

  void deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
  {
    if (wsName == m_wsName.toStdString())
      emit removeMe(this);
  }

  void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);

  /// Make the curve name
  static QString createCurveName(const QString& wsName,const QString& type,int index);
  /// Make a name for a copied curve
  static QString createCopyName(const QString& curveName);
  bool m_drawErrorBars;///< True for drawing error bars
  QString m_wsName;///< Workspace name. If empty the ws isn't in the data service
};

/**  This class implements QwtData with direct access to a MatrixWorkspace.
 */
class MantidQwtData: public QwtData
{
public:
  /// Constructor
  MantidQwtData(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace)
    :m_workspace(workspace){}

  /// Copy constructor
  MantidQwtData(const MantidQwtData& data)
    :m_workspace(data.m_workspace){}

  virtual ~MantidQwtData();

  bool sameWorkspace(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace)const;

  /// Return a new data object of the same type but with a new workspace
  virtual MantidQwtData* copy(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace)const = 0;
  /// Returns the error of the i-th data point
  virtual double e(size_t i)const = 0;
  /// Returns the x position of the error bar for the i-th data point (bin)
  virtual double ex(size_t i)const = 0;
protected:
  /// Pointer to the Mantid workspace
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_workspace;
};

/**  This class implements QwtData with direct access to a spectrum in a MatrixWorkspace.
 */
class MantidQwtDataSpectra: public MantidQwtData
{
public:
  /// Constructor
  MantidQwtDataSpectra(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,int specIndex);

  /// Copy constructor
  MantidQwtDataSpectra(const MantidQwtDataSpectra& data);

    //! \return Pointer to a copy (virtual copy constructor)
  virtual QwtData *copy() const {return new MantidQwtDataSpectra(*this);}

  //! \return Size of the data set
  virtual size_t size() const;

  /*!
  Return the x value of data point i
  \param i Index
  \return x X value of data point i
  */
  virtual double x(size_t i) const;
  /*!
  Return the y value of data point i
  \param i Index
  \return y Y value of data point i
  */
  virtual double y(size_t i) const;

  /// Return a new data object of the same type but with a new workspace
  virtual MantidQwtData* copy(boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace)const
  {
    return new MantidQwtDataSpectra(workspace,m_spec);
  }
  /// Returns the error of the i-th data point
  double e(size_t i)const;
  /// Returns the x position of the error bar for the i-th data point (bin)
  double ex(size_t i)const;

private:

  friend class MantidCurve;

  /// Spectrum index in the workspace
  int m_spec;
  /// Reference to the X vector
  const Mantid::MantidVec& m_X;
  /// Reference to the Y vector
  const Mantid::MantidVec& m_Y;
  /// Reference to the E vector
  const Mantid::MantidVec& m_E;
  /// Is the spectrum a histogram?
  bool m_isHistogram;
  /// This field can be set true for a histogram worspace. If it's true x(i) returns (X[i]+X[i+1])/2
  bool m_binCentres;

};


#endif // MANTID_CURVE_H
