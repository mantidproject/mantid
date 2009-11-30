#ifndef PEAK_PICKER_TOOL_H
#define PEAK_PICKER_TOOL_H

//---------------------------
// Includes
//---------------------------

#include "../PlotToolInterface.h"
#include "IFunctionWrapper.h"
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <Qt>

#include "../Graph.h"
#include "qwt_plot_item.h"

//---------------------------
// Forward declarations
//---------------------------

class MantidUI;
class QwtPlotCurve;
class QPoint;
class QToolBar;
class PeakRangeMarker;
class FitPropertyBrowser;

/** 
    This class is for selecting peaks on a graph for subsequent fitting.
    As a QwtPlotPicker it uses its eventFilter method to intercept mouse 
    and keyboard input.

    @author Roman Tolchenov, Tessella plc
    @date 10/08/2009

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
class PeakPickerTool : public QwtPlotPicker, public PlotToolInterface
{
	Q_OBJECT
	public:
    /// Constructor
		PeakPickerTool(Graph *graph, MantidUI *mantidUI);
    /// Destructor
    ~PeakPickerTool();
    /// Runtime type identifier
    int rtti() const { return Rtti_SelectPeakTool;};
    /// Receives and processes mouse and keyboard events
    bool eventFilter(QObject *obj, QEvent *event);
    /// Returns the marker object
    PeakRangeMarker* marker()const{return m_range;}
    /// Workspace name
    const QString& workspaceName()const{return m_wsName;}
    /// Spectrum index
    int spec()const{return m_spec;}
    /// The parent graph
    Graph* graph()const{return d_graph;}
  public slots:
    void windowStateChanged( Qt::WindowStates oldState, Qt::WindowStates newState );
  signals:
    void peakChanged();
	private slots:
    void functionChanged();
	private:
    FitPropertyBrowser* fitBrowser();
    /// The parent application window
		MantidUI* m_mantidUI;
    /// Marks on the graph the fitting range
    PeakRangeMarker* m_range;
    /// Workspace name
    QString m_wsName;
    /// Spectrum index
    int m_spec;
};

/**  This class stores the selected peaks parameters and displays
 *   peak markers. Derived from QwtPlotItem it overrides its virtual
 *   draw(..) method.
 */
class PeakRangeMarker: public QwtPlotItem, public IFunctionWrapper
{
public:
  // Peak parameter type
  //struct PeakParams
  //{
  //  std::string fnName;
  //  double centre;
  //  double height;
  //  double width;
  //  PeakParams(double c,double h,double w):fnName("Gaussian"),centre(c),height(h),width(w){}
  //};
  // Constructor
  PeakRangeMarker();
  // Drawing method
  virtual void draw(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRect &) const;
  // Add a new peak with centre c and height h. 
  void add(double c,double h);
  // Return the centre of the currently selected peak
  double centre()const;
  // Return the width of the currently selected peak
  double width()const;
  // Return the height of the currently selected peak
  double height()const;
  // Check if the width is been set
  bool isWidthSet()const;
  // Set the width set flag
  void widthIsSet(bool yes=true);
  // Change the width of the currently selected peak
  void setWidth(double x);
  // Return current function name
  std::string fnName()const;
  // Set new function name
  void fnName(const std::string& name);

  // Check if x is near the xMin marker (+-dx)
  bool clickedOnXMin(double x,double dx)
  {
    double c = xMin();
    return (fabs(x - c) <= dx);
  }
  // Check if x is near the xMax marker (+-dx)
  bool clickedOnXMax(double x,double dx)
  {
    double c = xMax();
    return (fabs(x - c) <= dx);
  }
  // Check if x is near a width marker (+-dx)
  bool clickedOnWidthMarker(double x,double dx)
  {
    double c = centre();
    double w = width()/2;
    return (fabs(x - c - w) <= dx) || (fabs(x - c + w) <= dx);
  }
  // Check if x is near a peak centre marker (+-dx). If true returns the peak's index or -1 otherwise.
  int clickedOnCentreMarker(double x,double dx)
  {
    //for(int i=0;i<m_params.size();i++)
    //  if (fabs(x - m_params[i].centre) <= dx) return i;
    return -1;
  }
  // Change current peak
  void setCurrent(int i)
  {
    //if (i >= 0 && i < m_params.size())
    //  m_current = i;
  }
  // Give new centre and height to the current peak
  void reset(double c,double h)
  {
    if (m_current < 0)
    {
      add(c,h);
      return;
    }

    //m_params[m_current].centre = c;
    //m_params[m_current].height = h;
  }
  // Indicates that the marker (and peak tool) is in a process of resetting a peak (changing centre and height)
  bool resetting()const{return m_resetting;}
  // Switch on/off the resetting flag
  void resetting(bool ok){m_resetting = ok;}
  // Lower fit boundary
  double xMin()const{return m_xMin;}
  void xMin(double x)
  {
    m_xMin = x;
    if (x > m_xMax) 
    {
      m_xMax = x;
    }
  }
  bool changingXMin()const{return m_changingXMin;}
  void changingXMin(bool ok){m_changingXMin = ok;}
  // Upper fit boundary
  double xMax()const  {return m_xMax;}
  void xMax(double x) 
  {
    m_xMax = x;
    if (x < m_xMin)
    {
      m_xMin = x;
    }
  }
  bool changingXMax()const{return m_changingXMax;}
  void changingXMax(bool ok){m_changingXMax = ok;}
private:
  
  //QList<PeakParams> m_params;// The parameter storage
  std::string m_fnName; // Last changed function name
  double m_width;   // Last changed width
  int m_current;    // Index of the current peak
  bool m_width_set; // The width set flag
  bool m_resetting; // The resetting flag
  double m_xMin;    // Lower fit boundary
  double m_xMax;    // Upper fit boundary
  bool m_changingXMin; // Flag indicating that changing of xMin is in progress
  bool m_changingXMax; // Flag indicating that changing of xMax is in progress

  //virtual QwtDoubleRect boundingRect() const{return QwtDoubleRect(p0, p1);}

};


#endif /* PEAK_PICKER_TOOL_H */
