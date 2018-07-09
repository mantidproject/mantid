/***************************************************************************
    File                 : OPJFile.h
    --------------------------------------------------------------------
    Copyright            : (C) 2005-2007 Stefan Gerlach
                           (C) 2007 by Alex Kargovsky, Ion Vasilief
    Email (use @ for *)  : kargovsky*yumr.phys.msu.su, ion_vasilief*yahoo.fr
    Description          : Origin project import class

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

/* Origin 7.5 column value display : see FORMAT */

#ifndef OPJFILE_H
#define OPJFILE_H

/* version 0.0 2007-09-26 */
#define LIBORIGIN_VERSION 0x00070926
#define LIBORIGIN_VERSION_STRING "2007-09-26"

#include "tree.hh"
#include <string>
#include <vector>

struct rect {
  short left;
  short top;
  short right;
  short bottom;
  int height() const { return bottom - top; };
  int width() const { return right - left; };
  rect() : left(0), top(0), right(0), bottom(0) {}
  rect(short width, short height)
      : left(0), top(0), right(width), bottom(height) {}
};

struct originWindow {
  enum State { Normal, Minimized, Maximized };
  enum Title { Name, Label, Both };

  std::string name;
  std::string label;
  int objectID;
  bool bHidden;
  State state;
  Title title;
  rect clientRect;
  double creation_date;     // Julian date/time
  double modification_date; // Julian date/time

  originWindow(std::string _name = "", std::string _label = "",
               bool _bHidden = false)
      : name(_name), label(_label), objectID(0), bHidden(_bHidden),
        state(Normal), title(Both), creation_date(0.0),
        modification_date(0.0){};
};
struct originData {
  int type; // 0 - double, 1 - string
  double d;
  std::string s;
  originData(double _d) : type(0), d(_d), s(""){};
  originData(char *_s) : type(1), d(1.0e-307), s(_s){};
};

enum ColumnType { X, Y, Z, XErr, YErr, Label, NONE };

struct spreadColumn {
  std::string name;
  ColumnType type;
  int value_type; // Numeric = 0, Text = 1, Date = 2, Time = 3, Month = 4, Day =
                  // 5, Text&Numeric = 6
  int value_type_specification; // see above
  int significant_digits;
  int decimal_places;
  int numeric_display_type; // Default Decimal Digits=0, Decimal Places=1,
                            // Significant Digits=2
  std::string command;
  std::string comment;
  int width;
  int index;
  std::vector<originData> odata;
  spreadColumn(std::string _name = "", int _index = 0)
      : name(_name), type(NONE), value_type(0), value_type_specification(0),
        significant_digits(6), decimal_places(6), numeric_display_type(0),
        command(""), comment(""), width(8), index(_index){};
};

struct spreadSheet : public originWindow {
  int maxRows;
  bool bLoose;
  bool bMultisheet;
  std::vector<spreadColumn> column;
  spreadSheet(std::string _name = "")
      : originWindow(_name), maxRows(0), bLoose(true), bMultisheet(false),
        column(){};
};

struct excel : public originWindow {
  int maxRows;
  bool bLoose;
  std::vector<spreadSheet> sheet;
  excel(std::string _name = "", std::string _label = "", int _maxRows = 0,
        bool _bHidden = false, bool _bLoose = true)
      : originWindow(_name, _label, _bHidden), maxRows(_maxRows),
        bLoose(_bLoose){};
};

struct matrix : public originWindow {
  enum ViewType { DataView, ImageView };
  enum HeaderViewType { ColumnRow, XY };
  int nr_rows;
  int nr_cols;
  int value_type_specification;
  int significant_digits;
  int decimal_places;
  int numeric_display_type; // Default Decimal Digits=0, Decimal Places=1,
                            // Significant Digits=2
  std::string command;
  int width;
  int index;
  ViewType view;
  HeaderViewType header;
  std::vector<double> data;
  matrix(std::string _name = "", int _index = 0)
      : originWindow(_name), nr_rows(0), nr_cols(0),
        value_type_specification(0), significant_digits(6), decimal_places(6),
        numeric_display_type(0), command(""), width(8), index(_index),
        view(DataView), header(ColumnRow){};
};

struct function {
  std::string name;
  int type; // Normal = 0, Polar = 1
  std::string formula;
  double begin;
  double end;
  int points;
  int index;
  function(std::string _name = "", int _index = 0)
      : name(_name), type(0), formula(""), begin(0.0), end(0.0), points(0),
        index(_index){};
};

struct text {
  std::string txt;
  rect clientRect;
  int color;
  int fontsize;
  int rotation;
  int tab;
  int border_type;
  int attach;

  text(const std::string &_txt = "")
      : txt(_txt), clientRect(), color(0), fontsize(0), rotation(0), tab(0),
        border_type(0), attach(0){};

  text(const std::string &_txt, const rect &_clientRect, int _color,
       int _fontsize, int _rotation, int _tab, int _border_type, int _attach)
      : txt(_txt), clientRect(_clientRect), color(_color), fontsize(_fontsize),
        rotation(_rotation), tab(_tab), border_type(_border_type),
        attach(_attach){};
};

struct pieProperties {
  unsigned char view_angle;
  unsigned char thickness;
  bool clockwise_rotation;
  short rotation;
  unsigned short radius;
  unsigned short horizontal_offset;
  unsigned long displaced_sections; // maximum - 32 sections
  unsigned short displacement;

  // labels
  bool format_automatic;
  bool format_values;
  bool format_percentages;
  bool format_categories;
  bool position_associate;
  unsigned short distance;

  pieProperties()
      : view_angle(0), thickness(0), clockwise_rotation(false), rotation(0),
        radius(0), horizontal_offset(0), displaced_sections(0), displacement(0),
        format_automatic(false), format_values(false),
        format_percentages(false), format_categories(false),
        position_associate(false), distance(0){};
};

struct vectorProperties {
  int color;
  double width;
  unsigned short arrow_lenght;
  unsigned char arrow_angle;
  bool arrow_closed;
  std::string endXColName;
  std::string endYColName;

  int position;
  std::string angleColName;
  std::string magnitudeColName;
  float multiplier;
  int const_angle;
  int const_magnitude;

  vectorProperties()
      : color(0), width(0.0), arrow_lenght(0), arrow_angle(0),
        arrow_closed(false), endXColName(), endYColName(), position(0),
        angleColName(), magnitudeColName(nullptr), multiplier(1.0),
        const_angle(0), const_magnitude(0){};
};

struct graphCurve {
  int type;
  std::string dataName;
  std::string xColName;
  std::string yColName;
  int line_color;
  int line_style;
  int line_connect;
  double line_width;

  bool fillarea;
  int fillarea_type;
  int fillarea_pattern;
  int fillarea_color;
  int fillarea_first_color;
  int fillarea_pattern_color;
  double fillarea_pattern_width;
  int fillarea_pattern_border_style;
  int fillarea_pattern_border_color;
  double fillarea_pattern_border_width;

  int symbol_type;
  int symbol_color;
  int symbol_fill_color;
  double symbol_size;
  int symbol_thickness;
  int point_offset;

  // pie
  pieProperties pie;

  // vector
  vectorProperties vector;
};

enum AxisPosition { Left = 0, Bottom = 1, Right = 2, Top = 3 };

struct graphAxisBreak {
  bool show;

  bool log10;
  double from;
  double to;
  int position;

  double scale_increment_before;
  double scale_increment_after;

  unsigned char minor_ticks_before;
  unsigned char minor_ticks_after;

  graphAxisBreak()
      : show(false), log10(false), from(0.0), to(0.0), position(0),
        scale_increment_before(0), scale_increment_after(0),
        minor_ticks_before(0), minor_ticks_after(0) {}
};

struct graphGrid {
  bool hidden;
  int color;
  int style;
  double width;
};

struct graphAxisFormat {
  bool hidden;
  int color;
  double thickness;
  double majorTickLength;
  int majorTicksType;
  int minorTicksType;
  int axisPosition;
  double axisPositionValue;
};

struct graphAxisTick {
  bool hidden;
  int color;
  int value_type; // Numeric = 0, Text = 1, Date = 2, Time = 3, Month = 4, Day =
                  // 5, Text&Numeric = 6
  int value_type_specification;
  int decimal_places;
  int fontsize;
  bool fontbold;
  std::string dataName;
  std::string colName;
  int rotation;
};

struct graphAxis {
  int pos;
  text label;
  double min;
  double max;
  double step;
  int majorTicks;
  int minorTicks;
  int scale;
  graphGrid majorGrid;
  graphGrid minorGrid;
  graphAxisFormat formatAxis[2];
  graphAxisTick tickAxis[2]; // bottom-top, left-right
};

struct rectangle {
  rect clientRect;
  int attach;
};

struct circle {
  rect clientRect;
  int attach;
};

struct lineVertex {
  int shape_type;
  double shape_width;
  double shape_length;
  double x;
  double y;
  lineVertex()
      : shape_type(0), shape_width(0.0), shape_length(0.0), x(0.0), y(0.0) {}
};

struct line {
  rect clientRect;
  int color;
  int attach;
  double width;
  int line_style;
  lineVertex begin;
  lineVertex end;
};

struct bitmap {
  rect clientRect;
  int attach;
  unsigned long size;
  unsigned char *data;
  double left;
  double top;
  double width;
  double height;
};

struct metafile {
  rect clientRect;
  int attach;
};

struct graphLayer {
  rect clientRect;
  text legend;
  graphAxis xAxis;
  graphAxis yAxis;

  graphAxisBreak xAxisBreak;
  graphAxisBreak yAxisBreak;

  double histogram_bin;
  double histogram_begin;
  double histogram_end;

  std::vector<text> texts;
  std::vector<line> lines;
  std::vector<bitmap> bitmaps;
  std::vector<graphCurve> curve;
};

struct graphLayerRange {
  double min;
  double max;
  double step;

  graphLayerRange(double _min = 0.0, double _max = 0.0, double _step = 0.0) {
    min = _min;
    max = _max;
    step = _step;
  };
};

struct graph : public originWindow {
  std::vector<graphLayer> layer;
  unsigned short width;
  unsigned short height;

  graph(std::string _name = "") : originWindow(_name), width(0), height(0){};
};

struct note : public originWindow {
  std::string text;
  note(std::string _name = "") : originWindow(_name){};
};

struct projectNode {
  int type; // 0 - object, 1 - folder
  std::string name;
  double creation_date;     // Julian date/time
  double modification_date; // Julian date/time

  projectNode(std::string _name = "", int _type = 0,
              double _creation_date = 0.0, double _modification_date = 0.0)
      : type(_type), name(_name), creation_date(_creation_date),
        modification_date(_modification_date){};
};

class OPJFile {
public:
  OPJFile(const char *filename);
  ~OPJFile() {
    for (unsigned int g = 0; g < GRAPH.size(); ++g)
      for (unsigned int l = 0; l < GRAPH[g].layer.size(); ++l)
        for (unsigned int b = 0; b < GRAPH[g].layer[l].bitmaps.size(); ++b)
          if (GRAPH[g].layer[l].bitmaps[b].size > 0)
            delete GRAPH[g].layer[l].bitmaps[b].data;
  }
  int Parse();
  double Version() const {
    return version / 100.0;
  } //!< get version of project file

  const tree<projectNode> *project() const { return &projectTree; }
  // spreadsheet properties
  int numSpreads() const {
    return static_cast<int>(SPREADSHEET.size());
  } //!< get number of spreadsheets
  const char *spreadName(int s) const {
    return SPREADSHEET[s].name.c_str();
  } //!< get name of spreadsheet s
  bool spreadHidden(int s) const {
    return SPREADSHEET[s].bHidden;
  } //!< is spreadsheet s hidden
  bool spreadLoose(int s) const {
    return SPREADSHEET[s].bLoose;
  } //!< is spreadsheet s loose
  rect spreadWindowRect(int s) const {
    return SPREADSHEET[s].clientRect;
  } //!< get window rectangle of spreadsheet s
  const char *spreadLabel(int s) const {
    return SPREADSHEET[s].label.c_str();
  } //!< get label of spreadsheet s
  double spreadCreationDate(int s) const {
    return SPREADSHEET[s].creation_date;
  } //!< get creation date of spreadsheet s
  double spreadModificationDate(int s) const {
    return SPREADSHEET[s].modification_date;
  } //!< get modification date of spreadsheet s
  originWindow::State spreadState(int s) const {
    return SPREADSHEET[s].state;
  } //!< get window state of spreadsheet s
  originWindow::Title spreadTitle(int s) const {
    return SPREADSHEET[s].title;
  } //!< get window state of spreadsheet s
  int numCols(int s) const {
    return static_cast<int>(SPREADSHEET[s].column.size());
  } //!< get number of columns of spreadsheet s
  int numRows(int s, int c) const {
    return static_cast<int>(SPREADSHEET[s].column[c].odata.size());
  } //!< get number of rows of column c of spreadsheet s
  int maxRows(int s) const {
    return SPREADSHEET[s].maxRows;
  } //!< get maximum number of rows of spreadsheet s

  // spreadsheet's column properties
  const char *colName(int s, int c) const {
    return SPREADSHEET[s].column[c].name.c_str();
  } //!< get name of column c of spreadsheet s
  ColumnType colType(int s, int c) const {
    return SPREADSHEET[s].column[c].type;
  } //!< get type of column c of spreadsheet s
  const char *colCommand(int s, int c) const {
    return SPREADSHEET[s].column[c].command.c_str();
  } //!< get command of column c of spreadsheet s
  const char *colComment(int s, int c) const {
    return SPREADSHEET[s].column[c].comment.c_str();
  } //!< get comment of column c of spreadsheet s
  int colValueType(int s, int c) const {
    return SPREADSHEET[s].column[c].value_type;
  } //!< get value type of column c of spreadsheet s
  int colValueTypeSpec(int s, int c) const {
    return SPREADSHEET[s].column[c].value_type_specification;
  } //!< get value type specification of column c of spreadsheet s
  int colSignificantDigits(int s, int c) const {
    return SPREADSHEET[s].column[c].significant_digits;
  } //!< get significant digits of column c of spreadsheet s
  int colDecPlaces(int s, int c) const {
    return SPREADSHEET[s].column[c].decimal_places;
  } //!< get decimal places of column c of spreadsheet s
  int colNumDisplayType(int s, int c) const {
    return SPREADSHEET[s].column[c].numeric_display_type;
  } //!< get numeric display type of column c of spreadsheet s
  int colWidth(int s, int c) const {
    return SPREADSHEET[s].column[c].width;
  } //!< get width of column c of spreadsheet s
  void *oData(int s, int c, int r, bool alwaysDouble = false) const {
    if (alwaysDouble)
      return (void *)const_cast<double *>(&SPREADSHEET[s].column[c].odata[r].d);
    if (SPREADSHEET[s].column[c].odata[r].type == 0)
      return (void *)const_cast<double *>(&SPREADSHEET[s].column[c].odata[r].d);
    else
      return (void *)const_cast<char *>(
          SPREADSHEET[s].column[c].odata[r].s.c_str());
  } //!< get data of column c/row r of spreadsheet s

  // matrix properties
  int numMatrices() const {
    return static_cast<int>(MATRIX.size());
  } //!< get number of matrices
  const char *matrixName(int m) const {
    return MATRIX[m].name.c_str();
  } //!< get name of matrix m
  bool matrixHidden(int m) const {
    return MATRIX[m].bHidden;
  } //!< is matrix m hidden
  rect matrixWindowRect(int m) const {
    return MATRIX[m].clientRect;
  } //!< get window rectangle of matrix m
  const char *matrixLabel(int m) const {
    return MATRIX[m].label.c_str();
  } //!< get label of matrix m
  double matrixCreationDate(int m) const {
    return MATRIX[m].creation_date;
  } //!< get creation date of matrix m
  double matrixModificationDate(int m) const {
    return MATRIX[m].modification_date;
  } //!< get modification date of matrix m
  originWindow::State matrixState(int m) const {
    return MATRIX[m].state;
  } //!< get window state of matrix m
  originWindow::Title matrixTitle(int m) const {
    return MATRIX[m].title;
  } //!< get window state of matrix m
  int numMatrixCols(int m) const {
    return MATRIX[m].nr_cols;
  } //!< get number of columns of matrix m
  int numMatrixRows(int m) const {
    return MATRIX[m].nr_rows;
  } //!< get number of rows of matrix m
  const char *matrixFormula(int m) const {
    return MATRIX[m].command.c_str();
  } //!< get formula of matrix m
  int matrixValueTypeSpec(int m) const {
    return MATRIX[m].value_type_specification;
  } //!< get value type specification of matrix m
  int matrixSignificantDigits(int m) const {
    return MATRIX[m].significant_digits;
  } //!< get significant digits of matrix m
  int matrixDecPlaces(int m) const {
    return MATRIX[m].decimal_places;
  } //!< get decimal places of matrix m
  int matrixNumDisplayType(int m) const {
    return MATRIX[m].numeric_display_type;
  } //!< get numeric display type of matrix m
  int matrixWidth(int m) const {
    return MATRIX[m].width;
  } //!< get width of matrix m
  matrix::ViewType matrixViewType(int m) const {
    return MATRIX[m].view;
  } //!< get view type of matrix m
  matrix::HeaderViewType matrixHeaderViewType(int m) const {
    return MATRIX[m].header;
  } //!< get header view type of matrix m
  double matrixData(int m, int c, int r) const {
    return MATRIX[m].data[r * MATRIX[m].nr_cols + c];
  } //!< get data of row r of column c of matrix m
  std::vector<double> matrixData(int m) const {
    return MATRIX[m].data;
  } //!< get data of matrix m

  // function properties
  int numFunctions() const {
    return static_cast<int>(FUNCTION.size());
  } //!< get number of functions
  int functionIndex(const char *s) const {
    return compareFunctionnames(s);
  } //!< get name of function s
  const char *functionName(int s) const {
    return FUNCTION[s].name.c_str();
  } //!< get name of function s
  int functionType(int s) const {
    return FUNCTION[s].type;
  } //!< get type of function s
  double functionBegin(int s) const {
    return FUNCTION[s].begin;
  } //!< get begin of interval of function s
  double functionEnd(int s) const {
    return FUNCTION[s].end;
  } //!< get end of interval of function s
  int functionPoints(int s) const {
    return FUNCTION[s].points;
  } //!< get number of points in interval of function s
  const char *functionFormula(int s) const {
    return FUNCTION[s].formula.c_str();
  } //!< get formula of function s

  // graph properties
  enum Color {
    Black = 0,
    Red = 1,
    Green = 2,
    Blue = 3,
    Cyan = 4,
    Magenta = 5,
    Yellow = 6,
    DarkYellow = 7,
    Navy = 8,
    Purple = 9,
    Wine = 10,
    Olive = 11,
    DarkCyan = 12,
    Royal = 13,
    Orange = 14,
    Violet = 15,
    Pink = 16,
    White = 17,
    LightGray = 18,
    Gray = 19,
    LTYellow = 20,
    LTCyan = 21,
    LTMagenta = 22,
    DarkGray = 23,
    Custom = 255
  };

  enum Plot {
    Line = 200,
    Scatter = 201,
    LineSymbol = 202,
    Column = 203,
    Area = 204,
    HiLoClose = 205,
    Box = 206,
    ColumnFloat = 207,
    Vector = 208,
    PlotDot = 209,
    Wall3D = 210,
    Ribbon3D = 211,
    Bar3D = 212,
    ColumnStack = 213,
    AreaStack = 214,
    Bar = 215,
    BarStack = 216,
    FlowVector = 218,
    Histogram = 219,
    MatrixImage = 220,
    Pie = 225,
    Contour = 226,
    Unknown = 230,
    ErrorBar = 231,
    TextPlot = 232,
    XErrorBar = 233,
    SurfaceColorMap = 236,
    SurfaceColorFill = 237,
    SurfaceWireframe = 238,
    SurfaceBars = 239,
    Line3D = 240,
    Text3D = 241,
    Mesh3D = 242,
    XYZTriangular = 245,
    LineSeries = 246,
    YErrorBar = 254,
    XYErrorBar = 255,
    GraphScatter3D = 0x8AF0,
    GraphTrajectory3D = 0x8AF1,
    Polar = 0x00020000,
    SmithChart = 0x00040000,
    FillArea = 0x00800000
  };

  enum LineStyle {
    Solid = 0,
    Dash = 1,
    Dot = 2,
    DashDot = 3,
    DashDotDot = 4,
    ShortDash = 5,
    ShortDot = 6,
    ShortDashDot = 7
  };

  enum LineConnect {
    NoLine = 0,
    Straight = 1,
    TwoPointSegment = 2,
    ThreePointSegment = 3,
    BSpline = 8,
    Spline = 9,
    StepHorizontal = 11,
    StepVertical = 12,
    StepHCenter = 13,
    StepVCenter = 14,
    Bezier = 15
  };

  enum Scale {
    Linear = 0,
    Log10 = 1,
    Probability = 2,
    Probit = 3,
    Reciprocal = 4,
    OffsetReciprocal = 5,
    Logit = 6,
    Ln = 7,
    Log2 = 8
  };

  enum ValueType {
    Numeric = 0,
    Text = 1,
    Time = 2,
    Date = 3,
    Month = 4,
    Day = 5,
    ColumnHeading = 6,
    TickIndexedDataset = 7,
    TextNumeric = 9,
    Categorical = 10
  };

  enum BorderType {
    BlackLine = 0,
    Shadow = 1,
    DarkMarble = 2,
    WhiteOut = 3,
    BlackOut = 4,
    None = -1
  };

  enum Attach { Frame = 0, Page = 1, Scale = 2 };

  enum VectorPosition { Tail, Midpoint, Head };

  int numGraphs() const {
    return static_cast<int>(GRAPH.size());
  } //!< get number of graphs
  const char *graphName(int s) const {
    return GRAPH[s].name.c_str();
  } //!< get name of graph s
  const char *graphLabel(int s) const {
    return GRAPH[s].label.c_str();
  } //!< get name of graph s
  double graphCreationDate(int s) const {
    return GRAPH[s].creation_date;
  } //!< get creation date of graph s
  double graphModificationDate(int s) const {
    return GRAPH[s].modification_date;
  } //!< get modification date of graph s
  originWindow::State graphState(int s) const {
    return GRAPH[s].state;
  } //!< get window state of graph s
  originWindow::Title graphTitle(int s) const {
    return GRAPH[s].title;
  } //!< get window state of graph s
  bool graphHidden(int s) const {
    return GRAPH[s].bHidden;
  } //!< is graph s hidden
  rect graphRect(int s) const {
    return rect(GRAPH[s].width, GRAPH[s].height);
  } //!< get rectangle of graph s
  rect graphWindowRect(int s) const {
    return GRAPH[s].clientRect;
  } //!< get window rectangle of graph s
  int numLayers(int s) const {
    return static_cast<int>(GRAPH[s].layer.size());
  } //!< get number of layers of graph s
  rect layerRect(int s, int l) const {
    return GRAPH[s].layer[l].clientRect;
  } //!< get rectangle of layer l of graph s
  text layerXAxisTitle(int s, int l) const {
    return GRAPH[s].layer[l].xAxis.label;
  } //!< get label of X-axis of layer l of graph s
  text layerYAxisTitle(int s, int l) const {
    return GRAPH[s].layer[l].yAxis.label;
  } //!< get label of Y-axis of layer l of graph s
  text layerLegend(int s, int l) const {
    return GRAPH[s].layer[l].legend;
  } //!< get legend of layer l of graph s
  std::vector<text> layerTexts(int s, int l) const {
    return GRAPH[s].layer[l].texts;
  } //!< get texts of layer l of graph s
  std::vector<line> layerLines(int s, int l) const {
    return GRAPH[s].layer[l].lines;
  } //!< get lines of layer l of graph s
  std::vector<bitmap> layerBitmaps(int s, int l) const {
    return GRAPH[s].layer[l].bitmaps;
  } //!< get bitmaps of layer l of graph s
  graphAxisBreak layerXBreak(int s, int l) const {
    return GRAPH[s].layer[l].xAxisBreak;
  } //!< get break of horizontal axis of layer l of graph s
  graphAxisBreak layerYBreak(int s, int l) const {
    return GRAPH[s].layer[l].yAxisBreak;
  } //!< get break of vertical axis of layer l of graph s
  graphLayerRange layerXRange(int s, int l) const {
    return graphLayerRange(GRAPH[s].layer[l].xAxis.min,
                           GRAPH[s].layer[l].xAxis.max,
                           GRAPH[s].layer[l].xAxis.step);
  } //!< get X-range of layer l of graph s
  graphLayerRange layerYRange(int s, int l) const {
    return graphLayerRange(GRAPH[s].layer[l].yAxis.min,
                           GRAPH[s].layer[l].yAxis.max,
                           GRAPH[s].layer[l].yAxis.step);
  } //!< get Y-range of layer l of graph s
  std::vector<int> layerXTicks(int s, int l) const {
    std::vector<int> tick;
    tick.push_back(GRAPH[s].layer[l].xAxis.majorTicks);
    tick.push_back(GRAPH[s].layer[l].xAxis.minorTicks);
    return tick;
  } //!< get X-axis ticks of layer l of graph s
  std::vector<int> layerYTicks(int s, int l) const {
    std::vector<int> tick;
    tick.push_back(GRAPH[s].layer[l].yAxis.majorTicks);
    tick.push_back(GRAPH[s].layer[l].yAxis.minorTicks);
    return tick;
  } //!< get Y-axis ticks of layer l of graph s
  std::vector<graphGrid> layerGrid(int s, int l) const {
    std::vector<graphGrid> grid;
    grid.push_back(GRAPH[s].layer[l].xAxis.majorGrid);
    grid.push_back(GRAPH[s].layer[l].xAxis.minorGrid);
    grid.push_back(GRAPH[s].layer[l].yAxis.majorGrid);
    grid.push_back(GRAPH[s].layer[l].yAxis.minorGrid);
    return grid;
  } //!< get grid of layer l of graph s
  std::vector<graphAxisFormat> layerAxisFormat(int s, int l) const {
    std::vector<graphAxisFormat> format;
    format.push_back(GRAPH[s].layer[l].yAxis.formatAxis[0]); // bottom
    format.push_back(GRAPH[s].layer[l].yAxis.formatAxis[1]); // top
    format.push_back(GRAPH[s].layer[l].xAxis.formatAxis[0]); // left
    format.push_back(GRAPH[s].layer[l].xAxis.formatAxis[1]); // right
    return format;
  } //!< get title and format of axes of layer l of graph s
  std::vector<graphAxisTick> layerAxisTickLabels(int s, int l) const {
    std::vector<graphAxisTick> tick;
    tick.push_back(GRAPH[s].layer[l].yAxis.tickAxis[0]); // bottom
    tick.push_back(GRAPH[s].layer[l].yAxis.tickAxis[1]); // top
    tick.push_back(GRAPH[s].layer[l].xAxis.tickAxis[0]); // left
    tick.push_back(GRAPH[s].layer[l].xAxis.tickAxis[1]); // right
    return tick;
  } //!< get tick labels of axes of layer l of graph s
  std::vector<double> layerHistogram(int s, int l) const {
    std::vector<double> range;
    range.push_back(GRAPH[s].layer[l].histogram_bin);
    range.push_back(GRAPH[s].layer[l].histogram_begin);
    range.push_back(GRAPH[s].layer[l].histogram_end);
    return range;
  } //!< get histogram bin of layer l of graph s
  int layerXScale(int s, int l) const {
    return GRAPH[s].layer[l].xAxis.scale;
  } //!< get scale of X-axis of layer l of graph s
  int layerYScale(int s, int l) const {
    return GRAPH[s].layer[l].yAxis.scale;
  } //!< get scale of Y-axis of layer l of graph s
  int numCurves(int s, int l) const {
    return static_cast<int>(GRAPH[s].layer[l].curve.size());
  } //!< get number of curves of layer l of graph s
  const char *curveDataName(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].dataName.c_str();
  } //!< get data source name of curve c of layer l of graph s
  const char *curveXColName(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].xColName.c_str();
  } //!< get X-column name of curve c of layer l of graph s
  const char *curveYColName(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].yColName.c_str();
  } //!< get Y-column name of curve c of layer l of graph s
  int curveType(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].type;
  } //!< get type of curve c of layer l of graph s
  int curveLineStyle(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].line_style;
  } //!< get line style of curve c of layer l of graph s
  int curveLineColor(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].line_color;
  } //!< get line color of curve c of layer l of graph s
  int curveLineConnect(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].line_connect;
  } //!< get line connect of curve c of layer l of graph s
  double curveLineWidth(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].line_width;
  } //!< get line width of curve c of layer l of graph s

  bool curveIsFilledArea(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].fillarea;
  } //!< get is filled area of curve c of layer l of graph s
  int curveFillAreaColor(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].fillarea_color;
  } //!< get area fillcolor of curve c of layer l of graph s
  int curveFillAreaFirstColor(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].fillarea_first_color;
  } //!< get area first fillcolor of curve c of layer l of graph s
  int curveFillPattern(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].fillarea_pattern;
  } //!< get fill pattern of curve c of layer l of graph s
  int curveFillPatternColor(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].fillarea_pattern_color;
  } //!< get fill pattern color of curve c of layer l of graph s
  double curveFillPatternWidth(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].fillarea_pattern_width;
  } //!< get fill pattern line width of curve c of layer l of graph s
  int curveFillPatternBorderStyle(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].fillarea_pattern_border_style;
  } //!< get fill pattern border style of curve c of layer l of graph s
  int curveFillPatternBorderColor(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].fillarea_pattern_border_color;
  } //!< get fill pattern border color of curve c of layer l of graph s
  double curveFillPatternBorderWidth(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].fillarea_pattern_border_width;
  } //!< get fill pattern border line width of curve c of layer l of graph s

  int curveSymbolType(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].symbol_type;
  } //!< get symbol type of curve c of layer l of graph s
  int curveSymbolColor(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].symbol_color;
  } //!< get symbol color of curve c of layer l of graph s
  int curveSymbolFillColor(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].symbol_fill_color;
  } //!< get symbol fill color of curve c of layer l of graph s
  double curveSymbolSize(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].symbol_size;
  } //!< get symbol size of curve c of layer l of graph s
  int curveSymbolThickness(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].symbol_thickness;
  } //!< get symbol thickness of curve c of layer l of graph s

  pieProperties curvePieProperties(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].pie;
  } //!< get pie properties of curve c of layer l of graph s
  vectorProperties curveVectorProperties(int s, int l, int c) const {
    return GRAPH[s].layer[l].curve[c].vector;
  } //!< get vector properties of curve c of layer l of graph s

  int numNotes() const {
    return static_cast<int>(NOTE.size());
  } //!< get number of notes
  const char *noteName(int n) const {
    return NOTE[n].name.c_str();
  } //!< get name of note n
  const char *noteLabel(int n) const {
    return NOTE[n].label.c_str();
  } //!< get label of note n
  const char *noteText(int n) const {
    return NOTE[n].text.c_str();
  } //!< get text of note n
  double noteCreationDate(int n) const {
    return NOTE[n].creation_date;
  } //!< get creation date of note n
  double noteModificationDate(int n) const {
    return NOTE[n].modification_date;
  } //!< get modification date of note n
  originWindow::State noteState(int n) const {
    return NOTE[n].state;
  } //!< get window state of note n
  originWindow::Title noteTitle(int n) const {
    return NOTE[n].title;
  } //!< get window state of note n

  const char *resultsLogString() const {
    return resultsLog.c_str();
  } //!< get Results Log

private:
  bool IsBigEndian();
  void ByteSwap(unsigned char *b, int n);
  int ParseFormatOld();
  int ParseFormatNew();
  int compareSpreadnames(char *sname) const; //!< returns matching spread index
  int compareExcelnames(char *sname) const;  //!< returns matching excel index
  int compareColumnnames(int spread,
                         char *sname) const; //!< returns matching column index
  int compareExcelColumnnames(int excel, int sheet, char *sname)
      const;                                 //!< returns matching column index
  int compareMatrixnames(char *sname) const; //!< returns matching matrix index
  int compareFunctionnames(
      const char *sname) const; //!< returns matching function index
  std::vector<std::string> findDataByIndex(int index) const;
  std::string findObjectByIndex(int index);
  void readSpreadInfo(FILE *fopj, int file_size, FILE *fdebug);
  void readExcelInfo(FILE *f, int file_size, FILE *debug);
  void readMatrixInfo(FILE *fopj, int file_size, FILE *fdebug);
  void readGraphInfo(FILE *fopj, int file_size, FILE *fdebug);
  void readGraphGridInfo(graphGrid &grid, FILE *fopj, FILE *debug, int pos);
  void readGraphAxisBreakInfo(graphAxisBreak &axis_break, FILE *fopj,
                              FILE *debug, int pos);
  void readGraphAxisFormatInfo(graphAxisFormat &format, FILE *fopj, FILE *debug,
                               int pos);
  void readGraphAxisTickLabelsInfo(graphAxisTick &tick, FILE *fopj, FILE *debug,
                                   int pos);
  void readProjectTree(FILE *f, FILE *debug);
  void readProjectTreeFolder(FILE *f, FILE *debug,
                             tree<projectNode>::iterator parent);
  void readWindowProperties(originWindow &window, FILE *f, FILE *debug, int POS,
                            int headersize);
  void skipObjectInfo(FILE *fopj, FILE *fdebug);
  void setColName(
      int spread); //!< set default column name starting from spreadsheet spread
  void convertSpreadToExcel(int spread);
  const char *filename; //!< project file name
  int version;          //!< project version
  int dataIndex;
  int objectIndex;
  std::string resultsLog;
  std::vector<spreadSheet> SPREADSHEET;
  std::vector<matrix> MATRIX;
  std::vector<excel> EXCEL;
  std::vector<function> FUNCTION;
  std::vector<graph> GRAPH;
  std::vector<note> NOTE;
  tree<projectNode> projectTree;
};

#endif // OPJFILE_H
