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
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <ostream>
#include <iomanip>
#include <sstream>

namespace Mantid
{
  namespace Crystal
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(MaskPeaksWorkspace)

    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;


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

      declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace", "", Direction::Input)
          , "A 2D event workspace");

      declareProperty(new WorkspaceProperty<PeaksWorkspace>("InPeaksWorkspace", "", Direction::Input), "Name of the peaks workspace.");

      declareProperty("XMin", -2, "Minimum of X (col) Range to mask peak");
      declareProperty("XMax", 2, "Maximum of X (col) Range to mask peak");
      declareProperty("YMin", -2, "Minimum of Y (row) Range to mask peak");
      declareProperty("YMax", 2, "Maximum of Y (row) Range to mask peak");

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


      int i, XPeak, YPeak;
      double col, row;

      // Build a map to sort by the peak bin count
      std::vector <std::pair<double, int> > v1;
      for (i = 0; i<peaksW->getNumberPeaks(); i++)
      {
        v1.push_back(std::pair<double, int>(peaksW->getPeaks()[i].getBinCount(), i));
      }
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
 
      // Loop of peaks
      std::vector <std::pair<double, int> >::iterator Iter1;
      for ( Iter1 = v1.begin() ; Iter1 != v1.end() ; ++Iter1 )
      {
        i = (*Iter1).second;
        // Direct ref to that peak
        Peak & peak = peaksW->getPeaks()[i];

        col = peak.getCol();
        row = peak.getRow();
        Kernel::V3D pos = peak.getDetPos();

        XPeak = int(col+0.5)-1;
        YPeak = int(row+0.5)-1;
  
        Geometry::IComponent_const_sptr comp = inst->getComponentByName(peak.getBankName());
        if (!comp) throw std::invalid_argument("Component "+peak.getBankName()+" does not exist in instrument");
        Geometry::RectangularDetector_const_sptr det = boost::dynamic_pointer_cast<const Geometry::RectangularDetector>(comp);
        if (!det) throw std::invalid_argument("Component "+peak.getBankName()+" is not a rectangular detector");
        for (int ix=Xmin; ix <= Xmax; ix++)
          for (int iy=Ymin; iy <= Ymax; iy++)
          {
            //Find the pixel ID at that XY position on the rectangular detector
            if(XPeak+ix >= det->xpixels() || XPeak+ix < 0)continue;
            if(YPeak+iy >= det->ypixels() || YPeak+iy < 0)continue;
            int pixelID = det->getAtXY(XPeak+ix,YPeak+iy)->getID();


            //Find the corresponding workspace index, if any
            if (pixel_to_wi->find(pixelID) != pixel_to_wi->end())
            {
              size_t wi = (*pixel_to_wi)[pixelID];
              const MantidVec& X = inputW->readX(wi);

              // Add information to TableWorkspace
              API::TableRow newrow = tablews->appendRow();
              newrow << X[0] <<  X[X.size()-1];
              std::stringstream ss;
              ss << wi;
              newrow << ss.str();

              // inputW->getEventList(wi).maskTof(X[0],X[X.size()-1]);
              // std::cout << wi << ", " << X[0] << ", " << X[X.size()-1] << std::endl;
            }
          }
      } // ENDFOR(Iter1)

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
      Xmin = getProperty("XMin");
      Xmax = getProperty("XMax");
      Ymin = getProperty("YMin");
      Ymax = getProperty("YMax");
      if (Xmin >=  Xmax)
        throw std::runtime_error("Must specify Xmin<Xmax");
      if (Ymin >=  Ymax)
        throw std::runtime_error("Must specify Ymin<Ymax");
    }



  } // namespace Crystal
} // namespace Mantid
