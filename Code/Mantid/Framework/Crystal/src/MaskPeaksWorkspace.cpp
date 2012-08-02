/*WIKI* 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/MaskPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Strings.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <set>

namespace Mantid
{
  namespace Crystal
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(MaskPeaksWorkspace)

    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;
    using std::string;


    /// Constructor
    MaskPeaksWorkspace::MaskPeaksWorkspace()
    {}

    /// Destructor
    MaskPeaksWorkspace::~MaskPeaksWorkspace()
    {}

    /** Initialisation method. Declares properties to be used in algorithm.
     *
     */
    void MaskPeaksWorkspace::init()
    {

      declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace", "", Direction::Input),
                      "A 2D event workspace");
      declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace", "", Direction::Input),
                      "Name of the peaks workspace.");
      declareProperty("XMin", -2, "Minimum of X (col) Range to mask peak");
      declareProperty("XMax", 2, "Maximum of X (col) Range to mask peak");
      declareProperty("YMin", -2, "Minimum of Y (row) Range to mask peak");
      declareProperty("YMax", 2, "Maximum of Y (row) Range to mask peak");
      declareProperty("TOFMin", EMPTY_DBL(), "Minimum TOF relative to peak's center TOF.");
      declareProperty("TOFMax", EMPTY_DBL(), "Maximum TOF relative to peak's center TOF.");
    }

    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     */
    void MaskPeaksWorkspace::exec()
    {
      retrieveProperties();
  
      MantidVecPtr XValues;
      PeaksWorkspace_sptr peaksW;
      peaksW = AnalysisDataService::Instance().retrieveWS<PeaksWorkspace>(getProperty("InPeaksWorkspace"));

      //To get the workspace index from the detector ID
      detid2index_map * pixel_to_wi = inputW->getDetectorIDToWorkspaceIndexMap(true);
      //Get some stuff from the input workspace
      Geometry::Instrument_const_sptr inst = inputW->getInstrument();
      if (!inst)
        throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");

      // Init a table workspace
      DataObjects::TableWorkspace_sptr tablews =
          boost::shared_ptr<DataObjects::TableWorkspace>(new DataObjects::TableWorkspace());
      tablews->addColumn("double", "XMin");
      tablews->addColumn("double", "XMax");
      tablews->addColumn("str", "SpectraList");
 
      // Loop over peaks
      const std::vector<Peak> & peaks = peaksW->getPeaks();
      std::vector <std::pair<double, int> >::iterator Iter1;
      for ( auto peak = peaks.begin(); peak != peaks.end(); ++peak )
      {
        // get the peak location on the detector
        double col = peak->getCol();
        double row = peak->getRow();
        int xPeak = int(col+0.5)-1;
        int yPeak = int(row+0.5)-1;
        g_log.debug() << "Generating information for peak at x=" << xPeak << " y=" << yPeak << "\n";
  
        // the detector component for the peak will have all pixels that we mask
        const string bankName = peak->getBankName();
        Geometry::IComponent_const_sptr comp = inst->getComponentByName(bankName);
        if (!comp)
        {
          throw std::invalid_argument("Component "+bankName+" does not exist in instrument");
        }
        Geometry::RectangularDetector_const_sptr det
            = boost::dynamic_pointer_cast<const Geometry::RectangularDetector>(comp);
        if (!det)
        {
          throw std::invalid_argument("Component "+bankName+" is not a rectangular detector");
        }

        // determine the range in time-of-flight
        double x0;
        double xf;
        { // scope limit the workspace index
          size_t wi = this->getWkspIndex(pixel_to_wi, det, xPeak, yPeak);
          this->getTofRange(x0, xf, peak->getTOF(), inputW->readX(wi));
        }

        // determine the spectrum numbers to mask
        std::set<size_t> spectra;
        for (int ix=m_xMin; ix <= m_xMax; ix++)
        {
          for (int iy=m_yMin; iy <= m_yMax; iy++)
          {
            //Find the pixel ID at that XY position on the rectangular detector
            if(xPeak+ix >= det->xpixels() || xPeak+ix < 0)continue;
            if(yPeak+iy >= det->ypixels() || yPeak+iy < 0)continue;
            spectra.insert(this->getWkspIndex(pixel_to_wi, det, xPeak+ix, yPeak+iy));
          }
        }

        // append to the table workspace
        API::TableRow newrow = tablews->appendRow();
        newrow << x0 << xf << Kernel::Strings::toString(spectra);
      } // end loop over peaks


      // Mask bins
      API::IAlgorithm_sptr maskbinstb = this->createSubAlgorithm("MaskBinsFromTable", 0.5, 1.0, true);
      maskbinstb->initialize();
      maskbinstb->setPropertyValue("InputWorkspace", inputW->getName());
      maskbinstb->setPropertyValue("OutputWorkspace", inputW->getName());
      maskbinstb->setProperty("MaskingInformation", tablews);
      maskbinstb->execute();

      //Clean up memory
      delete pixel_to_wi;

      return;
    }

    void MaskPeaksWorkspace::retrieveProperties()
    {
      inputW = getProperty("InputWorkspace");

      m_xMin = getProperty("XMin");
      m_xMax = getProperty("XMax");
      if (m_xMin >=  m_xMax)
        throw std::runtime_error("Must specify Xmin<Xmax");

      m_yMin = getProperty("YMin");
      m_yMax = getProperty("YMax");
      if (m_yMin >=  m_yMax)
        throw std::runtime_error("Must specify Ymin<Ymax");

      // Get the value of TOF range to mask
      m_tofMin = getProperty("TOFMin");
      m_tofMax = getProperty("TOFMax");
      if ((!isEmpty(m_tofMin)) && (!isEmpty(m_tofMax)))
      {
        if (m_tofMin >= m_tofMax)
          throw std::runtime_error("Must specify TOFMin < TOFMax");
      }
      else if ((!isEmpty(m_tofMin)) || (!isEmpty(m_tofMax))) // check if only one is empty
      {
        throw std::runtime_error("Must specify both TOFMin and TOFMax or neither");
      }
    }

    size_t MaskPeaksWorkspace::getWkspIndex(detid2index_map *pixel_to_wi, Geometry::RectangularDetector_const_sptr det,
                                            const int x, const int y)
    {
      int pixelID = det->getAtXY(x,y)->getID();

      //Find the corresponding workspace index, if any
      if (pixel_to_wi->find(pixelID) == pixel_to_wi->end())
      {
        std::stringstream msg;
        msg << "Failed to find workspace index for x=" << x << " y=" << y;
        throw std::runtime_error(msg.str());
      }
      return (*pixel_to_wi)[pixelID];
    }

    /**
     * @param tofMin Return value for minimum tof to be masked
     * @param tofMax Return value for maximum tof to be masked
     * @param tofPeak time-of-flight of the single crystal peak
     * @param tof tof-of-flight axis for the spectrum where the peak supposedly exists
     */
    void MaskPeaksWorkspace::getTofRange(double &tofMin, double &tofMax, const double tofPeak, const MantidVec& tof)
    {
      tofMin = tof.front();
      tofMax = tof.back()-1;
      if (!isEmpty(m_tofMin))
      {
          tofMin = tofPeak + m_tofMin;
      }
      if (!isEmpty(m_tofMax))
      {
          tofMax = tofPeak + m_tofMax;
      }
    }

  } // namespace Crystal
} // namespace Mantid
