#include "MantidVatesSimpleGuiViewWidgets/PeakViewerVsi.h"
#include "MantidVatesSimpleGuiViewWidgets/PeaksWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/CameraManager.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/PeakTransformHKL.h"
#include "MantidAPI/PeakTransformQSample.h"
#include "MantidAPI/PeakTransformQLab.h"
#include "MantidAPI/IPeak.h"
#include "MantidKernel/V3D.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidVatesAPI/PeaksPresenterVsi.h"
#include "MantidVatesAPI/NullPeaksPresenterVsi.h"
#include "MantidVatesAPI/ConcretePeaksPresenterVsi.h"
#include "MantidQtAPI/PlotAxis.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/Crystal/PeakShape.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <algorithm>

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <vtkSMPropertyHelper.h>
#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>

#include <QString>
#include <QPointer>
#include <QVBoxLayout>
#include <QLayout>
#include <QLayoutItem>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
  /**
   * Constructor
   * @param cameraManager A cameraManager pointer.
   * @param parent A pointer to a QWidget parent.
   */
  PeaksViewerVsi::PeaksViewerVsi(boost::shared_ptr<CameraManager> cameraManager, QWidget *parent) : QWidget(parent), 
                                                                                                    m_cameraManager(cameraManager),
                                                                                                    m_presenter(new Mantid::VATES::NullPeaksPresenterVsi()),
                                                                                                    m_peaksWidget(NULL)
  {
    m_peakTransformSelector.registerCandidate(boost::make_shared<Mantid::API::PeakTransformHKLFactory>());
    m_peakTransformSelector.registerCandidate(boost::make_shared<Mantid::API::PeakTransformQSampleFactory>());
    m_peakTransformSelector.registerCandidate(boost::make_shared<Mantid::API::PeakTransformQLabFactory>());
  }

  /**
   * Check for viewable peaks.
   * @returns A vector of the peak indices which are visible and which are not visible.
   */ 
  std::vector<bool> PeaksViewerVsi::getViewablePeaks()
  {
    std::vector<bool> viewablePeaks;
    if (m_presenter)
    {
      // Get the up to date area
      updateViewableArea();

      //Get a list with viewable peak coordinates
      viewablePeaks = m_presenter->getViewablePeaks();
    }
    return viewablePeaks;
  }

  /**
   * Add a new workspace
   * @param source A new peaks source
   * @param splatSource A pointer to the splatter source
   */
  void PeaksViewerVsi::addWorkspace(pqPipelineSource* source, QPointer<pqPipelineSource> splatSource)
  {
    if (!source || !splatSource)
    {
      throw std::invalid_argument("The pqPipelineSource of the peaks workspace does not exist.");
    }

    // Get the pointer to the peaks workspace 
    std::string wsName(vtkSMPropertyHelper(source->getProxy(), "WorkspaceName", true).GetAsString());
    std::string peaksFrame(vtkSMPropertyHelper(source->getProxy(), "Peak Dimensions", true).GetAsString());

    // Get dimensions from splattersource
    std::vector<std::string> dimInfo = extractFrameFromSource(splatSource);
    if (dimInfo.size() < 2)
    {
      throw std::invalid_argument("The workspace needs to have at least two dimensions");
    }

    std::string dimCompare = dimInfo[0];
    std::transform(dimCompare.begin(), dimCompare.end(),dimCompare.begin(), ::toupper);
    std::transform(peaksFrame.begin(), peaksFrame.end(),peaksFrame.begin(), ::toupper);
    // Check if frames match
    if (dimCompare.find(peaksFrame) == std::string::npos)
    {
      throw std::runtime_error("The workspaces don't match");
    }

    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace;
    try
    {
      peaksWorkspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IPeaksWorkspace>(wsName);
    }
    catch(Mantid::Kernel::Exception::NotFoundError&)
    {
      throw std::invalid_argument("The peaks workspace cannot be retrieved.");
    }
    
    std::string frame;
    try 
    {
      Mantid::API::PeakTransformFactory_sptr transformFactory = m_peakTransformSelector.makeChoice(dimInfo[0], dimInfo[1]);
      Mantid::API::PeakTransform_sptr transform = transformFactory->createTransform(dimInfo[0], dimInfo[1]);
      frame = transform->getFriendlyName();
    } catch (std::invalid_argument &ex) {
      throw std::invalid_argument("Couldn't create the transform factory");
    }

    m_presenter = boost::make_shared<Mantid::VATES::ConcretePeaksPresenterVsi>(peaksWorkspace, m_cameraManager->getCurrentViewFrustum(), frame);
  }

  /**
   * Update the view region for the presenters
   */
  void PeaksViewerVsi::updateViewableArea()
  {
    Mantid::VATES::ViewFrustum frustum = m_cameraManager->getCurrentViewFrustum();
    m_presenter->updateViewFrustum(frustum);
  }

  /**
   * Extrac the frame from the source
   * @param splatSource A pointer to a splatter plot source.
   */
  std::vector<std::string> PeaksViewerVsi::extractFrameFromSource(QPointer<pqPipelineSource> splatSource)
  {
    pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(splatSource);
    
    if (!filter)
    {
      throw std::invalid_argument("The splatter source is not a filter");
    }

    pqPipelineSource* originalSource = filter->getInput(0);

    if (!originalSource)
    {
      throw std::invalid_argument("The original source cannot be found");
    }

    std::string wsName(vtkSMPropertyHelper(originalSource->getProxy(), "WorkspaceName", true).GetAsString());
    Mantid::API::IMDEventWorkspace_sptr eventWorkspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IMDEventWorkspace>(wsName);

    std::vector<std::string> dimensionInfo;
    for (size_t i = 0; i < eventWorkspace->getNumDims(); i++)
    {
      dimensionInfo.push_back(MantidQt::API::PlotAxis(*(eventWorkspace->getDimension(i))).title().toStdString());
    }
    
    return dimensionInfo;
  }

  /**
   * Check if the peaks viewer has a peaks workspace loaded.
   * @returns If the a peaks workspace is loaded.
   */
  bool PeaksViewerVsi::hasPeaks()
  {
    if (!m_presenter)
    {
      return false;
    }
    else if (boost::dynamic_pointer_cast<Mantid::VATES::NullPeaksPresenterVsi>(m_presenter))
    {
      return false;
    }
    else 
    {
      return true;
    }
  }

  /**
   * Show the table with the visible peaks
   */
  void PeaksViewerVsi::showTable()
  {
    createTable(false);
  }

  /**
   * Show all peaks in the table.
   */
  void PeaksViewerVsi::showFullTable()
  {
    createTable(true);
  }

  /**
   * Create the table
   * @param full If the full table is to be displayed or only visible peaks.
   */
  void PeaksViewerVsi::createTable(bool full)
  {
    // Create the table if it does not exist
    if (hasPeaks())
    {
      if (layout()) 
      {
        removeLayout(this);
      }

      this->setLayout(new QVBoxLayout);

      // Create new widget
      PeaksWidget* widget = new PeaksWidget(m_presenter->getPeaksWorkspace(), m_presenter->getFrame(), this);
      QObject::connect(widget, SIGNAL(zoomToPeak(Mantid::API::IPeaksWorkspace_sptr, int)),
                       this, SLOT(onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr, int)));
      
      // Initialize the viewablePeaks to be true
      std::vector<bool> viewablePeaks(m_presenter->getPeaksWorkspace()->getNumberPeaks(),true);

      if (!full)
      {
        try
        {
          viewablePeaks = getViewablePeaks();
        } catch (...)
        {
          // Log 
        }
      }

      widget->setupMvc(viewablePeaks);
      layout()->addWidget(widget);
    }
  }

  /**
  * Remove the layout
  * @param widget
  */
  void PeaksViewerVsi::removeLayout(QWidget *widget) {
    QLayout *layout = widget->layout();
    if (layout != 0) {
      QLayoutItem *item;
      while ((item = layout->takeAt(0)) != 0){
        layout->removeItem(item);
        delete item->widget();
      }
      delete layout;
    }
  }

  /**
   * Remvove the table.
   */
  void PeaksViewerVsi::removeTable()
  {
    if (layout()) {
      removeLayout(this);
    }
  }

 /**
  * Zoom to a specific peak
  * @param peaksWorkspace The peaksworkspace which is currently being displayed.
  * @param row The selected row.
  */
  void PeaksViewerVsi::onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row)
  {
    // Extract the position
    Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = peaksWorkspace->getSpecialCoordinateSystem();

    Mantid::Kernel::V3D position;
    switch(coordinateSystem)
    {
      case(Mantid::Kernel::SpecialCoordinateSystem::QLab):
        position = peaksWorkspace->getPeak(row).getQLabFrame();
        break;
      case(Mantid::Kernel::SpecialCoordinateSystem::QSample):
        position = peaksWorkspace->getPeak(row).getQSampleFrame();
        break;
      case(Mantid::Kernel::SpecialCoordinateSystem::HKL):
        position = peaksWorkspace->getPeak(row).getHKL();
        break;
      default:
        return;
    }

    // Peak radius
    Mantid::Geometry::PeakShape_sptr shape(peaksWorkspace->getPeakPtr(row)->getPeakShape().clone());
    double maxRadius = getMaxRadius(shape);
    
    // Peak position
    double xpos = position[0];
    double ypos = position[1];
    double zpos = position[2];
    
    // Direction
    

    // Reset camera
    m_cameraManager->setCameraToPeak(xpos, ypos, zpos, maxRadius);
  }

  /**
   * Get the maximal radius
   * @param shape The shape of a peak.
   * @param The maximal radius of the peak.
   */
  double PeaksViewerVsi::getMaxRadius(Mantid::Geometry::PeakShape_sptr shape)
  {
    boost::shared_ptr<Mantid::DataObjects::NoShape> nullShape = boost::dynamic_pointer_cast<Mantid::DataObjects::NoShape>(shape);
    boost::shared_ptr<Mantid::DataObjects::PeakShapeEllipsoid> ellipsoidShape = boost::dynamic_pointer_cast<Mantid::DataObjects::PeakShapeEllipsoid>(shape);
    boost::shared_ptr<Mantid::DataObjects::PeakShapeSpherical> sphericalShape = boost::dynamic_pointer_cast<Mantid::DataObjects::PeakShapeSpherical>(shape);
    
    if (nullShape)
    {
      return 0;
    }
    else if (ellipsoidShape)
    {
      std::vector<double> radius = ellipsoidShape->abcRadiiBackgroundOuter();
      return *(std::max_element(radius.begin(),radius.end()));
    }
    else if (sphericalShape)
    {
      if (boost::optional<double> radius = sphericalShape->backgroundOuterRadius())
      {
        return *radius;
      }
      else
      {
        return 0;
      }
    }
    else
    {
      return 0;
    }
  }
}
}
}