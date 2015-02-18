#include "MantidVatesAPI/ConcretePeaksPresenterVsi.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidDataObjects/NoShape.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidAPI/IPeak.h"
#include "MantidGeometry/Crystal/PeakShape.h"
namespace Mantid
{
namespace VATES
{
  /**
   * Constructor
   * @param peaksWorkspace The peaks workspace.
   */
  ConcretePeaksPresenterVsi::ConcretePeaksPresenterVsi(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace,
                                                       ViewFrustum frustum,
                                                       std::string frame) : m_viewableRegion(frustum),
                                                                            m_peaksWorkspace(peaksWorkspace),
                                                                            m_frame(frame)                                                                                                     
  {
  }

  ///Destructor
  ConcretePeaksPresenterVsi::~ConcretePeaksPresenterVsi()
  {
  }

  /**
   * Update the view frustum
   */
  void ConcretePeaksPresenterVsi::updateViewFrustum(ViewFrustum frustum)
  {
    m_viewableRegion = frustum;
  }

  /**
   * Get the viewable peaks. Essentially copied from the slice viewer.
   * @retruns A vector indicating which of the peaks are viewable.
   */
  std::vector<bool> ConcretePeaksPresenterVsi::getViewablePeaks()
  {
    //Need to apply a transform.
    // Don't bother to find peaks in the region if there are no peaks to find.
    Mantid::API::ITableWorkspace_sptr outTable;

    if (this->m_peaksWorkspace->getNumberPeaks() >= 1) 
    {
      double effectiveRadius = 1e-2;
      std::string viewable = m_viewableRegion.toExtentsAsString();
      Mantid::API::IPeaksWorkspace_sptr peaksWS =  m_peaksWorkspace;
      
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("PeaksInRegion");
      alg->setChild(true);
      alg->setRethrows(true);
      alg->initialize();
      alg->setProperty("InputWorkspace", peaksWS);
      alg->setProperty("OutputWorkspace", peaksWS->name() + "_peaks_in_region");
      alg->setProperty("Extents", viewable);
      alg->setProperty("CheckPeakExtents", true);
      alg->setProperty("PeakRadius", effectiveRadius);
      alg->setPropertyValue("CoordinateFrame", m_frame);
      alg->execute();
      outTable = alg->getProperty("OutputWorkspace");
      std::vector<bool> viewablePeaks(outTable->rowCount());
      for (size_t i = 0; i < outTable->rowCount(); ++i) {
        viewablePeaks[i] = outTable->cell<Mantid::API::Boolean>(i, 1);
      }
      m_viewablePeaks = viewablePeaks;
    }
    else{
      // No peaks will be viewable
      m_viewablePeaks = std::vector<bool>();
    }
    
    return m_viewablePeaks;
  }

  /**
   * Get the underlying peaks workspace
   * @returns A pointer to the underlying peaks workspace.
   */
  Mantid::API::IPeaksWorkspace_sptr ConcretePeaksPresenterVsi::getPeaksWorkspace()
  {
    return m_peaksWorkspace;
  }

  /**
   * Get the frame
   * @returns The frame.
   */
  std::string ConcretePeaksPresenterVsi::getFrame()
  {
    return m_frame;
  }

  /**
   * Get the name of the underlying peaks workspace.
   * @returns The name of the peaks workspace.
   */
  std::string ConcretePeaksPresenterVsi::getPeaksWorkspaceName()
  {
    return m_peaksWorkspace->getName();
  }

  /**
   * Get the peaks info for a single peak, defined by the row in the peaks table.
   * @param peaksWorkspace A pointer to a peaks workspace.
   * @param row The row in the peaks table.
   * @param position A reference which holds the position of the peak.
   * @param radius A reference which holds the radius of the peak.
   */
  void ConcretePeaksPresenterVsi::getPeaksInfo(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, int row, Mantid::Kernel::V3D& position, double& radius)
  {
    // Extract the position
    Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = peaksWorkspace->getSpecialCoordinateSystem();

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
        throw std::invalid_argument("The coordinate system is invalid.\n");
    }

    // Peak radius
    Mantid::Geometry::PeakShape_sptr shape(peaksWorkspace->getPeakPtr(row)->getPeakShape().clone());
    radius = getMaxRadius(shape);
  }

  /**
   * Get the maximal radius
   * @param shape The shape of a peak.
   * @param The maximal radius of the peak.
   */
  double ConcretePeaksPresenterVsi::getMaxRadius(Mantid::Geometry::PeakShape_sptr shape)
  {
    const double defaultRadius = 1.0;
    boost::shared_ptr<Mantid::DataObjects::NoShape> nullShape = boost::dynamic_pointer_cast<Mantid::DataObjects::NoShape>(shape);
    boost::shared_ptr<Mantid::DataObjects::PeakShapeEllipsoid> ellipsoidShape = boost::dynamic_pointer_cast<Mantid::DataObjects::PeakShapeEllipsoid>(shape);
    boost::shared_ptr<Mantid::DataObjects::PeakShapeSpherical> sphericalShape = boost::dynamic_pointer_cast<Mantid::DataObjects::PeakShapeSpherical>(shape);

    if (nullShape)
    {
      return defaultRadius;
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
        return defaultRadius;
      }
    }
    else
    {
      return defaultRadius;
    }
  }

}
}