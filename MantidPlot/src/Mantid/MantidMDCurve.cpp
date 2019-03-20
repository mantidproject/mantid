// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDCurve.h"
#include "../ApplicationWindow.h"
#include "../Graph.h"
#include "../MultiLayer.h"
#include "ErrorBarSettings.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/IMDWorkspace.h"
#include <qpainter.h>
#include <qwt_symbol.h>

using namespace Mantid::API;
using namespace MantidQt::API;

namespace {
/// static logger
Mantid::Kernel::Logger g_log("MantidMDCurve");
} // namespace

/**
 *  @param wsName :: The workspace name.
 *  @param g :: The Graph widget which will display the curve
 *  @param err :: True if the errors are to be plotted
 *  @param distr :: True if this is a distribution
 *  @param style :: Graph style
 *  @throw std::invalid_argument if the index is out of range for the given
 * workspace
 */
MantidMDCurve::MantidMDCurve(const QString &wsName, Graph *g, bool err,
                             bool distr, GraphOptions::CurveType style)
    : MantidCurve(wsName, err), m_wsName(wsName) {
  if (!g) {
    throw std::invalid_argument(
        "MantidMDCurve::MantidMDCurve() - NULL graph pointer not allowed");
  }
  init(g, distr, style);
}

MantidMDCurve::MantidMDCurve(const MantidMDCurve &c)
    : MantidCurve(createCopyName(c.title().text()), c.m_drawErrorBars,
                  c.m_drawAllErrorBars),
      m_wsName(c.m_wsName) {
  setData(c.data());
  observePostDelete();
  connect(this, SIGNAL(resetData(const QString &)), this,
          SLOT(dataReset(const QString &)));
  observeAfterReplace();
  observeADSClear();
}

/**
 *  @param g :: The Graph widget which will display the curve
 *  @param distr :: True if this is a distribution,
 *  not applicable here.
 *  @param style :: The graph style to use
 *  @param multipleSpectra :: True if there are multiple spectra,
 *  not applicable here.
 */
void MantidMDCurve::init(Graph *g, bool distr, GraphOptions::CurveType style,
                         bool multipleSpectra) {
  UNUSED_ARG(distr);
  UNUSED_ARG(multipleSpectra);
  IMDWorkspace_const_sptr ws = boost::dynamic_pointer_cast<IMDWorkspace>(
      AnalysisDataService::Instance().retrieve(m_wsName.toStdString()));
  if (!ws) {
    std::string message =
        "Could not extract IMDWorkspace of name: " + m_wsName.toStdString();
    throw std::runtime_error(message);
  }
  if (ws->getNonIntegratedDimensions().size() != 1) {
    std::string message = "This plot only applies to MD Workspaces with a "
                          "single expanded dimension";
    throw std::invalid_argument(message);
  }

  this->setTitle(m_wsName + "-signal");

  const bool log = g->isLog(QwtPlot::yLeft);
  MantidQwtIMDWorkspaceData data(ws, log);
  setData(data);

  int lineWidth = 1;
  MultiLayer *ml = dynamic_cast<MultiLayer *>(g->parent()->parent()->parent());
  if (ml && (style == GraphOptions::Unspecified ||
             ml->applicationWindow()->applyCurveStyleToMantid)) {
    // FIXME: Style HorizontalSteps does NOT seem to be applied
    applyStyleChoice(style, ml, lineWidth);
  } else {
    setStyle(QwtPlotCurve::Lines);
  }
  g->insertCurve(this, lineWidth);

  // set the option to draw all error bars from the global settings
  if (hasErrorBars()) {
    setErrorBars(true, g->multiLayer()->applicationWindow()->drawAllErrors);
  }
  // Initialise error bar colour to match curve colour
  m_errorSettings->m_color = pen().color();
  m_errorSettings->setWidth(pen().widthF());

  connect(g, SIGNAL(axisScaleChanged(int, bool)), this,
          SLOT(axisScaleChanged(int, bool)));
  observePostDelete();
  connect(this, SIGNAL(resetData(const QString &)), this,
          SLOT(dataReset(const QString &)));
  observeAfterReplace();
  observeADSClear();
}

MantidMDCurve::~MantidMDCurve() {
  observePostDelete(false);
  observeAfterReplace(false);
  observeADSClear(false);
}

/**
 * Clone the curve for the use by a particular Graph
 */
MantidMDCurve *MantidMDCurve::clone(const Graph * /*g*/) const {
  MantidMDCurve *mc = new MantidMDCurve(*this); /*
   if (g)
   {
     mc->setDrawAsDistribution(g->isDistribution());
   }*/
  return mc;
}

void MantidMDCurve::setData(const QwtData &data) {
  if (!dynamic_cast<const MantidQwtIMDWorkspaceData *>(&data))
    throw std::runtime_error(
        "Only MantidQwtIMDWorkspaceData can be set to a MantidMDCurve");
  PlotCurve::setData(data);
}

QwtDoubleRect MantidMDCurve::boundingRect() const {
  return MantidCurve::boundingRect();
}

void MantidMDCurve::draw(QPainter *p, const QwtScaleMap &xMap,
                         const QwtScaleMap &yMap, const QRect &rect) const {
  PlotCurve::draw(p, xMap, yMap, rect);

  if (m_drawErrorBars) // drawing error bars
  {
    const MantidQwtIMDWorkspaceData *d =
        dynamic_cast<const MantidQwtIMDWorkspaceData *>(&data());
    if (!d) {
      throw std::runtime_error(
          "Only MantidQwtIMDWorkspaceData can be set to a MantidMDCurve");
    }
    doDraw(p, xMap, yMap, rect, d);
  }
}

/**  Resets the data if wsName is the name of this workspace
 *  @param wsName :: The name of a workspace which data has been changed in the
 * data service.
 */
void MantidMDCurve::dataReset(const QString &wsName) {
  if (m_wsName != wsName)
    return;
  const std::string wsNameStd = wsName.toStdString();
  Mantid::API::IMDWorkspace_sptr mws;
  try {
    Mantid::API::Workspace_sptr base =
        Mantid::API::AnalysisDataService::Instance().retrieve(wsNameStd);
    mws = boost::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(base);
  } catch (std::runtime_error &) {
    g_log.information() << "Workspace " << wsNameStd
                        << " could not be found - plotted curve(s) deleted\n";
    mws = Mantid::API::IMDWorkspace_sptr();
  }

  if (!mws)
    return;
  const MantidQwtIMDWorkspaceData *new_mantidData(nullptr);
  try {
    new_mantidData = mantidData()->copy(mws);
    setData(*new_mantidData);
    setStyle(QwtPlotCurve::Lines);
    // Queue this plot to be updated once all MantidQwtIMDWorkspaceData objects
    // for this workspace have been
    emit dataUpdated();
  } catch (std::range_error &) {
    // Get here if the new workspace has fewer spectra and the plotted one no
    // longer exists
    g_log.information()
        << "Workspace " << wsNameStd
        << " now has fewer spectra - plotted curve(s) deleted\n";
    postDeleteHandle(wsNameStd);
  }
  delete new_mantidData;
}

/* This method saves the curve details to a string.
 * Useful for loading/saving mantid project.
 */
QString MantidMDCurve::saveToString() {
  QString s;
  s = "MantidMDCurve\t" + m_wsName + "\t" + QString::number(m_drawErrorBars) +
      "\n";
  return s;
}

void MantidMDCurve::afterReplaceHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  (void)ws;

  invalidateBoundingRect();
  emit resetData(QString::fromStdString(wsName));
}

MantidQwtIMDWorkspaceData *MantidMDCurve::mantidData() {
  MantidQwtIMDWorkspaceData *d =
      dynamic_cast<MantidQwtIMDWorkspaceData *>(&data());
  return d;
}

const MantidQwtIMDWorkspaceData *MantidMDCurve::mantidData() const {
  const MantidQwtIMDWorkspaceData *d =
      dynamic_cast<const MantidQwtIMDWorkspaceData *>(&data());
  return d;
}
