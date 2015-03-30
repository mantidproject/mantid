#include "MantidVatesSimpleGuiViewWidgets/PeaksTableControllerVsi.h"

#include "MantidVatesSimpleGuiViewWidgets/PeaksTabWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/CameraManager.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/PeakTransformHKL.h"
#include "MantidAPI/PeakTransformQSample.h"
#include "MantidAPI/PeakTransformQLab.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/Logger.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidVatesAPI/NullPeaksPresenterVsi.h"
#include "MantidVatesAPI/ConcretePeaksPresenterVsi.h"
#include "MantidVatesAPI/CompositePeaksPresenterVsi.h"
#include "MantidQtSliceViewer/PeakPalette.h"
#include "MantidQtAPI/PlotAxis.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
#pragma warning disable 1170
#endif
#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>
#include <pqPipelineRepresentation.h>
#include <pqRenderView.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMDoubleVectorProperty.h>

#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif

#include <QString>
#include <QPointer>
#include <QVBoxLayout>
#include <QLayout>
#include <QLayoutItem>
#include <QColor>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <sstream>

namespace Mantid {
namespace Vates {
namespace SimpleGui {

namespace {
Mantid::Kernel::Logger g_log("PeakViewerVsi");
}

/**
 * Constructor
 * @param cameraManager A cameraManager pointer.
 * @param parent A pointer to a QWidget parent.
 */
PeaksTableControllerVsi::PeaksTableControllerVsi(
    boost::shared_ptr<CameraManager> cameraManager, QWidget *parent)
    : QWidget(parent), m_cameraManager(cameraManager),
      m_presenter(new Mantid::VATES::CompositePeaksPresenterVsi()),
      m_peaksTabWidget(NULL), m_peakMarker(NULL) {
  m_peakTransformSelector.registerCandidate(
      boost::make_shared<Mantid::API::PeakTransformHKLFactory>());
  m_peakTransformSelector.registerCandidate(
      boost::make_shared<Mantid::API::PeakTransformQSampleFactory>());
  m_peakTransformSelector.registerCandidate(
      boost::make_shared<Mantid::API::PeakTransformQLabFactory>());
}

PeaksTableControllerVsi::~PeaksTableControllerVsi() {
  destroySinglePeakSource();
}

/**
 * Check for viewable peaks.
 * @returns A vector of the peak indices which are visible and which are not
 * visible.
 */
std::vector<bool> PeaksTableControllerVsi::getViewablePeaks() {
  std::vector<bool> viewablePeaks;
  if (m_presenter) {
    // Get the up to date area
    updateViewableArea();

    // Get a list with viewable peak coordinates
    try {
      viewablePeaks = m_presenter->getViewablePeaks();
    } catch (...) {
      g_log.warning() << "The viewable peaks could not be retrieved. \n";
    }
  }
  return viewablePeaks;
}

/**
 * Add a new workspace
 * @param source A new peaks source
 * @param splatSource A pointer to the splatter source
 */
void PeaksTableControllerVsi::addWorkspace(
    pqPipelineSource *source, QPointer<pqPipelineSource> splatSource) {
  try {
    if (!source || !splatSource) {
      throw std::invalid_argument(
          "The pqPipelineSource of the peaks workspace does not exist.");
    }

    // Get the pointer to the peaks workspace
    std::string wsName(vtkSMPropertyHelper(source->getProxy(), "WorkspaceName",
                                           true).GetAsString());
    std::string peaksFrame(vtkSMPropertyHelper(source->getProxy(),
                                               "Peak Dimensions",
                                               true).GetAsString());

    // Get dimensions from splattersource
    std::vector<std::string> dimInfo = extractFrameFromSource(splatSource);
    if (dimInfo.size() < 2) {
      throw std::invalid_argument(
          "The workspace needs to have at least two dimensions");
    }

    std::string dimCompare = dimInfo[0];
    std::transform(dimCompare.begin(), dimCompare.end(), dimCompare.begin(),
                   ::toupper);
    std::transform(peaksFrame.begin(), peaksFrame.end(), peaksFrame.begin(),
                   ::toupper);
    // Check if frames match
    if (dimCompare.find(peaksFrame) == std::string::npos) {
      throw std::runtime_error("The workspaces do not match.");
    }

    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::IPeaksWorkspace>(wsName);

    Mantid::API::PeakTransformFactory_sptr transformFactory =
        m_peakTransformSelector.makeChoice(dimInfo[0], dimInfo[1]);
    Mantid::API::PeakTransform_sptr transform =
        transformFactory->createTransform(dimInfo[0], dimInfo[1]);
    std::string frame = transform->getFriendlyName();

    m_presenter->addPresenter(
        boost::make_shared<Mantid::VATES::ConcretePeaksPresenterVsi>(
            peaksWorkspace, m_cameraManager->getCurrentViewFrustum(), frame));

    // If the tab widget is visible, then update it
    if (m_peaksTabWidget) {
      std::map<std::string, std::vector<bool>> viewablePeaks =
          m_presenter->getInitializedViewablePeaks();
      m_peaksTabWidget->addNewPeaksWorkspace(
          peaksWorkspace, viewablePeaks[peaksWorkspace->getName()]);
      m_peaksTabWidget->updateTabs(viewablePeaks, getColors());
      updatePeakWorkspaceColor();
    }
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    g_log.warning() << "Could not retrieve the peaks workspace.\n";
    throw;
  } catch (std::invalid_argument &ex) {
    g_log.warning() << ex.what();
    throw;
  } catch (std::runtime_error &ex) {
    g_log.warning() << ex.what();
    throw;
  }
}

/**
 * Get the colors for the tabs
 * @returns A map of peak workspace names and colors
 */
std::map<std::string, QColor> PeaksTableControllerVsi::getColors() {

  std::map<std::string, QColor> colors;

  std::vector<Mantid::API::IPeaksWorkspace_sptr> peakWs =
      m_presenter->getPeaksWorkspaces();
  for (std::vector<Mantid::API::IPeaksWorkspace_sptr>::iterator it =
           peakWs.begin();
       it != peakWs.end(); ++it) {
    const int pos = static_cast<int>(std::distance(peakWs.begin(), it));
    QColor color = m_peakPalette.foregroundIndexToColour(pos);
    const std::string name = (*it)->getName();
    colors.insert(std::pair<std::string, QColor>(name, color));
  }

  return colors;
}

/**
 * Update the color of the peak workspace representation.
 */
void PeaksTableControllerVsi::updatePeakWorkspaceColor() {
  std::vector<Mantid::API::IPeaksWorkspace_sptr> peakWs =
      m_presenter->getPeaksWorkspaces();
  for (std::vector<Mantid::API::IPeaksWorkspace_sptr>::iterator it =
           peakWs.begin();
       it != peakWs.end(); ++it) {
    const int pos = static_cast<int>(std::distance(peakWs.begin(), it));
    QColor color = m_peakPalette.foregroundIndexToColour(pos);
    const std::string name = (*it)->getName();

    // Find the source whcih is associated with the peak workspace and change
    // its representation
    pqServer *server = pqActiveObjects::instance().activeServer();
    pqServerManagerModel *smModel =
        pqApplicationCore::instance()->getServerManagerModel();
    QList<pqPipelineSource *> sources =
        smModel->findItems<pqPipelineSource *>(server);
    for (QList<pqPipelineSource *>::iterator src = sources.begin();
         src != sources.end(); ++src) {
      // Make sure that the source is a peak workspace

      std::string xmlName((*src)->getProxy()->GetXMLName());
      if ((xmlName.find("Peaks Source") != std::string::npos)) {
        std::string workspaceName(
            vtkSMPropertyHelper((*src)->getProxy(), "WorkspaceName")
                .GetAsString());
        if (workspaceName == name) {
          int r = color.red();
          int g = color.green();
          int b = color.blue();

          double red = static_cast<double>(r) / 255.0;
          double green = static_cast<double>(g) / 255.0;
          double blue = static_cast<double>(b) / 255.0;

          pqDataRepresentation *rep =
              (*src)
                  ->getRepresentation(pqActiveObjects::instance().activeView());
           pqPipelineRepresentation *pipelineRepresentation =
               qobject_cast<pqPipelineRepresentation *>(rep);
          pipelineRepresentation->getProxy()->UpdatePropertyInformation();

          vtkSMDoubleVectorProperty *prop =
              vtkSMDoubleVectorProperty::SafeDownCast(
                  pipelineRepresentation->getProxy()->GetProperty(
                      "DiffuseColor"));
          prop->SetElement(0, red);
          prop->SetElement(1, green);
          prop->SetElement(2, blue);
          pipelineRepresentation->getProxy()->UpdateVTKObjects();
          pipelineRepresentation->updateHelperProxies();
          pqActiveObjects::instance().activeView()->forceRender();
          break;
        }
      }
    }
  }
}

/**
 * Update the view region for the presenters
 */
void PeaksTableControllerVsi::updateViewableArea() {
  Mantid::VATES::ViewFrustum frustum = m_cameraManager->getCurrentViewFrustum();
  m_presenter->updateViewFrustum(frustum);
}

/**
 * Extract the frame from the source
 * @param splatSource A pointer to a splatter plot source.
 */
std::vector<std::string> PeaksTableControllerVsi::extractFrameFromSource(
    QPointer<pqPipelineSource> splatSource) {
  pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(splatSource);

  if (!filter) {
    throw std::invalid_argument("The splatter source is not a filter.");
  }

  // Check the original source
  pqPipelineSource *originalSource = filter->getInput(0);
  if (!originalSource) {
    throw std::invalid_argument("The original source cannot be found.");
  }

  std::string xmlName(originalSource->getProxy()->GetXMLName());
  if (!(xmlName.find("MDEW") != std::string::npos)) {
    throw std::invalid_argument("The original source cannot be found.");
  }

  std::string wsName(vtkSMPropertyHelper(originalSource->getProxy(),
                                         "WorkspaceName", true).GetAsString());
  Mantid::API::IMDEventWorkspace_sptr eventWorkspace =
      Mantid::API::AnalysisDataService::Instance()
          .retrieveWS<Mantid::API::IMDEventWorkspace>(wsName);

  std::vector<std::string> dimensionInfo;
  for (size_t i = 0; i < eventWorkspace->getNumDims(); i++) {
    dimensionInfo.push_back(
        MantidQt::API::PlotAxis(*(eventWorkspace->getDimension(i)))
            .title()
            .toStdString());
  }

  // Update the current coordiate system
  m_coordinateSystem = eventWorkspace->getSpecialCoordinateSystem();

  return dimensionInfo;
}

/**
 * Check if the peaks viewer has a peaks workspace loaded.
 * @returns If the a peaks workspace is loaded.
 */
bool PeaksTableControllerVsi::hasPeaks() {
  if (!m_presenter || !m_presenter->hasPeaks()) {
    return false;
  } else {
    return true;
  }
}

/**
 * Show all peaks in the table.
 */
void PeaksTableControllerVsi::showFullTable() { createTable(true); }

/**
 * Create the table
 * @param full If the full table is to be displayed or only visible peaks.
 */
void PeaksTableControllerVsi::createTable(bool full) {
  // Create the table if it does not exist
  if (hasPeaks()) {
    if (layout()) {
      removeLayout(this);
    }

    // Create new widget
    try {
      // Set the layout of the table
      this->setLayout(new QVBoxLayout);

      PeaksTabWidget *widget = new PeaksTabWidget(
          m_presenter->getPeaksWorkspaces(), m_presenter->getFrame(), this);
      QObject::connect(
          widget, SIGNAL(zoomToPeak(Mantid::API::IPeaksWorkspace_sptr, int)),
          this, SLOT(onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr, int)));
      QObject::connect(
          widget, SIGNAL(sortPeaks(const std::string &, const bool,
                                   Mantid::API::IPeaksWorkspace_sptr)),
          this, SLOT(onPeaksSorted(const std::string &, const bool,
                                   Mantid::API::IPeaksWorkspace_sptr)));
      // Initialize the viewablePeaks to be true
      std::map<std::string, std::vector<bool>> viewablePeaks =
          m_presenter->getInitializedViewablePeaks();

      if (!full) {
        //
        // viewablePeaks = getViewablePeaks();
      }

      widget->setupMvc(viewablePeaks);
      layout()->addWidget(widget);
      m_peaksTabWidget = widget;
      // Set the color
      m_peaksTabWidget->updateTabs(viewablePeaks, getColors());
      updatePeakWorkspaceColor();
    } catch (std::runtime_error &ex) {
      g_log.warning()
          << "Could not setup the the peaks widget for the splatterplot: "
          << ex.what() << "\n";
    } catch (...) {
      g_log.warning()
          << "Could not setup the the peaks widget for the splatterplot.\n";
    }
  }
}

/**
* Remove the layout
* @param widget
*/
void PeaksTableControllerVsi::removeLayout(QWidget *widget) {
  QLayout *layout = widget->layout();
  if (layout != 0) {
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != 0) {
      layout->removeItem(item);
      delete item->widget();
    }
    delete layout;
  }
}

/**
 * Remove the table.
 */
void PeaksTableControllerVsi::removeTable() {
  destroySinglePeakSource();
  if (m_peaksTabWidget) {
    m_peaksTabWidget->deleteLater();
  }
  m_peaksTabWidget = NULL;
}

/**
 * Zoom to a specific peak
 * @param peaksWorkspace The peaksworkspace which is currently being displayed.
 * @param row The selected row.
 */
void PeaksTableControllerVsi::onZoomToPeak(
    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row) {
  try {
    double radius;
    Mantid::Kernel::V3D position;

    m_presenter->getPeaksInfo(peaksWorkspace, row, position, radius, m_coordinateSystem);

    // Reset camera
    m_cameraManager->setCameraToPeak(position[0], position[1], position[2],
                                     radius);

    // Place a marker glyph at the position
    if (!m_peakMarker) {
      generateSinglePeaksSource(position[0], position[1], position[2], radius);
    } else {
      resetSinglePeaksSource(position[0], position[1], position[2], radius);
    }

    emit setRotationToPoint(position[0], position[1], position[2]);
  } catch (std::invalid_argument &ex) {
    g_log.warning() << ex.what();
    emit setRotationToPoint(0.0, 0.0, 0.0);
  }
}

/**
 * Generate a single peak  glyph
 * @param position1 Position 1 of the glyph.
 * @param position2 Position 2 of the glyph.
 * @param position3 Position 3 of the glyph.
 * @param radius The radius of the peak.
 */
void PeaksTableControllerVsi::generateSinglePeaksSource(double position1,
                                                        double position2,
                                                        double position3,
                                                        double radius) {
  // Create the source from the plugin
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqPipelineSource *src =
      builder->createSource("sources", "SinglePeakMarkerSource", server);
  vtkSMPropertyHelper(src->getProxy(), "Position1").Set(position1);
  vtkSMPropertyHelper(src->getProxy(), "Position2").Set(position2);
  vtkSMPropertyHelper(src->getProxy(), "Position3").Set(position3);
  vtkSMPropertyHelper(src->getProxy(), "RadiusMarker").Set(radius);

  vtkSMSourceProxy *srcProxy = vtkSMSourceProxy::SafeDownCast(src->getProxy());
  srcProxy->UpdateVTKObjects();
  srcProxy->Modified();
  srcProxy->UpdatePipelineInformation();
  src->updatePipeline();

  pqDataRepresentation *drep = builder->createDataRepresentation(
      src->getOutputPort(0), pqActiveObjects::instance().activeView());
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set("Surface");
  srcProxy->UpdateVTKObjects();
  srcProxy->Modified();
  srcProxy->UpdatePipelineInformation();
  src->updatePipeline();

  pqActiveObjects::instance().activeView()->forceRender();

  m_peakMarker = src;

  // We need to make sure we detect when the source is destroyed, as the user
  // can delete it in the pipeline browser
  QObject::connect(m_peakMarker, SIGNAL(destroyed()), this,
                   SLOT(onPeakMarkerDestroyed()));
}

/**
 * Destroy a single peaks source.
 */
void PeaksTableControllerVsi::destroySinglePeakSource() {
  if (m_peakMarker) {
    pqObjectBuilder *builder =
        pqApplicationCore::instance()->getObjectBuilder();
    builder->destroy(m_peakMarker);

    m_peakMarker = NULL;
  }
}

/**
 * On Single Peak Marker destroyed
 */
void PeaksTableControllerVsi::onPeakMarkerDestroyed() { m_peakMarker = NULL; }

/**
 * Reset the single peak source
 * @param position1 Position 1 of the glyph.
 * @param position2 Position 2 of the glyph.
 * @param position3 Position 3 of the glyph.
 * @param radius The radius of the peak.
 */
void PeaksTableControllerVsi::resetSinglePeaksSource(double position1,
                                                     double position2,
                                                     double position3,
                                                     double radius) {
  vtkSMPropertyHelper(m_peakMarker->getProxy(), "Position1").Set(position1);
  vtkSMPropertyHelper(m_peakMarker->getProxy(), "Position2").Set(position2);
  vtkSMPropertyHelper(m_peakMarker->getProxy(), "Position3").Set(position3);
  vtkSMPropertyHelper(m_peakMarker->getProxy(), "RadiusMarker").Set(radius);

  vtkSMSourceProxy *srcProxy =
      vtkSMSourceProxy::SafeDownCast(m_peakMarker->getProxy());
  srcProxy->UpdateVTKObjects();
  srcProxy->Modified();
  srcProxy->UpdatePipelineInformation();
  m_peakMarker->updatePipeline();

  pqActiveObjects::instance().activeView()->forceRender();
}

/**
 * Get the workspace names as a concatenated string
 * @param delimiter The delimiter to concatenate workspace names.
 * @returns The concatenated workspace names.
 */
std::string
PeaksTableControllerVsi::getConcatenatedWorkspaceNames(std::string delimiter) {
  std::vector<std::string> peaksWorkspaceNames =
      m_presenter->getPeaksWorkspaceNames();
  std::stringstream stream;
  for (size_t i = 0; i < peaksWorkspaceNames.size(); i++) {
    stream << peaksWorkspaceNames[i];
    // Don't add a delimiter after the last element
    if (i != (peaksWorkspaceNames.size() - 1)) {
      stream << delimiter;
    }
  }
  return stream.str();
}

/**
 * Update the  presenters with the available peak workspaces
 * @param peakSources A list with available peak sources
 * @param splatSource The splatterplot source
 */
void PeaksTableControllerVsi::updatePeaksWorkspaces(
    QList<QPointer<pqPipelineSource>> peakSources,
    pqPipelineSource *splatSource) {
  // Check if the presenters exist and which need to be added
  std::vector<std::string> peaksWorkspaceNames;

  std::vector<pqPipelineSource *> nonTrackedWorkspaces;
  std::vector<std::string> trackedWorkspaceNames =
      m_presenter->getPeaksWorkspaceNames();
  for (QList<QPointer<pqPipelineSource>>::Iterator it = peakSources.begin();
       it != peakSources.end(); ++it) {
    std::string workspaceName(
        vtkSMPropertyHelper((*it)->getProxy(), "WorkspaceName").GetAsString());

    peaksWorkspaceNames.push_back(workspaceName);

    int count = static_cast<int>(std::count(trackedWorkspaceNames.begin(),
                                            trackedWorkspaceNames.end(),
                                            workspaceName));

    if (count == 0) {
      nonTrackedWorkspaces.push_back(*it);
    }
  }

  if (splatSource) {
    // Add the workspaces which are missing in the presenter
    for (std::vector<pqPipelineSource *>::iterator it =
             nonTrackedWorkspaces.begin();
         it != nonTrackedWorkspaces.end(); ++it) {
      addWorkspace(*it, splatSource);
    }
  }

  // Now update all the presenter
  m_presenter->updateWorkspaces(peaksWorkspaceNames);
  if (!peakSources.empty() && m_peaksTabWidget) {
    m_peaksTabWidget->updateTabs(m_presenter->getInitializedViewablePeaks(),
                                 getColors());
    updatePeakWorkspaceColor();
  }

  // If there are no presenters left, we want to destroy the table
  if (!hasPeaks()) {
    removeTable();
  }
}

/**
 * Sort the peaks according to a selected column.
 * @param columnToSortBy The column by which to sort.
 * @param sortAscending If sort ascending or descending.
 * @param ws The workspace which requires sorting.
 */
void PeaksTableControllerVsi::onPeaksSorted(
    const std::string &columnToSortBy, const bool sortAscending,
    Mantid::API::IPeaksWorkspace_sptr ws) {
  // Invoke the ording command on the presenters
  m_presenter->sortPeaksWorkspace(columnToSortBy, sortAscending, ws);
  // Update the tabs
}
}
}
}