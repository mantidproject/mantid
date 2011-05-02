//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/MaskPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <ostream>
#include <iomanip>
#include <sstream>

namespace Mantid
{
  namespace DataHandling
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

      declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input, new EventWorkspaceValidator<>)
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
      peaksW = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(getProperty("InPeaksWorkspace")));


      int i, XPeak, YPeak;
      double col, row, l1, l2, wl;

      // Build a map to sort by the peak bin count
      std::vector <std::pair<double, int> > v1;
      for (i = 0; i<peaksW->getNumberPeaks(); i++)
        v1.push_back(std::pair<double, int>(peaksW->getPeaks()[i].getBinCount(), i));

      //To get the workspace index from the detector ID
      IndexToIndexMap * pixel_to_wi = inputW->getDetectorIDToWorkspaceIndexMap(true);
      //Get some stuff from the input workspace
      Geometry::IInstrument_sptr inst = inputW->getInstrument();
      if (!inst)
        throw std::runtime_error("The InputWorkspace does not have a valid instrument attached to it!");

 
      std::vector <std::pair<double, int> >::iterator Iter1;
      for ( Iter1 = v1.begin() ; Iter1 != v1.end() ; Iter1++ )
      {
        i = (*Iter1).second;
        // Direct ref to that peak
        Peak & peak = peaksW->getPeaks()[i];

        l1 = peak.getL1();
        col = peak.getCol();
        row = peak.getRow();
        Geometry::V3D pos = peak.getDetPos();
        l2 = pos.norm();
        wl = peak.getWavelength();

        XPeak = int(col+0.5)-1;
        YPeak = int(row+0.5)-1;
  
        boost::shared_ptr<Geometry::IComponent> comp = inst->getComponentByName(peak.getBankName());
        if (!comp) throw std::invalid_argument("Component "+peak.getBankName()+" does not exist in instrument");
        boost::shared_ptr<Geometry::RectangularDetector> det = boost::dynamic_pointer_cast<Geometry::RectangularDetector>(comp);
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
              int wi = (*pixel_to_wi)[pixelID];
              const MantidVec& X = inputW->readX(wi);
              eventW->getEventList(wi).maskTof(X[0],X[X.size()-1]);
            }
          }

      }
    //Clean up memory
    delete pixel_to_wi;
    }

    void MaskPeaksWorkspace::retrieveProperties()
    {
      inputW = getProperty("InputWorkspace");
      eventW = boost::dynamic_pointer_cast<EventWorkspace>( inputW );
      Xmin = getProperty("XMin");
      Xmax = getProperty("XMax");
      Ymin = getProperty("YMin");
      Ymax = getProperty("YMax");
      if (Xmin >=  Xmax)
        throw std::runtime_error("Must specify Xmin<Xmax");
      if (Ymin >=  Ymax)
        throw std::runtime_error("Must specify Ymin<Ymax");
    }



  } // namespace DataHandling
} // namespace Mantid
