#include "MantidQtWidgets/MplCpp/MplFigureCanvas.h"
#include "MantidQtWidgets/MplCpp/NDArray1D.h"
#include "MantidQtWidgets/MplCpp/PythonErrors.h"
#include "MantidQtWidgets/MplCpp/SipUtils.h"
#include "MantidQtWidgets/Common/PythonThreading.h"
#include "MantidQtWidgets/Common/QtCompat.h"

#include <QMouseEvent>
#include <QVBoxLayout>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
//------------------------------------------------------------------------------
// Static constants/functions
//------------------------------------------------------------------------------
#if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0) &&                                 \
    QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
// Define PyQt version and matplotlib backend
const char *PYQT_MODULE = "PyQt4";
const char *MPL_QT_BACKEND = "matplotlib.backends.backend_qt4agg";

#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) &&                               \
    QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// Define PyQt version and matplotlib backend
const char *PYQT_MODULE = "PyQt5";
const char *MPL_QT_BACKEND = "matplotlib.backends.backend_qt5agg";

#else
#error "Unknown Qt version. Cannot determine matplotlib backend."
#endif

// Return static instance of figure type
// The GIL must be held to call this
const PythonObject &mplFigureType() {
  static PythonObject figureType;
  if (figureType.isNone()) {
    figureType = getAttrOnModule("matplotlib.figure", "Figure");
  }
  return figureType;
}

// Return static instance of figure canvas type
// The GIL must be held to call this
const PythonObject &mplFigureCanvasType() {
  static PythonObject figureCanvasType;
  if (figureCanvasType.isNone()) {
    // Importing PyQt version first helps matplotlib select the correct backend.
    // We should do this in some kind of initialisation routine
    importModule(PYQT_MODULE);
    figureCanvasType = getAttrOnModule(MPL_QT_BACKEND, "FigureCanvasQTAgg");
  }
  return figureCanvasType;
}

// Return static instance of navigation toolbar type
// The GIL must be held to call this
const PythonObject &mplNavigationToolbarType() {
  static PythonObject navigationToolbarType;
  if (navigationToolbarType.isNone()) {
    navigationToolbarType =
        getAttrOnModule(MPL_QT_BACKEND, "NavigationToolbar2QT");
  }
  return navigationToolbarType;
}

/**
 * Extract a double from a given element of an array or tuple. Warning: No
 * checks are done on the validity of the object or the index
 * @param obj A reference to the sequence
 * @param i The index of interest
 * @return A C long from this element
 */
double doubleInSeq(const PythonObject &obj, Py_ssize_t i) {
  return PyFloat_AsDouble(PySequence_Fast_GET_ITEM(obj.get(), i));
}

/**
 * Extract a long from a given element of an array or tuple. Warning: No
 * checks are done on the validity of the object or the index
 * @param obj A reference to the sequence
 * @param i The index of interest
 * @return A C long from this element
 */
long longInSeq(const PythonObject &obj, Py_ssize_t i) {
  return TO_LONG(PySequence_Fast_GET_ITEM(obj.get(), i));
}
}

//------------------------------------------------------------------------------
// MplFigureCanvas::PyObjectHolder - Private implementation
//------------------------------------------------------------------------------
struct MplFigureCanvas::PyObjectHolder {
  // QtAgg canvas object
  PythonObject canvas;
  // Navigation toolbar instance
  PythonObject toolbar;
  // A pointer to the C++ data contained within the Python object
  QWidget *canvasWidget;
  // List of lines on current plot
  std::vector<PythonObject> lines;

  // constructor
  PyObjectHolder(int subplotLayout) {
    ScopedPythonGIL gil;
    // Create a figure and axes with the given layout
    auto figure = PythonObject::fromNewRef(
        PyObject_CallObject(mplFigureType().get(), NULL));
    auto axes = PythonObject::fromNewRef(
        PyObject_CallMethod(figure.get(), PYSTR_LITERAL("add_subplot"),
                            PYSTR_LITERAL("i"), subplotLayout));
    detail::decref(PyObject_CallMethod(figure.get(),
                                       PYSTR_LITERAL("set_tight_layout"),
                                       PYSTR_LITERAL("{s:f}"), "pad", 0.5));
    canvas = PythonObject::fromNewRef(PyObject_CallFunction(
        mplFigureCanvasType().get(), PYSTR_LITERAL("(O)"), figure.get()));
    canvasWidget = static_cast<QWidget *>(SipUtils::unwrap(canvas.get()));
    assert(canvasWidget);

    // Hidden toolbar - used to access the zoom functionality
    toolbar = PythonObject::fromNewRef(PyObject_CallFunction(
        mplNavigationToolbarType().get(), PYSTR_LITERAL("(OOi)"), canvas.get(),
        Py_None, 0));
    // check for _views attribute
    if (!toolbar.hasAttr("_views")) {
      throw std::logic_error("NavigationToolbar class expects an _views "
                             "attribute but none was found");
    }
  }

  /**
   * Return the Axes object that is currently active. Analogous to figure.gca()
   * @return matplotlib.axes.Axes object
   */
  PythonObject gca() {
    auto figure = canvas.getAttr("figure");
    return PythonObject::fromNewRef(PyObject_CallMethod(
        figure.get(), PYSTR_LITERAL("gca"), PYSTR_LITERAL(""), nullptr));
  }
};

//------------------------------------------------------------------------------
// MplFigureCanvas
//------------------------------------------------------------------------------
/**
 * @brief Constructs an empty plot widget with the given subplot layout.
 *
 * @param subplotLayout The sublayout geometry defined in matplotlib's
 * convenience format: [Default=111]. See
 * https://matplotlib.org/api/pyplot_api.html#matplotlib.pyplot.subplot
 *
 * @param parent A pointer to the parent widget, can be nullptr
 */
MplFigureCanvas::MplFigureCanvas(int subplotLayout, QWidget *parent)
    : QWidget(parent), m_pydata(nullptr) {
  m_pydata = new PyObjectHolder(subplotLayout);
  setLayout(new QVBoxLayout);
  layout()->addWidget(m_pydata->canvasWidget);
  m_pydata->canvasWidget->setMouseTracking(false);
  m_pydata->canvasWidget->installEventFilter(this);
}

/**
 * @brief Destroys the object
 */
MplFigureCanvas::~MplFigureCanvas() { delete m_pydata; }

/**
 * Get the "real" canvas widget. This is generally only required
 * when needing to install event filters to capture mouse events as
 * it's not possible to override the methods in Python from C++
 * @return The real canvas object
 */
QWidget *MplFigureCanvas::canvasWidget() const {
  return m_pydata->canvasWidget;
}

/**
 * Retrieve information about the subplot geometry
 * @return A SubPlotSpec object defining the geometry
 */
SubPlotSpec MplFigureCanvas::geometry() const {
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto geometry = PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL("get_geometry"), PYSTR_LITERAL(""), nullptr));

  return SubPlotSpec(longInSeq(geometry, 0), longInSeq(geometry, 1));
}

/**
 * @return True if the canvas has been zoomed
 */
bool MplFigureCanvas::isZoomed() const {
  // We have to rely on a "private" attribute of the toolbar
  // to determine if the first view (unzoomed) matches the current or not
  ScopedPythonGIL gil;
  auto views = m_pydata->toolbar.getAttr("_views");
  auto firstViewRaw = PyObject_CallMethod(
      views.get(), PYSTR_LITERAL("__getitem__"), PYSTR_LITERAL("(i)"), 0);
  // no elements indicates no zoom has taken place yet
  if (!firstViewRaw) {
    PyErr_Clear();
    return false;
  }
  // deal with the ref count properly
  auto firstView = PythonObject::fromNewRef(firstViewRaw);
  auto cur = PythonObject::fromNewRef(
      PyObject_CallMethod(views.get(), PYSTR_LITERAL("__call__"), nullptr));
  // zero indicates false comparison, i.e. cur != first so we are zoomed
  return PyObject_RichCompareBool(cur.get(), firstView.get(), Py_EQ) == 0;
}

/**
 * Get a label from the canvas
 * @param type The label type
 * @return The label on the requested axis
 */
QString MplFigureCanvas::label(const Axes::Label type) const {
  const char *method;
  if (type == Axes::Label::X)
    method = "get_xlabel";
  else if (type == Axes::Label::Y)
    method = "get_ylabel";
  else if (type == Axes::Label::Title)
    method = "get_title";
  else
    throw std::logic_error("MplFigureCanvas::getLabel() - Unknown label type.");

  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto label = PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL(method), PYSTR_LITERAL(""), nullptr));
  return QString::fromAscii(TO_CSTRING(label.get()));
}

/** Query the limits for a given axis
 * @param type An enumeration of Axes::Scale type
 * @return The limits as a tuple
 */
std::tuple<double, double>
MplFigureCanvas::limits(const Axes::Scale type) const {
  const char *method;
  if (type == Axes::Scale::X)
    method = "get_xlim";
  else if (type == Axes::Scale::Y)
    method = "get_ylim";
  else
    throw std::logic_error(
        "MplFigureCanvas::getLimits() - Scale type must be X or Y");
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto limits = PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL(method), PYSTR_LITERAL(""), nullptr));
  return std::make_tuple(doubleInSeq(limits, 0), doubleInSeq(limits, 1));
}

/**
 * @return The number of Line2Ds on the canvas
 */
size_t MplFigureCanvas::nlines() const {
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto lines = PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL("get_lines"), PYSTR_LITERAL(""), nullptr));
  return static_cast<size_t>(PyList_Size(lines.get()));
}

/**
 * Return the scale type on the given axis
 * @param type An enumeration giving the axis type
 * @return A string defining the scale type
 */
QString MplFigureCanvas::scaleType(const Axes::Scale type) const {
  const char *method;
  if (type == Axes::Scale::X)
    method = "get_xscale";
  else if (type == Axes::Scale::Y)
    method = "get_yscale";
  else
    throw std::logic_error(
        "MplFigureCanvas::getScale() - Scale type must be X or Y");
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto scale = PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL(method), PYSTR_LITERAL(""), nullptr));
  return QString::fromAscii(TO_CSTRING(scale.get()));
}

/**
 * Assume the given coordinates are Qt pixel coordinates, the result of a mouse
 * click event for example, and transform them to data coordinates
 * @param pos
 * @return A QPointF defining the data position. It can be null if the pos
 * is not within he axis limits
 */
QPointF MplFigureCanvas::toDataCoordinates(QPoint pos) const {
  // This duplicates what happens in
  // matplotlib.backends.backend_qt5.FigureCanvasQT &
  // matplotlib.backend_bases.LocationEvent where we transform first to
  // matplotlib's coordinate system, (0,0) is bottom left
  // and then to the data coordinates
  ScopedPythonGIL gil;
  // Get dpi to take into to transform to physical pixels
  auto GetDpiRatio = [](const PythonObject &canvas) {
    double dpiRatio(1.0);
    // This needs to be fast so avoid the exception throwing
    // PythonObject::getAttr
    auto result =
        PyObject_GetAttrString(canvas.get(), PYSTR_LITERAL("_dpi_ratio"));
    if (result) {
      dpiRatio = PyFloat_AsDouble(result);
      detail::decref(result);
    } else {
      PyErr_Clear();
    }
    return dpiRatio;
  };
  const auto &canvas = m_pydata->canvas;
  const double dpiRatio = GetDpiRatio(canvas);
  const double xPosPhysical = pos.x() * dpiRatio;
  // Y=0 is at the bottom
  auto pyHeight = canvas.getAttr("figure").getAttr("bbox").getAttr("height");
  double cppHeight = PyFloat_AsDouble(pyHeight.get());
  const double yPosPhysical = ((cppHeight / dpiRatio) - pos.y()) * dpiRatio;

  // Transform to data coordinates
  QPointF dataCoords;
  auto axes = m_pydata->gca();
  try {
    auto transData = axes.getAttr("transData");
    auto invTransform = PythonObject::fromNewRef(
        PyObject_CallMethod(transData.get(), PYSTR_LITERAL("inverted"),
                            PYSTR_LITERAL(""), nullptr));
    auto result = NDArray1D<double>::fromNewRef(PyObject_CallMethod(
        invTransform.get(), PYSTR_LITERAL("transform_point"),
        PYSTR_LITERAL("((ff))"), xPosPhysical, yPosPhysical));
    dataCoords =
        QPointF(static_cast<qreal>(result[0]), static_cast<qreal>(result[1]));
  } catch (PythonError &) {
    // an exception indicates no transform possible. Matplotlib sets this as
    // an empty data coordinate so we will do the same
  }
  return dataCoords;
}

/**
 * Equivalent of Figure.add_subplot. If the subplot already exists then
 * it simply sets that plot number to be active
 * @param subplotLayout Subplot geometry in matplotlib convenience format,
 * e.g 212 would stack 2 plots on top of each other and set the second
 * to active
 */
void MplFigureCanvas::addSubPlot(int subplotLayout) {
  ScopedPythonGIL gil;
  auto figure = m_pydata->canvas.getAttr("figure");
  auto result = PyObject_CallMethod(figure.get(), PYSTR_LITERAL("add_subplot"),
                                    PYSTR_LITERAL("(i)"), subplotLayout);
  if (!result)
    throw PythonError();
  detail::decref(result);
}

/**
 * Redraw everything on the canvas
 */
void MplFigureCanvas::draw() {
  ScopedPythonGIL gil;
  drawNoGIL();
}

/**
 * Resets the current view to it's original state before
 * any zoom/pan operations were called.
 */
void MplFigureCanvas::home() {
  ScopedPythonGIL gil;
  detail::decref(PyObject_CallMethod(m_pydata->toolbar.get(),
                                     PYSTR_LITERAL("home"), PYSTR_LITERAL("")));
}

/** Set the color of the canvas outside of the axes
 * @param color A matplotlib color string
 */
void MplFigureCanvas::setCanvasFaceColor(const char *color) {
  ScopedPythonGIL gil;
  setCanvasFaceColorNoGIL(color);
}

/**
 * Similar to pushing the zoom button on the standard matplotlib
 * toolbar. Toggle this once to enable zoom support
 */
void MplFigureCanvas::toggleZoomMode() {
  ScopedPythonGIL gil;
  detail::decref(PyObject_CallMethod(m_pydata->toolbar.get(),
                                     PYSTR_LITERAL("zoom"), PYSTR_LITERAL("")));
}

/**
 * Set the color of the given line
 * @param index The index of an existing line. If it does not exist then
 * this is a no-op.
 * @param color A string indicating a matplotlib color
 */
void MplFigureCanvas::setLineColor(const size_t index, const char *color) {
  auto &lines = m_pydata->lines;
  if (lines.empty() || index >= lines.size())
    return;
  ScopedPythonGIL gil;
  auto &line = lines[index];
  detail::decref(PyObject_CallMethod(line.get(), PYSTR_LITERAL("set_color"),
                                     PYSTR_LITERAL("(s)"), color));
}

/**
 * Plot lines to the current axis
 * @param x A container of X points. Requires support for forward iteration.
 * @param y A container of Y points. Requires support for forward iteration.
 * @param format A format string for the line/markers
 */
template <typename XArrayType, typename YArrayType>
void MplFigureCanvas::plotLine(const XArrayType &x, const YArrayType &y,
                               const char *format) {
  ScopedPythonGIL gil;
  NDArray1D<double> xnp(x), ynp(y);
  auto axes = m_pydata->gca();
  // This will return a list of lines but we know we are only plotting 1
  auto pylines = PythonObject::fromBorrowedRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL("plot"), PYSTR_LITERAL("(OOs)"), xnp.get(),
      ynp.get(), format));
  m_pydata->lines.emplace_back(
      PythonObject::fromBorrowedRef(PyList_GET_ITEM(pylines.get(), 0)));
}

/**
 * Remove a line from the canvas based on the index
 * @param index The index of the line to remove. If it does not exist then
 * this is a no-op.
 */
void MplFigureCanvas::removeLine(const size_t index) {
  auto &lines = m_pydata->lines;
  if (lines.empty() || index >= lines.size())
    return;
  auto posIter = std::next(std::begin(lines), index);
  auto line = *posIter;
  // .erase will cause a decrement of the reference count so we should
  // really hold the GIL at that point
  ScopedPythonGIL gil;
  lines.erase(posIter);
  detail::decref(PyObject_CallMethod(line.get(), PYSTR_LITERAL("remove"),
                                     PYSTR_LITERAL(""), nullptr));
}

/**
 * Clear the current axes of artists
 */
void MplFigureCanvas::clearLines() {
  if (m_pydata->lines.empty())
    return;
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  detail::decref(PyObject_CallMethod(axes.get(), PYSTR_LITERAL("cla"),
                                     PYSTR_LITERAL(""), nullptr));
  m_pydata->lines.clear();
}

/**
 * Set a label on the requested axis
 * @param type Type of label
 * @param label Label for the axis
 */
void MplFigureCanvas::setLabel(const Axes::Label type, const char *label) {
  const char *method;
  if (type == Axes::Label::X)
    method = "set_xlabel";
  else if (type == Axes::Label::Y)
    method = "set_ylabel";
  else if (type == Axes::Label::Title)
    method = "set_title";
  else
    throw std::logic_error("MplFigureCanvas::setLabel() - Unknown label type.");

  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto result = PyObject_CallMethod(axes.get(), PYSTR_LITERAL(method),
                                    PYSTR_LITERAL("(s)"), label);
  if (!result)
    throw PythonError();
  detail::decref(result);
}

/**
 * Set the font size of the tick labels
 * @param axis Enumeration defining the axis
 * @param size The new font size
 */
void MplFigureCanvas::setTickLabelFontSize(const Axes::Scale axis,
                                           double size) {
  const char *xAxis = "x";
  const char *yAxis = "y";
  const char *bothAxes = "both";
  const char *whichAxis = bothAxes;
  if (axis == Axes::Scale::X) {
    whichAxis = xAxis;
  } else if (axis == Axes::Scale::Y) {
    whichAxis = yAxis;
  }
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto methodName = Py_BuildValue("s", "tick_params");
  auto method = PyObject_GetAttr(axes.get(), methodName);
  auto args = Py_BuildValue("(s)", whichAxis);
  auto kwargs = Py_BuildValue("{s:d}", "labelsize", size);
  auto result = PyObject_Call(method, args, kwargs);
  if (!result)
    throw PythonError();
  detail::decref(result);
  detail::decref(kwargs);
  detail::decref(args);
  detail::decref(method);
  detail::decref(methodName);
}

/**
 * Set the scale type on an axis. Redraw is called if requested
 * @param axis Enumeration defining the axis
 * @param scaleType The type of scale
 * @param redraw If true then call Axes.draw to repaint the canvas
 */
void MplFigureCanvas::setScale(const Axes::Scale axis, const char *scaleType,
                               bool redraw) {
  auto scaleSetter =
      [](const PythonObject &axes, const char *method, const char *value) {
        auto result = PyObject_CallMethod(axes.get(), PYSTR_LITERAL(method),
                                          PYSTR_LITERAL("(s)"), value);
        if (!result)
          throw PythonError();
        detail::decref(result);
      };
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  if (axis == Axes::Scale::Both || axis == Axes::Scale::X)
    scaleSetter(axes, "set_xscale", scaleType);
  if (axis == Axes::Scale::Both || axis == Axes::Scale::Y)
    scaleSetter(axes, "set_yscale", scaleType);

  if (redraw) {
    PyObject_CallMethod(axes.get(), PYSTR_LITERAL("autoscale"),
                        PYSTR_LITERAL("(i)"), 1);
    drawNoGIL();
  }
}

/**
 * Rescale the axis limits to the data
 * @param axis Choose the axis to rescale
 * @param redraw If true then call canvas->draw() to repaint the canvas
 */
void MplFigureCanvas::rescaleToData(const Axes::Scale axis, bool redraw) {
  int scaleX(0), scaleY(0);
  switch (axis) {
  case Axes::Scale::Both:
    scaleX = 1;
    scaleY = 1;
    break;
  case Axes::Scale::X:
    scaleX = 1;
    break;
  case Axes::Scale::Y:
    scaleY = 1;
    break;
  default:
    throw std::logic_error(
        "MplFigureCanvas::rescaleToData() - Unknown Axis type.");
  };
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  detail::decref(PyObject_CallMethod(axes.get(), PYSTR_LITERAL("relim"),
                                     PYSTR_LITERAL(""), nullptr));
  if (redraw) {
    detail::decref(
        PyObject_CallMethod(axes.get(), PYSTR_LITERAL("autoscale_view"),
                            PYSTR_LITERAL("(iii)"), 1, scaleX, scaleY));
    drawNoGIL();
  }
}

/**
 * Add an arbitrary text label to the canvas
 * @param x X position in data coordinates
 * @param y Y position in data coordinates
 * @param label The string label to attach to the canvas
 */
PythonObject MplFigureCanvas::addText(double x, double y, const char *label,
                                      const char *horizontalalignment) {
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  return PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL("text"), PYSTR_LITERAL("(ffs{s:s})"), x, y,
      label, PYSTR_LITERAL("horizontalalignment"),
      PYSTR_LITERAL(horizontalalignment)));
}

/**
 * Intercept events destined for the child canvas. The Matplotlib canvas
 * defines its own event handlers for certain events. As this class
 * does not directly inherit from the matplotlib canvas we cannot use the
 * standard virtual event methods to intercept them. Instead this
 * event filter allows us to capture and process them. We simply
 * call the standard event handler functions that a widget would
 * expect to call. The toDataCoordinates() method can be used to obtain
 * the data coordinates for a given mouse location.
 * @param watched A pointer to the object being watched
 * @param evt The event dispatched to the watched object
 * @return True if the event was filtered by this function, false otherwise
 */
bool MplFigureCanvas::eventFilter(QObject *watched, QEvent *evt) {
  assert(watched == canvasWidget());
  Q_UNUSED(watched); // make release build happy
  auto type = evt->type();
  if (type == QEvent::MouseButtonPress)
    mousePressEvent(static_cast<QMouseEvent *>(evt));
  else if (type == QEvent::MouseButtonRelease)
    mouseReleaseEvent(static_cast<QMouseEvent *>(evt));
  else if (type == QEvent::MouseButtonDblClick)
    mouseDoubleClickEvent(static_cast<QMouseEvent *>(evt));

  // default doesn't filter
  return false;
}

/**
 * The implementation of draw that does NOT lock the GIL. Use with caution.
 */
void MplFigureCanvas::drawNoGIL() {
  detail::decref(PyObject_CallMethod(m_pydata->canvas.get(),
                                     PYSTR_LITERAL("draw"), PYSTR_LITERAL(""),
                                     nullptr));
}

/**
 * Set the face color of the figure. This does not lock the GIL - use
 * with caution
 * @param color A matplotlib color string
 */
void MplFigureCanvas::setCanvasFaceColorNoGIL(const char *color) {
  auto figure = PythonObject::fromNewRef(
      PyObject_GetAttrString(m_pydata->canvas.get(), PYSTR_LITERAL("figure")));
  detail::decref(
      PyObject_CallMethod(figure.get(), PYSTR_LITERAL("set_facecolor"),
                          PYSTR_LITERAL("(s)"), PYSTR_LITERAL(color)));
}

//------------------------------------------------------------------------------
// Explicit template instantations
//------------------------------------------------------------------------------
using VectorDouble = std::vector<double>;
template EXPORT_OPT_MANTIDQT_MPLCPP void
MplFigureCanvas::plotLine<VectorDouble, VectorDouble>(const VectorDouble &,
                                                      const VectorDouble &,
                                                      const char *);
}
}
}
