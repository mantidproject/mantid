#ifndef MANTIDPLOT_MANTIDCURVE_H
#define MANTIDPLOT_MANTIDCURVE_H

#include "../PlotCurve.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/Workspace.h"
#include "MantidQwtWorkspaceData.h"

/** Base class for MantidCurve types. 
    
    @date 17/11/2011

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class Graph;
class MantidCurve :public PlotCurve, public MantidQt::API::WorkspaceObserver
{
  Q_OBJECT
public:
  /// Constructor
  MantidCurve(const QString& wsName);
  /// Default constructor
  MantidCurve();
  /// Destructor
  virtual ~MantidCurve();
  /// Clone
  virtual MantidCurve* clone(const Graph* g) const = 0;
  /// Get mantid data 
  virtual const MantidQwtWorkspaceData* mantidData() const = 0;
  /// Get mantid data 
  virtual MantidQwtWorkspaceData* mantidData() = 0;
  /// Invalidates the bounding rect forcing it to be recalculated
  void invalidateBoundingRect(){m_boundingRect = QwtDoubleRect();}

  /*-------------------------------------------------------------------------------------
  Public Base/Common methods
  -------------------------------------------------------------------------------------*/

   QwtDoubleRect MantidCurve::boundingRect() const;

  /*-------------------------------------------------------------------------------------
  End Public Base/Common methods
  -------------------------------------------------------------------------------------*/

protected slots:
  
  void axisScaleChanged(int axis, bool toLog);

protected:
  /*-------------------------------------------------------------------------------------
  Protected Base/Common methods
  -------------------------------------------------------------------------------------*/

  /// Apply the style choice
  void applyStyleChoice(Graph::CurveType style, MultiLayer* ml, int& lineWidth);

  /*-------------------------------------------------------------------------------------
  End Protected Base/Common methods
  -------------------------------------------------------------------------------------*/
  
private:
  /// The bounding rect used by qwt to set the axes
  mutable QwtDoubleRect m_boundingRect;
  
  //To ensure that all MantidCurves can work with Mantid Workspaces.
  virtual void init(Graph* g, bool distr, Graph::CurveType style) = 0;
};

#endif

