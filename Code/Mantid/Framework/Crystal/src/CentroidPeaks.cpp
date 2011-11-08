#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidCrystal/CentroidPeaks.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/VectorHelper.h"

using Mantid::DataObjects::PeaksWorkspace;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CentroidPeaks)

  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;
  using namespace Mantid::Kernel;
  using namespace Mantid::Crystal;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CentroidPeaks::CentroidPeaks()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CentroidPeaks::~CentroidPeaks()
  {
  }


  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CentroidPeaks::initDocs()
  {
    this->setWikiSummary("Find the centroid of single-crystal peaks in a 2D Workspace, in order to refine their positions.");
    this->setOptionalMessage("Find the centroid of single-crystal peaks in a 2D Workspace, in order to refine their positions.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CentroidPeaks::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input 2D Workspace.");

    declareProperty(new PropertyWithValue<int>("PeakRadius",10,Direction::Input),
        "Fixed radius around each peak position in which to calculate the centroid.");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace","",Direction::Input),
        "A PeaksWorkspace containing the peaks to centroid.");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutPeaksWorkspace","",Direction::Output),
        "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
        "with the peaks' positions modified by the new found centroids.");
  }

  //----------------------------------------------------------------------------------------------
  /** Integrate the peaks of the workspace using parameters saved in the algorithm class
   * @param ws ::  2D Workspace to integrate
   */
  void CentroidPeaks::integrate(API::MatrixWorkspace_sptr ws)
  {

    /// Peak workspace to centroid
    Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS = getProperty("InPeaksWorkspace");

    /// Output peaks workspace, create if needed
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS = getProperty("OutPeaksWorkspace");
    if (peakWS != inPeakWS)
      peakWS = inPeakWS->clone();


    /// Radius to use around peaks
    int PeakRadius = getProperty("PeakRadius");

    PRAGMA_OMP(parallel for schedule(dynamic, 10) )
    for (int i=0; i < int(peakWS->getNumberPeaks()); ++i)
    {
      // Get a direct ref to that peak.
      IPeak & peak = peakWS->getPeak(i);
      int col = peak.getCol();
      int row = peak.getRow();
      double TOFPeakd = peak.getTOF();
      Geometry::Instrument_const_sptr Iptr = peak.getInstrument();
      std::string bankName = peak.getBankName();

      boost::shared_ptr<const IComponent> parent = Iptr->getComponentByName(bankName);

      if (parent->type().compare("RectangularDetector") != 0)
      {

        std::cout<<"   getPanel C type="<<parent->type()<<std::endl;
                throw std::runtime_error("Improper Peak Argument");
      }
      boost::shared_ptr<const RectangularDetector> RDet = boost::shared_dynamic_cast<
                const RectangularDetector>(parent);
      double intensity = 0.0;
      double chancentroid = 0.0;
      boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
      Mantid::detid2index_map::iterator it;
      it = (*wi_to_detid_map).find(pixel->getID());
      size_t workspaceIndex = (it->second);

      Mantid::MantidVec X = ws->readX(workspaceIndex);
      Mantid::MantidVec histogram = ws->readY(workspaceIndex);

      int chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
      int chanstart = chan-PeakRadius;
      int chanend = chan+PeakRadius;
      for (int ichan=chanstart; ichan<=chanend; ++ichan)
      {
        intensity += histogram[ichan];
        chancentroid += ichan*histogram[ichan];
      }
      chan = int(chancentroid/intensity);
      intensity = 0.0;
      double rowcentroid = 0.0;
      int rowstart = std::max(0,row-PeakRadius);
      int rowend = std::min(RDet->ypixels()-1,row+PeakRadius);
      for (int irow=rowstart; irow<=rowend; ++irow)
      {
        boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, irow);
        Mantid::detid2index_map::iterator it;
        it = (*wi_to_detid_map).find(pixel->getID());
        size_t workspaceIndex = (it->second);

        Mantid::MantidVec X = ws->readX(workspaceIndex);
        Mantid::MantidVec histogram = ws->readY(workspaceIndex);

        int chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
        intensity += histogram[chan];
        rowcentroid += irow*histogram[chan];
      }
      row = std::min(RDet->ypixels()-1,int(rowcentroid/intensity));
      row = std::max(0,row);
      intensity = 0.0;
      double colcentroid = 0.0;
      int colstart = std::max(0,col-PeakRadius);
      int colend = std::min(RDet->xpixels()-1,col+PeakRadius);
      for (int icol=colstart; icol<=colend; ++icol)
      {
        boost::shared_ptr<Detector> pixel = RDet->getAtXY(icol, row);
        Mantid::detid2index_map::iterator it;
        it = (*wi_to_detid_map).find(pixel->getID());
        size_t workspaceIndex = (it->second);

        Mantid::MantidVec X = ws->readX(workspaceIndex);
        Mantid::MantidVec histogram = ws->readY(workspaceIndex);

        int chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
        intensity += histogram[chan];
        colcentroid += icol*histogram[chan];
      }
      col = std::min(RDet->xpixels()-1,int(colcentroid/intensity));
      col = std::max(0,col);
      intensity = 0.0;
      rowcentroid = 0.0;
      rowstart = std::max(0,row-PeakRadius);
      rowend = std::min(RDet->ypixels()-1,row+PeakRadius);
      for (int irow=rowstart; irow<=rowend; ++irow)
      {
        boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, irow);
        Mantid::detid2index_map::iterator it;
        it = (*wi_to_detid_map).find(pixel->getID());
        size_t workspaceIndex = (it->second);

        Mantid::MantidVec X = ws->readX(workspaceIndex);
        Mantid::MantidVec histogram = ws->readY(workspaceIndex);

        int chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
        intensity += histogram[chan];
        rowcentroid += irow*histogram[chan];
      }
      row = std::min(RDet->ypixels()-1,int(rowcentroid/intensity));
      row = std::max(0,row);
      pixel = RDet->getAtXY(col, row);
      peak.setDetectorID(pixel->getID());
    }

    // Save the output
    setProperty("OutPeaksWorkspace", peakWS);

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CentroidPeaks::exec()
  {
    inWS = getProperty("InputWorkspace");

    // For quickly looking up workspace index from det id
    wi_to_detid_map = inWS->getDetectorIDToWorkspaceIndexMap(true);
    integrate(inWS);
  }

} // namespace Mantid
} // namespace Crystal
