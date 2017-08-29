#ifndef MPLFIGURECANVAS_H
#define MPLFIGURECANVAS_H
#include "MantidQtWidgets/MplCpp/DllOption.h"
#include <QWidget>

#include <tuple>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/// Defines the geometry of the canvas
struct EXPORT_OPT_MANTIDQT_MPLCPP SubPlotSpec {
  SubPlotSpec(long rows, long cols) : nrows(rows), ncols(cols) {}
  // These are long to match Python ints so we can avoid some casts
  long nrows;
  long ncols;
};

/**
 * @brief Non-member equality operator
 * @param lhs Left-hand side object
 * @param rhs Right-hand side object
 * @return True if the number of rows/cols match
 */
inline bool operator==(const SubPlotSpec &lhs, const SubPlotSpec &rhs) {
  return lhs.nrows == rhs.nrows && lhs.ncols == rhs.ncols;
}

/**
 * Encapsulates properties related to matplotlib Axes instances
 */
struct Axes {
  enum class Label { X, Y, Title };
  enum class Scale { X, Y, Both };
};

/**
 * C++ wrapper of a matplotlib backend FigureCanvas
 *
 * It uses the Agg version of the Qt matplotlib backend that matches the
 * Qt version that the library is compiled against. It sets a tight layout
 * for the canvas so that the resize behaviour is as expected.
 *
 * This replicates the example of embedding in Python from
 * https://matplotlib.org/examples/user_interfaces/embedding_in_qt5.html
 */
class EXPORT_OPT_MANTIDQT_MPLCPP MplFigureCanvas : public QWidget {
  Q_OBJECT
public:
  MplFigureCanvas(int subplotLayout = 111, QWidget *parent = nullptr);
  ~MplFigureCanvas();

  ///@{
  ///@name Query properties
  QWidget *canvasWidget() const;
  SubPlotSpec geometry() const;
  size_t nlines() const;
  QString label(const Axes::Label type) const;
  QString scaleType(const Axes::Scale type) const;
  ///@}

  ///@{
  ///@name Canvas modification
  void draw();
  void addSubPlot(int subplotLayout);
  ///@}

  ///@{
  ///@name Lines control
  template <typename XArrayType, typename YArrayType>
  void plotLine(const XArrayType &x, const YArrayType &y, const char *format);
  void setLineColor(const size_t index, const char *color);
  void removeLine(const size_t index);
  void clearLines();
  ///@}

  ///@{
  ///@name Axis annotation
  void setLabel(const Axes::Label type, const char *label);
  inline void setLabel(const Axes::Label type, const std::string &label) {
    setLabel(type, label.c_str());
  }
  inline void setLabel(const Axes::Label type, const QString &label) {
    setLabel(type, label.toAscii().constData());
  }
  ///@}

  ///@{
  ///@name Scaling
  void setScale(const Axes::Scale axis, const char *scaleType,
                bool redraw = false);
  void rescaleToData(const Axes::Scale axis, bool redraw = false);
  ///@}

private:
  void drawNoGIL();

  // Python objects are held in an hidden type to avoid Python
  // polluting the header
  struct PyObjectHolder;
  PyObjectHolder *m_pydata;
};
}
}
}
#endif // MPLFIGURECANVAS_H
