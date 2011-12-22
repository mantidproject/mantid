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
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace","",Direction::Input),
        "A PeaksWorkspace containing the peaks to centroid.");

    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input 2D Workspace.");

    declareProperty(new PropertyWithValue<int>("PeakRadius",10,Direction::Input),
        "Fixed radius around each peak position in which to calculate the centroid.");

    declareProperty(new PropertyWithValue<int>("EdgePixels",0,Direction::Input),
      "The number of pixels where peaks are removed at edges. Only for instruments with RectangularDetectors. " );

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutPeaksWorkspace","",Direction::Output),
        "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
        "with the peaks' positions modified by the new found centroids.");
  }

  //----------------------------------------------------------------------------------------------
  /** Integrate the peaks of the workspace using parameters saved in the algorithm class
   * @param inWS ::  2D Workspace to integrate
   */
  void CentroidPeaks::integrate()
  {

    /// Peak workspace to centroid
    Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS = getProperty("InPeaksWorkspace");

    /// Output peaks workspace, create if needed
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS = getProperty("OutPeaksWorkspace");
    if (peakWS != inPeakWS)
      peakWS = inPeakWS->clone();

    //To get the workspace index from the detector ID
    detid2index_map * pixel_to_wi = inWS->getDetectorIDToWorkspaceIndexMap(true);

    /// Radius to use around peaks
    int PeakRadius = getProperty("PeakRadius");

    int MinPeaks = -1;
    int MaxPeaks = -1;
    size_t Numberwi = inWS->getNumberHistograms();
    int NumberPeaks = peakWS->getNumberPeaks();
    for (int i = 0; i<NumberPeaks; i++)
    {
      Peak & peak = peakWS->getPeaks()[i];
      int pixelID = peak.getDetectorID();

      // Find the workspace index for this detector ID
      if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
      {
         size_t wi = (*pixel_to_wi)[pixelID];
         if(MinPeaks == -1 && peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi) MinPeaks = i;
         if(peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi) MaxPeaks = i;
      }
    }
    Progress prog(this, MinPeaks, 1.0, MaxPeaks);
    PARALLEL_FOR2(inWS,peakWS)
    for (int i = MinPeaks; i<= MaxPeaks; i++)
    {
      PARALLEL_START_INTERUPT_REGION
      // Get a direct ref to that peak.
      IPeak & peak = peakWS->getPeak(i);
      int col = peak.getCol();
      int row = peak.getRow();
      double TOFPeakd = peak.getTOF();
      Geometry::Instrument_const_sptr Iptr = peak.getInstrument();
      std::string bankName = peak.getBankName();

      boost::shared_ptr<const IComponent> parent = Iptr->getComponentByName(bankName);
      if (!parent) continue;
      if (parent->type().compare("RectangularDetector") != 0) continue;
      boost::shared_ptr<const RectangularDetector> RDet = boost::shared_dynamic_cast<
                const RectangularDetector>(parent);
      double intensity = 0.0;
      double chancentroid = 0.0;
      boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
      Mantid::detid2index_map::iterator it;
      it = (*wi_to_detid_map).find(pixel->getID());
      if (it == (*wi_to_detid_map).end())continue;
      size_t workspaceIndex = (it->second);

      Mantid::MantidVec X = inWS->readX(workspaceIndex);
      Mantid::MantidVec histogram = inWS->readY(workspaceIndex);

      int chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
      int chanstart = std::max(0,chan-PeakRadius);
      int chanend = std::min(static_cast<int>(X.size()),chan+PeakRadius);
      double rowcentroid = 0.0;
      int rowstart = std::max(0,row-PeakRadius);
      int rowend = std::min(RDet->ypixels()-1,row+PeakRadius);
      double colcentroid = 0.0;
      int colstart = std::max(0,col-PeakRadius);
      int colend = std::min(RDet->xpixels()-1,col+PeakRadius);
      for (int ichan=chanstart; ichan<=chanend; ++ichan)
      {
        for (int irow=rowstart; irow<=rowend; ++irow)
        {
          for (int icol=colstart; icol<=colend; ++icol)
          {
            boost::shared_ptr<Detector> pixel = RDet->getAtXY(icol, irow);
            Mantid::detid2index_map::iterator it;
            it = (*wi_to_detid_map).find(pixel->getID());
            if (it == (*wi_to_detid_map).end())continue;
            size_t workspaceIndex = (it->second);
    
            Mantid::MantidVec X = inWS->readX(workspaceIndex);
            Mantid::MantidVec histogram = inWS->readY(workspaceIndex);
    
            intensity += histogram[ichan];
            rowcentroid += irow*histogram[ichan];
            colcentroid += icol*histogram[ichan];
            chancentroid += ichan*histogram[ichan];
          }
        }
      }
    // Set pixelID to change row and col
      row = std::min(RDet->ypixels()-1,int(rowcentroid/intensity));
      row = std::max(0,row);
      col = std::min(RDet->xpixels()-1,int(colcentroid/intensity));
      col = std::max(0,col);
      pixel = RDet->getAtXY(col, row);
      peak.setDetectorID(pixel->getID());
    // Set wavelength to change tof for peak object
      it = (*wi_to_detid_map).find(pixel->getID());
      workspaceIndex = (it->second);

      X = inWS->readX(workspaceIndex);
      histogram = inWS->readY(workspaceIndex);

      chan = int(chancentroid/intensity);
      chan = std::max(0,chan);
      chan = std::min(static_cast<int>(X.size()),chan);
      Mantid::Kernel::Units::Wavelength wl;
      std::vector<double> timeflight;
      timeflight.push_back(X[chan]);
      double scattering = peak.getScattering();
      double L1 = peak.getL1();
      double L2 = peak.getL2();
      wl.fromTOF(timeflight, timeflight, L1, L2, scattering, 0, 0, 0);
      const double lambda = timeflight[0];
      timeflight.clear();

      peak.setWavelength(lambda);
      peak.setBinCount(histogram[chan]);
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    for (int i= int(peakWS->getNumberPeaks())-1; i>=0; --i)
    {
      // Get a direct ref to that peak.
      IPeak & peak = peakWS->getPeak(i);
      int col = peak.getCol();
      int row = peak.getRow();
      Geometry::Instrument_const_sptr Iptr = peak.getInstrument();
      std::string bankName = peak.getBankName();

      boost::shared_ptr<const IComponent> parent = Iptr->getComponentByName(bankName);

      if (!parent) continue;
      if (parent->type().compare("RectangularDetector") != 0) continue;
      boost::shared_ptr<const RectangularDetector> RDet = boost::shared_dynamic_cast<
                const RectangularDetector>(parent);
      int Edge = getProperty("EdgePixels");
      if (col < Edge || col > (RDet->xpixels()-Edge) || row < Edge || row > (RDet->ypixels()-Edge))
      {
        peakWS->removePeak(i);
      }
    }
    // Save the output
    setProperty("OutPeaksWorkspace", peakWS);

  }

  //----------------------------------------------------------------------------------------------
  /** Integrate the peaks of the workspace using parameters saved in the algorithm class
   * @param inWS ::  2D Workspace to integrate
   */
  void CentroidPeaks::integrateEvent()
  {

    /// Peak workspace to centroid
    Mantid::DataObjects::PeaksWorkspace_sptr inPeakWS = getProperty("InPeaksWorkspace");

    /// Output peaks workspace, create if needed
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS = getProperty("OutPeaksWorkspace");
    if (peakWS != inPeakWS)
      peakWS = inPeakWS->clone();

    //To get the workspace index from the detector ID
    detid2index_map * pixel_to_wi = inWS->getDetectorIDToWorkspaceIndexMap(true);

    /// Radius to use around peaks
    int PeakRadius = getProperty("PeakRadius");

    int MinPeaks = -1;
    int MaxPeaks = -1;
    size_t Numberwi = inWS->getNumberHistograms();
    int NumberPeaks = peakWS->getNumberPeaks();
    for (int i = 0; i<NumberPeaks; i++)
    {
      Peak & peak = peakWS->getPeaks()[i];
      int pixelID = peak.getDetectorID();

      // Find the workspace index for this detector ID
      if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
      {
         size_t wi = (*pixel_to_wi)[pixelID];
         if(MinPeaks == -1 && peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi) MinPeaks = i;
         if(peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi) MaxPeaks = i;
      }
    }
    Progress prog(this, MinPeaks, 1.0, MaxPeaks);
    PARALLEL_FOR2(inWS,peakWS)
    for (int i = MinPeaks; i<= MaxPeaks; i++)
    {
      PARALLEL_START_INTERUPT_REGION
      // Get a direct ref to that peak.
      IPeak & peak = peakWS->getPeak(i);
      int col = peak.getCol();
      int row = peak.getRow();
      double TOFPeakd = peak.getTOF();
      Geometry::Instrument_const_sptr Iptr = peak.getInstrument();
      std::string bankName = peak.getBankName();

      boost::shared_ptr<const IComponent> parent = Iptr->getComponentByName(bankName);

      if (!parent) continue;
      if (parent->type().compare("RectangularDetector") != 0) continue;
      boost::shared_ptr<const RectangularDetector> RDet = boost::shared_dynamic_cast<
                const RectangularDetector>(parent);

      double intensity = 0.0;
      double tofcentroid = 0.0;
      boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
      Mantid::detid2index_map::iterator it;
      it = (*wi_to_detid_map).find(pixel->getID());

      double tofstart = TOFPeakd*std::pow(1.004,-PeakRadius);
      double tofend = TOFPeakd*std::pow(1.004,PeakRadius);
      double rowcentroid = 0.0;
      int rowstart = std::max(0,row-PeakRadius);
      int rowend = std::min(RDet->ypixels()-1,row+PeakRadius);
      double colcentroid = 0.0;
      int colstart = std::max(0,col-PeakRadius);
      int colend = std::min(RDet->xpixels()-1,col+PeakRadius);
      for (int irow=rowstart; irow<=rowend; ++irow)
      {
        for (int icol=colstart; icol<=colend; ++icol)
        {
          boost::shared_ptr<Detector> pixel = RDet->getAtXY(icol, irow);
          Mantid::detid2index_map::iterator it;
          it = (*wi_to_detid_map).find(pixel->getID());
          size_t workspaceIndex = (it->second);
          EventList el = eventW->getEventList(workspaceIndex);
          el.switchTo(WEIGHTED_NOTIME);
          std::vector<WeightedEventNoTime> events = el.getWeightedEventsNoTime();

          std::vector<WeightedEventNoTime>::iterator itev;
          std::vector<WeightedEventNoTime>::iterator itev_end = events.end();

          // Check for events in tof range
          for (itev = events.begin(); itev != itev_end; ++itev)
          {
            double tof = itev->tof();
            if( tof > tofstart && tof < tofend)
            {
              double weight = itev->weight();
              intensity += weight;
              rowcentroid += irow*weight;
              colcentroid += icol*weight;
              tofcentroid += tof*weight;
            }
          }
        }
      }
    // Set pixelID to change row and col
      row = std::min(RDet->ypixels()-1,int(rowcentroid/intensity));
      row = std::max(0,row);
      col = std::min(RDet->xpixels()-1,int(colcentroid/intensity));
      col = std::max(0,col);
      pixel = RDet->getAtXY(col, row);
      peak.setDetectorID(pixel->getID());

    // Set wavelength to change tof for peak object
      double tof = tofcentroid/intensity;
      Mantid::Kernel::Units::Wavelength wl;
      std::vector<double> timeflight;
      timeflight.push_back(tof);
      double scattering = peak.getScattering();
      double L1 = peak.getL1();
      double L2 = peak.getL2();
      wl.fromTOF(timeflight, timeflight, L1, L2, scattering, 0, 0, 0);
      const double lambda = timeflight[0];
      timeflight.clear();

      peak.setWavelength(lambda);
      peak.setBinCount(intensity);
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    for (int i= int(peakWS->getNumberPeaks())-1; i>=0; --i)
    {
      // Get a direct ref to that peak.
      IPeak & peak = peakWS->getPeak(i);
      int col = peak.getCol();
      int row = peak.getRow();
      Geometry::Instrument_const_sptr Iptr = peak.getInstrument();
      std::string bankName = peak.getBankName();

      boost::shared_ptr<const IComponent> parent = Iptr->getComponentByName(bankName);

      if (!parent) continue;
      if (parent->type().compare("RectangularDetector") != 0) continue;
      boost::shared_ptr<const RectangularDetector> RDet = boost::shared_dynamic_cast<
                const RectangularDetector>(parent);
      int Edge = getProperty("EdgePixels");
      if (col < Edge || col > (RDet->xpixels()-Edge) || row < Edge || row > (RDet->ypixels()-Edge))
      {
        peakWS->removePeak(i);
      }
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

    eventW = boost::dynamic_pointer_cast<EventWorkspace>( inWS );
    if(eventW)
    {
      eventW->sortAll(TOF_SORT, NULL);
      this->integrateEvent();
    }
    else
    {
      this->integrate();
    }
  }

} // namespace Mantid
} // namespace Crystal
