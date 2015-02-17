#include "MantidVatesAPI/ConcretePeaksPresenterVsi.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"

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
}
}