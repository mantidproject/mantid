#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidCrystal/CentroidPeaks.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidAPI/MemoryManager.h"

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
  CentroidPeaks::CentroidPeaks() : wi_to_detid_map(NULL)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CentroidPeaks::~CentroidPeaks()
  {
    delete wi_to_detid_map;
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

    int MinPeaks = -1;
    int MaxPeaks = -1;
    size_t Numberwi = inWS->getNumberHistograms();
    int NumberPeaks = peakWS->getNumberPeaks();
    for (int i = 0; i<NumberPeaks; i++)
    {
      Peak & peak = peakWS->getPeaks()[i];
      int pixelID = peak.getDetectorID();

      // Find the workspace index for this detector ID
      if (wi_to_detid_map->find(pixelID) != wi_to_detid_map->end())
      {
         size_t wi = (*wi_to_detid_map)[pixelID];
         if(MinPeaks == -1 && peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi) MinPeaks = i;
         if(peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi) MaxPeaks = i;
      }
    }
    Geometry::Instrument_const_sptr Iptr = inWS->getInstrument();
    Progress prog(this, MinPeaks, 1.0, MaxPeaks);
    PARALLEL_FOR2(inWS,peakWS)
    for (int i = MinPeaks; i<= MaxPeaks; i++)
    {
      PARALLEL_START_INTERUPT_REGION
      // Get a direct ref to that peak.
      IPeak & peak = peakWS->getPeak(i);
      int col = peak.getCol();
      int row = peak.getRow();
      int pixelID = peak.getDetectorID();
      detid2index_map::const_iterator it = wi_to_detid_map->find(pixelID);
      if ( it == wi_to_detid_map->end() )
      {
        continue;
      }
      size_t workspaceIndex = it->second;
      double TOFPeakd = peak.getTOF();
      const MantidVec & X = inWS->readX(workspaceIndex);
      int chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
      std::string bankName = peak.getBankName();

      boost::shared_ptr<const IComponent> parent = Iptr->getComponentByName(bankName);
      if (!parent) continue;
      if (parent->type().compare("RectangularDetector") != 0)
      {
          int nPixels = std::max<int>(0, getProperty("EdgePixels"));
          std::vector<Geometry::IComponent_const_sptr> children;
          boost::shared_ptr<const Geometry::ICompAssembly> asmb = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
          asmb->getChildren(children, false);
          boost::shared_ptr<const Geometry::ICompAssembly> asmb2 = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
          std::vector<Geometry::IComponent_const_sptr> grandchildren;
          asmb2->getChildren(grandchildren,false);
          int NROWS = static_cast<int>(grandchildren.size());
          int NCOLS = static_cast<int>(children.size());
    	  if (row < nPixels || col < nPixels || NROWS-row < nPixels || NCOLS-col < nPixels) continue;
    	  //Only works for WISH for non-RectangularDetector  TODO-make more general
    	  //if (bankName.compare("WISH") != 0)continue;
          IAlgorithm_sptr slice_alg = createSubAlgorithm("IntegratePeakTimeSlices");
          slice_alg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", inWS);
          std::ostringstream tab_str;
          tab_str << "LogTable" << i;

          slice_alg->setPropertyValue("OutputWorkspace", tab_str.str());
          slice_alg->setProperty<PeaksWorkspace_sptr>("Peaks", peakWS);
          slice_alg->setProperty("PeakIndex", i);
          double qspan = 0.12;
		  if (peakWS->mutableSample().hasOrientedLattice())
		  {
		    OrientedLattice latt = peakWS->mutableSample().getOrientedLattice();
		    qspan = 1./std::max(latt.a(), std::max(latt.b(),latt.c()));//1/6*2Pi about 1
		  }
          slice_alg->setProperty("PeakQspan", qspan);


          slice_alg->setProperty("NBadEdgePixels", nPixels);
          try
          {
          slice_alg->executeAsSubAlg();
          Mantid::API::MemoryManager::Instance().releaseFreeMemory();

          TableWorkspace_sptr logtable = slice_alg->getProperty("OutputWorkspace");

          double Imax = 0;
          for (int iTOF=0; iTOF < static_cast<int>(logtable->rowCount()); iTOF++)
          {
              double intensity = logtable->getRef<double>(std::string("Intensity"), iTOF);
              if (intensity > Imax)
              {
				  int irow = static_cast<int>(logtable->getRef<double>(std::string("Mrow"), iTOF));
				  int icol = static_cast<int>(logtable->getRef<double>(std::string("Mcol"), iTOF));
				  std::string bankName0 = bankName;
				  bankName0.erase(0,4);
				  std::ostringstream pixelString;
				  pixelString << Iptr->getName() << "/" << bankName0 << "/"
				      << bankName << "/tube" << std::setw(3) << std::setfill('0')<< icol << "/pixel" << std::setw(4) << std::setfill('0')<< irow;
				  Geometry::IComponent_const_sptr component = Iptr->getComponentByName(pixelString.str());
                                  boost::shared_ptr<const Detector> pixel = boost::dynamic_pointer_cast<const Detector>(component);
				  if (pixel) pixelID = pixel->getID();
				  chan = static_cast<int>(logtable->getRef<double>(std::string("Channel"), iTOF));
              }
          }
          } catch (...)
          {

             g_log.debug("Error in IntegratePeakTimeSlices");
          }
      }
	  else
      {
      boost::shared_ptr<const RectangularDetector> RDet = boost::shared_dynamic_cast<
                const RectangularDetector>(parent);
      double intensity = 0.0;
      double chancentroid = 0.0;
      boost::shared_ptr<Detector> pixel = RDet->getAtXY(col, row);
      it = wi_to_detid_map->find(pixel->getID());
      if ( it == wi_to_detid_map->end() )
      {
        continue;
      }
      workspaceIndex = it->second;

      const MantidVec & X = inWS->readX(workspaceIndex);

      chan = Kernel::VectorHelper::getBinIndex(X, TOFPeakd);
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
            detid2index_map::const_iterator it = wi_to_detid_map->find(pixel->getID());
            if ( it == wi_to_detid_map->end() ) continue;
            size_t workspaceIndex = (it->second);
    
            const MantidVec & histogram = inWS->readY(workspaceIndex);
    
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
      pixelID = pixel->getID();
      chan = int(chancentroid/intensity);
      chan = std::max(0,chan);
      chan = std::min(static_cast<int>(inWS->blocksize()),chan);
      }
      peak.setDetectorID(pixelID);
    // Set wavelength to change tof for peak object
      it = wi_to_detid_map->find(pixelID);
      workspaceIndex = (it->second);
      Mantid::Kernel::Units::Wavelength wl;
      std::vector<double> timeflight;
      timeflight.push_back(inWS->readX(workspaceIndex)[chan]);
      double scattering = peak.getScattering();
      double L1 = peak.getL1();
      double L2 = peak.getL2();
      wl.fromTOF(timeflight, timeflight, L1, L2, scattering, 0, 0, 0);
      const double lambda = timeflight[0];
      timeflight.clear();

      peak.setWavelength(lambda);
      peak.setBinCount(inWS->readY(workspaceIndex)[chan]);
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    for (int i= int(peakWS->getNumberPeaks())-1; i>=0; --i)
    {
      // Get a direct ref to that peak.
      IPeak & peak = peakWS->getPeak(i);
      int col = peak.getCol();
      int row = peak.getRow();
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

    int MinPeaks = -1;
    int MaxPeaks = -1;
    size_t Numberwi = inWS->getNumberHistograms();
    int NumberPeaks = peakWS->getNumberPeaks();
    for (int i = 0; i<NumberPeaks; i++)
    {
      Peak & peak = peakWS->getPeaks()[i];
      int pixelID = peak.getDetectorID();

      // Find the workspace index for this detector ID
      if (wi_to_detid_map->find(pixelID) != wi_to_detid_map->end())
      {
         size_t wi = (*wi_to_detid_map)[pixelID];
         if(MinPeaks == -1 && peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi) MinPeaks = i;
         if(peak.getRunNumber() == inWS->getRunNumber() && wi < Numberwi) MaxPeaks = i;
      }
    }
    Geometry::Instrument_const_sptr Iptr = inWS->getInstrument();
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
          it = wi_to_detid_map->find(pixel->getID());
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
    wi_to_detid_map = inWS->getDetectorIDToWorkspaceIndexMap(false);

    eventW = boost::dynamic_pointer_cast<const EventWorkspace>( inWS );
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
