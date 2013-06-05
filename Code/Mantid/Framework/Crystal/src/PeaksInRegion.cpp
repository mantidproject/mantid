/*WIKI*
Determines which peaks intersect a defined region in either QLab, QSample or HKL space.
*WIKI*/

#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <vector>
#include "MantidCrystal/PeaksInRegion.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Crystal
{

  std::string detectorSpaceFrame()
  {
    return "Detector space";
  }

  std::string qLabFrame()
  {
    return "Q (lab frame)";
  }

  std::string qSampleFrame()
  {
    return "Q (sample frame)";
  }

  std::string hklFrame()
  {
    return "HKL";
  }

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PeaksInRegion)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PeaksInRegion::PeaksInRegion()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PeaksInRegion::~PeaksInRegion()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string PeaksInRegion::name() const { return "PeaksInRegion";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int PeaksInRegion::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string PeaksInRegion::category() const { return "crystal";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PeaksInRegion::initDocs()
  {
    this->setWikiSummary("Find peaks intersecting a region.");
    this->setOptionalMessage(this->getWikiSummary());
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PeaksInRegion::init()
  {
    declareProperty(new WorkspaceProperty<IPeaksWorkspace>("InputWorkspace","",Direction::Input), "An input peaks workspace.");

    std::vector<std::string> propOptions;
    propOptions.push_back(detectorSpaceFrame());
    propOptions.push_back(qLabFrame());
    propOptions.push_back(qSampleFrame());
    propOptions.push_back(hklFrame());

    declareProperty("CoordinateFrame", "DetectorSpace" ,boost::make_shared<StringListValidator>(propOptions),
      "What coordinate system to use for intersection criteria?\n"
      "  DetectorSpace: Real-space coordinates.\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Momentum in the sample frame.\n"
      "  HKL"
       );

    declareProperty("CheckPeakExtents", false, "Include any peak in the region that has a shape extent extending into that region.");

    declareProperty("PeakRadius", 0.0, "Effective peak radius in CoordinateFrame");

    auto manditoryExtents = boost::make_shared<Mantid::Kernel::MandatoryValidator<std::vector<double> > >();

    std::vector<double> extents(2,0);
    extents[0]=-50;extents[1]=+50;
    declareProperty(
      new ArrayProperty<double>("Extents", extents, manditoryExtents),
      "A comma separated list of min, max for each dimension,\n"
      "specifying the extents of each dimension. Optional, default +-50 in each dimension.");

    declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace","",Direction::Output), "An output table workspace. Two columns. Peak index into input workspace, and boolean, where true is for positive intersection.");
  
    setPropertySettings("PeakRadius", new EnabledWhenProperty("CheckPeakExtents", IS_NOT_DEFAULT) );
  }

  void validateExtentsInput(std::vector<double>& extents)
  {
    if(extents.size() != 6)
    {
      throw std::invalid_argument("Six commma separated entries for the extents expected");
    }
    if(extents[0] >= extents[1])
    {
      throw std::invalid_argument("xmin >= xmax");
    }
    if(extents[2] >= extents[3])
    {
      throw std::invalid_argument("ymin >= ymax");
    }
    if(extents[4] >= extents[5])
    {
      throw std::invalid_argument("zmin >= zmax");
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void PeaksInRegion::exec()
  {
    std::vector<double> extents = this->getProperty("Extents");
    validateExtentsInput(extents);

    const std::string coordinateFrame = this->getPropertyValue("CoordinateFrame");
    IPeaksWorkspace_sptr ws = this->getProperty("InputWorkspace");
    const bool checkPeakExtents = this->getProperty("CheckPeakExtents");
    const double peakRadius = this->getProperty("PeakRadius");

    boost::function<V3D(IPeak*)> coordFrameFunc = &IPeak::getHKL;
    if(coordinateFrame == detectorSpaceFrame())
    {
      coordFrameFunc = &IPeak::getDetectorPosition;
    }
    else if(coordinateFrame == qLabFrame())
    {
      coordFrameFunc = &IPeak::getQLabFrame;
    }
    else if(coordinateFrame == qSampleFrame())
    {
      coordFrameFunc = &IPeak::getQSampleFrame;
    }

    const int nPeaks = ws->getNumberPeaks();

    Mantid::DataObjects::TableWorkspace_sptr outputWorkspace = boost::make_shared<Mantid::DataObjects::TableWorkspace>(ws->rowCount());
    outputWorkspace->addColumn("int", "PeakIndex");
    outputWorkspace->addColumn("bool", "Intersecting");

    const int minXIndex = 0;
    const int maxXIndex = 1;
    const int minYIndex = 2;
    const int maxYIndex = 3;
    const int minZIndex = 4;
    const int maxZIndex = 5;

    // Clockwise ordering of points around the extents box
    /*

    on front face. Positive z extends into plane.

    p2|---|p3
      |   |
    p1|---|p4
    */
    V3D point1(extents[minXIndex], extents[minYIndex], extents[minZIndex]);
    V3D point2(extents[minXIndex], extents[maxYIndex], extents[minZIndex]);
    V3D point3(extents[maxXIndex], extents[maxYIndex], extents[minZIndex]);
    V3D point4(extents[maxXIndex], extents[minYIndex], extents[minZIndex]);
    V3D point5(extents[minXIndex], extents[minYIndex], extents[maxZIndex]);
    V3D point6(extents[minXIndex], extents[maxYIndex], extents[maxZIndex]);
    V3D point7(extents[maxXIndex], extents[maxYIndex], extents[maxZIndex]);
    V3D point8(extents[maxXIndex], extents[minYIndex], extents[maxZIndex]);

    V3D faces[6][3] =
    { 
      {point1, point5, point6} // These define a face normal to x at xmin.
      ,{point4, point7, point8} // These define a face normal to x at xmax.
      ,{point1, point4, point8} // These define a face normal to y at ymin.
      ,{point2, point3, point7} // These define a face normal to y at ymax.
      ,{point1, point2, point3} // These define a face normal to z at zmin.
      ,{point5, point6, point7}
    }; // These define a face normal to z at zmax.

    // Calculate the normals for each face.
    V3D normals[6];
    for(int i = 0; i < 6; ++i)
    {
      V3D* face = faces[i];
      normals[i] = (face[1] - face[0]).cross_prod((face[2] - face[0]));
      normals[i].normalize();
    }

    // Candidate for parallelisation.
     PARALLEL_FOR2(ws, outputWorkspace)
      for(int i = 0; i < nPeaks; ++i)
      {
      PARALLEL_START_INTERUPT_REGION
      IPeak* peak =  ws->getPeakPtr(i);
      V3D peakCenter = coordFrameFunc(peak);

      bool doesIntersect = true;
      if (peakCenter[0] < extents[0] || peakCenter[0] >= extents[1]
      || peakCenter[1] < extents[2] || peakCenter[1] >= extents[3] 
      || peakCenter[2] < extents[4] || peakCenter[2] >= extents[5]) 
      {
      
        // Out of bounds.
        doesIntersect = false;
        
        if(checkPeakExtents)
        {
          // Take account of radius spherical extents.
          for(int i = 0; i < 6; ++i)
          {
            double distance = normals[i].scalar_prod(peakCenter - faces[i][0]); // Distance between plane and peak center.
            if(peakRadius > std::abs(normals[i].scalar_prod(peakCenter - faces[i][0]))) // Sphere passes through one of the faces, so intersects the box.
            {
              doesIntersect = true;
              break;
            }
          }
        }
      }

      TableRow row = outputWorkspace->getRow(i);
      row << i << doesIntersect;
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION


    setProperty("OutputWorkspace", outputWorkspace);
  }



} // namespace Crystal
} // namespace Mantid
