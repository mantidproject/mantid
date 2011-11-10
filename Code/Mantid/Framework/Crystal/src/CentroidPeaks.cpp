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


    /// Radius to use around peaks
    int PeakRadius = getProperty("PeakRadius");

    PARALLEL_FOR2(inWS,peakWS)
    for (int i=0; i < int(peakWS->getNumberPeaks()); ++i)
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

      Mantid::MantidVec X = inWS->readX(workspaceIndex);
      Mantid::MantidVec histogram = inWS->readY(workspaceIndex);

      int chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
      int chanstart = std::max(0,chan-PeakRadius);
      int chanend = std::min(static_cast<int>(X.size()),chan+PeakRadius);
      double rowcentroid = 0.0;
      int roinWStart = std::max(0,row-PeakRadius);
      int rowend = std::min(RDet->ypixels()-1,row+PeakRadius);
      double colcentroid = 0.0;
      int colstart = std::max(0,col-PeakRadius);
      int colend = std::min(RDet->xpixels()-1,col+PeakRadius);
      for (int ichan=chanstart; ichan<=chanend; ++ichan)
      {
        for (int irow=roinWStart; irow<=rowend; ++irow)
        {
          for (int icol=colstart; icol<=colend; ++icol)
          {
            boost::shared_ptr<Detector> pixel = RDet->getAtXY(icol, irow);
            Mantid::detid2index_map::iterator it;
            it = (*wi_to_detid_map).find(pixel->getID());
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

      if (parent->type().compare("RectangularDetector") != 0)
      {

        std::cout<<"   getPanel C type="<<parent->type()<<std::endl;
                throw std::runtime_error("Improper Peak Argument");
      }
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


    /// Radius to use around peaks
    int PeakRadius = getProperty("PeakRadius");

    PARALLEL_FOR2(inWS,peakWS)
    for (int i=0; i < int(peakWS->getNumberPeaks()); ++i)
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

      Mantid::MantidVec X = inWS->readX(workspaceIndex);
      Mantid::MantidVec histogram = inWS->readY(workspaceIndex);

      int chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
      int chanstart = std::max(0,chan-PeakRadius);
      int chanend = std::min(static_cast<int>(X.size()),chan+PeakRadius);
      double rowcentroid = 0.0;
      int roinWStart = std::max(0,row-PeakRadius);
      int rowend = std::min(RDet->ypixels()-1,row+PeakRadius);
      double colcentroid = 0.0;
      int colstart = std::max(0,col-PeakRadius);
      int colend = std::min(RDet->xpixels()-1,col+PeakRadius);
      for (int ichan=chanstart; ichan<=chanend; ++ichan)
      {
        for (int irow=roinWStart; irow<=rowend; ++irow)
        {
          for (int icol=colstart; icol<=colend; ++icol)
          {
            boost::shared_ptr<Detector> pixel = RDet->getAtXY(icol, irow);
            Mantid::detid2index_map::iterator it;
            it = (*wi_to_detid_map).find(pixel->getID());
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

      if (parent->type().compare("RectangularDetector") != 0)
      {

        std::cout<<"   getPanel C type="<<parent->type()<<std::endl;
                throw std::runtime_error("Improper Peak Argument");
      }
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
