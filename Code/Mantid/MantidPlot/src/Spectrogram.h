/***************************************************************************
	File                 : Spectrogram.h
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2006 by Ion Vasilief
	Email (use @ for *)  : ion_vasilief*yahoo.fr
	Description          : QtiPlot's Spectrogram Class
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#include "Matrix.h"
#include "UserFunction.h"
#include <qwt_raster_data.h>
#include <qwt_plot.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_color_map.h>
#include <qwt_plot_marker.h>
#include "qwt_color_map.h"
#include "PlotCurve.h"

#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtAPI/MantidColorMap.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "Mantid/InstrumentWidget/GLColor.h"

#include <fstream>
#include <float.h>
#include <boost/shared_ptr.hpp>
#include <QPainter>
#include <qobject.h>

class MatrixData;
class PlotMarker;

namespace MantidQt
{
  namespace API
  {
    class QwtRasterDataMD;
  }
}

class Spectrogram: public QObject, public QwtPlotSpectrogram, public MantidQt::API::WorkspaceObserver
{
  Q_OBJECT

public:
  Spectrogram();
  Spectrogram(Matrix *m);
  Spectrogram(const QString &wsName, const Mantid::API::IMDWorkspace_const_sptr & workspace);
  Spectrogram(Function2D *f,int nrows, int ncols,double left, double top, double width, double height,double minz,double maxz);//Mantid
  Spectrogram(Function2D *f,int nrows, int ncols,QwtDoubleRect bRect,double minz,double maxz);//Mantid
  ~Spectrogram();

  void loadFromProject(const std::string& lines);
  std::string saveToProject();

  /// Handles delete notification
  void postDeleteHandle(const std::string& wsName);
  /// Handles afterReplace notification
  void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);
  /// Handle an ADS clear notificiation
  void clearADSHandle();

  enum ColorMapPolicy{GrayScale, Default, Custom};

  virtual QImage renderImage(
      const QwtScaleMap &xMap, const QwtScaleMap &yMap,
      const QwtDoubleRect &rect) const;


  Spectrogram* copy();
  Matrix * matrix(){return d_matrix;};
  Function2D *funct(){return d_funct;}

  int levels()const{return (int)contourLevels().size() + 1;};
  void setLevelsNumber(int levels);

  bool hasColorScale();
  int colorScaleAxis(){return color_axis;};
  void showColorScale(int axis, bool on = true);

  int colorBarWidth();
  void setColorBarWidth(int width);

  void setGrayScale();
  void setDefaultColorMap();
  MantidColorMap getDefaultColorMap();
  static QwtLinearColorMap defaultColorMap();

  void loadColorMap(const QString& file);
  void setCustomColorMap(const QwtLinearColorMap& map);
  void setMantidColorMap(const MantidColorMap &map);
  void updateData(Matrix *m);
  void updateData(const Mantid::API::IMDWorkspace_const_sptr & workspace);
  MantidQt::API::QwtRasterDataMD *dataFromWorkspace(const Mantid::API::IMDWorkspace_const_sptr & workspace);
  void postDataUpdate();

  ColorMapPolicy colorMapPolicy()const{return color_map_policy;};

  virtual QwtDoubleRect boundingRect() const;
  double getMinPositiveValue()const;
  void setContourPenList(QList<QPen> lst);
  void setContourLinePen(int index, const QPen &pen);
  QList<QPen> contourPenList(){return d_pen_list;};
  //! Flag telling if we use the color map to calculate the pen (QwtPlotSpectrogram::contourPen()).
  bool d_color_map_pen;
  bool useColorMapPen()const{return d_color_map_pen;};
  void setColorMapPen(bool on = true);
  void showContourLineLabels(bool show = true);
  bool hasLabels(){return d_show_labels;};
  QFont labelsFont(){return d_labels_font;};
  void setLabelsFont(const QFont& font);

  QList <PlotMarker *> labelsList(){return d_labels_list;};
  const MantidColorMap & getColorMap() const;
  MantidColorMap & mutableColorMap();
  void saveSettings();
  void loadSettings();
  void setColorMapFileName(QString colormapName);
  double labelsRotation(){return d_labels_angle;};
  void setLabelsRotation(double angle);
  bool labelsWhiteOut(){return d_white_out_labels;};
  void setLabelsWhiteOut(bool whiteOut);
  void setLabelsOffset(double x, double y);
  void setLabelOffset(int index, double x, double y);
  QColor labelsColor(){return d_labels_color;};
  void setLabelsColor(const QColor& c);
  //	void setLabelOffset(int index, double x, double y);

  void updateForNewMaxData(const double new_max);
  void updateForNewMinData(const double new_min);
  void recount();
  void setCustomColorMap(const QwtColorMap &map);

  void setContourLevels (const QwtValueList & levels);
  bool hasSelectedLabels();
  bool selectedLabels(const QPoint& pos);
  double labelsXOffset(){return d_labels_x_offset;};
  double labelsYOffset(){return d_labels_y_offset;};
  void selectLabel(bool on);
  /// change intensity of the colors
  void changeIntensity( double start,double end);
  ///sets a boolan flag for intensity changes
  void setIntensityChange(bool on);
  /// returns boolan flag intensity change
  bool isIntensityChanged();

signals:
  void removeMe(Spectrogram*);

protected:
  virtual void drawContourLines (QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QwtRasterData::ContourLines &lines) const;
  void updateLabels(QPainter *p, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QwtRasterData::ContourLines &lines) const;
  void createLabels();

  //! Pointer to the source data matrix
  Matrix *d_matrix;
  Function2D *d_funct;
  /// Pointer to data source for a workspace
  MantidQt::API::QwtRasterDataMD *d_wsData;
  /// Name of the workspace backing the spectrogram
  std::string d_wsName;

  //! Axis used to display the color scale
  int color_axis;

  //! Flags
  ColorMapPolicy color_map_policy;

  QwtLinearColorMap color_map;

  QList<QPen> d_pen_list;
  //! Flag telling if we display the labels
  bool d_show_labels;
  //! Flag telling if we paint a white background for the labels
  bool d_white_out_labels;
  double d_labels_angle;
  //! List of the text labels associated to this spectrogram.
  QList <PlotMarker *> d_labels_list;
  //! Keeps track of the plot marker on which the user clicked when selecting the labels.
  PlotMarker *d_selected_label;

  //! Keep track of the coordinates of the point where the user clicked when selecting the labels.
  double d_click_pos_x, d_click_pos_y;
  //! Labels color
  QColor d_labels_color;
  double d_labels_x_offset, d_labels_y_offset;
  int d_labels_align;
  //! Labels font
  QFont d_labels_font;

  MantidColorMap mColorMap;
  QString mCurrentColorMap;

  int m_nRows;
  int m_nColumns;
  /// Store a value between 0->255 for each of the integrated spectra.
  std::vector<unsigned char> mScaledValues;
  /// boolean flag to indicate intensity changed
  bool m_bIntensityChanged;
};

class SpectrogramData: public QwtRasterData
{
public:
  SpectrogramData(const QwtDoubleRect &rect):QwtRasterData(rect){}
  virtual double getMinPositiveValue()const = 0;
};


class MatrixData: public SpectrogramData
{
public:
  MatrixData(Matrix *m):
    SpectrogramData(m->boundingRect()),
    d_matrix(m)
  {
    n_rows = d_matrix->numRows();
    n_cols = d_matrix->numCols();

    d_m = new double* [n_rows];
    for ( int l = 0; l < n_rows; ++l)
      d_m[l] = new double [n_cols];

    for (int i = 0; i < n_rows; i++)
      for (int j = 0; j < n_cols; j++)
        d_m[i][j] = d_matrix->cell(i, j);

    m->range(&min_z, &max_z);

    x_start = d_matrix->xStart();
    dx = d_matrix->dx();
    y_start = d_matrix->yStart();
    dy = d_matrix->dy();
  }

  ~MatrixData()
  {
    for (int i = 0; i < n_rows; i++)
      delete [] d_m[i];

    delete [] d_m;
  };

  virtual QwtRasterData *copy() const
  {
    return new MatrixData(d_matrix);
  }

  virtual QwtDoubleInterval range() const
  {
    return QwtDoubleInterval(min_z, max_z);
  }

  virtual QSize rasterHint (const QwtDoubleRect &) const
  {
    return QSize(n_cols, n_rows);
  }

  virtual double value(double x, double y) const;

  double getMinPositiveValue()const;

private:
  //! Pointer to the source data matrix
  Matrix *d_matrix;

  //! Vector used to store in memory the data from the source matrix window
  double** d_m;

  //! Data size
  int n_rows, n_cols;

  //! Min and max values in the source data matrix
  double min_z, max_z;

  //! Data resolution in x(columns) and y(rows)
  double dx, dy;

  //! X axis left value in the data matrix
  double x_start;

  //! Y axis bottom value in the data matrix
  double y_start;

};

class FunctionData: public SpectrogramData
{
public:
  FunctionData(Function2D *f,int nrows, int ncols,double left, double top, double width, double height,double minz,double maxz):
    SpectrogramData(QwtDoubleRect(left, top, width, height)),
    d_funct(f),n_rows(nrows),n_cols(ncols),min_z(minz),max_z(maxz)
  {
  }

  FunctionData(Function2D *f,int nrows, int ncols,QwtDoubleRect bRect,double minz,double maxz):
    SpectrogramData(bRect),
    d_funct(f),n_rows(nrows),n_cols(ncols),min_z(minz),max_z(maxz)
  {
  }

  ~FunctionData()
  {
  };

  virtual QwtRasterData *copy() const
  {
    return new FunctionData(d_funct, n_rows, n_cols,boundingRect(),min_z,max_z);
  }

  virtual QwtDoubleInterval range() const
  {
    return QwtDoubleInterval(min_z, max_z);
  }

  virtual QSize rasterHint (const QwtDoubleRect &) const
  {
    return QSize(n_cols, n_rows);
  }

  virtual double value(double x, double y) const
  {
    //static std::ofstream f("funct.txt");
    //f<<x<<' '<<y<<' '<<d_funct->operator()(x,y)<<'\n';
    return d_funct->operator()(x,y);
  }

  double getMinPositiveValue()const{return d_funct->getMinPositiveValue();}

private:
  Function2D *d_funct;

  //! Data size
  int n_rows, n_cols;

  //! Min and max values in the source data matrix
  double min_z, max_z;
};

#endif
