// The odd line ending on this file is to prevent cxxtest attempting to parse this collection of helper functions as if
// it were a test suite.

#ifndef WORKSPACECREATIONHELPER_H_
#define WORKSPACECREATIONHELPER_H_

#include <cmath>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

class WorkspaceCreationHelper
{
public:

  template<typename T>
  class FibSeries
  {
  private:
    T x1;  /// Initial value 1;
    T x2;  /// Initial value 2;

  public:

    FibSeries() : x1(1),x2(1) {}
    T operator()() { const T out(x1+x2); x1=x2; x2=out;  return out; }
  };

  static Workspace1D_sptr Create1DWorkspaceRand(int size)
  {
    MantidVecPtr x1,y1,e1;
    x1.access().resize(size,1);
    y1.access().resize(size);
    std::generate(y1.access().begin(),y1.access().end(),rand);
    e1.access().resize(size);
    std::generate(e1.access().begin(),e1.access().end(),rand);
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->initialize(1,size,size);
    retVal->setX(x1);
    retVal->setData(y1,e1);
    return retVal;
  }

  static Workspace1D_sptr Create1DWorkspaceFib(int size)
  {
    MantidVecPtr x1,y1,e1;
    x1.access().resize(size,1);
    y1.access().resize(size);
    std::generate(y1.access().begin(),y1.access().end(),FibSeries<double>());
    e1.access().resize(size);
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->initialize(1,size,size);
    retVal->setX(x1);
    retVal->setData(y1,e1);
    return retVal;
  }
  static Workspace2D_sptr Create2DWorkspace(int xlen, int ylen)
  {
    return Create2DWorkspaceBinned(xlen, ylen);
  }

  static Workspace2D_sptr Create2DWorkspace123(int xlen, int ylen,bool isHist=0, const std::set<int> & maskedWorkspaceIndices = std::set<int>())
  {
    MantidVecPtr x1,y1,e1;
    x1.access().resize(isHist?xlen+1:xlen,1);
    y1.access().resize(xlen,2);
    e1.access().resize(xlen,3);
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen,isHist?xlen+1:xlen,xlen);
    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);
      retVal->setData(i,y1,e1);
    }

    retVal = maskSpectra(retVal, maskedWorkspaceIndices);

    return retVal;
  }

  static Workspace2D_sptr Create2DWorkspace154(int xlen, int ylen,bool isHist=0, const std::set<int> & maskedWorkspaceIndices = std::set<int>())
  {
    MantidVecPtr x1,y1,e1;
    x1.access().resize(isHist?xlen+1:xlen,1);
    y1.access().resize(xlen,5);
    e1.access().resize(xlen,4);
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen,isHist?xlen+1:xlen,xlen);
    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);
      retVal->setData(i,y1,e1);
    }

    retVal = maskSpectra(retVal, maskedWorkspaceIndices);

    return retVal;
  }

  static Workspace2D_sptr maskSpectra(Workspace2D_sptr workspace, const std::set<int> & maskedWorkspaceIndices)
  {
    // We need detectors to be able to mask them.
    workspace->setInstrument(boost::shared_ptr<Instrument>(new Instrument));
    boost::shared_ptr<Instrument> instrument = workspace->getBaseInstrument();

    std::string xmlShape = "<sphere id=\"shape\"> ";
    xmlShape +=	"<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" /> " ;
    xmlShape +=	"<radius val=\"0.05\" /> " ;
    xmlShape +=	"</sphere>";
    xmlShape +=	"<algebra val=\"shape\" /> ";  

    ShapeFactory sFactory;
    boost::shared_ptr<Object> shape = sFactory.createShape(xmlShape);

    const int nhist(workspace->getNumberHistograms());

    ParameterMap& pmap = workspace->instrumentParameters();
    for( int i = 0; i < nhist; ++i )
    {
      workspace->getAxis(1)->spectraNo(i) = i;
    }
    workspace->mutableSpectraMap().populateSimple(0, nhist);

    for( int i = 0; i < nhist; ++i )
    {
      Detector *det = new Detector("det",shape, NULL);
      det->setPos(i,i+1,1);
      det->setID(i);
      instrument->markAsDetector(det);     
      if ( maskedWorkspaceIndices.find(i) != maskedWorkspaceIndices.end() )
      {
 	pmap.addBool(det,"masked",true);
      }
    }
    return workspace;
  }

  static Workspace2D_sptr Create2DWorkspaceBinned(int nhist, int nbins, double x0=0.0, double deltax = 1.0)
  {
    MantidVecPtr x,y,e;
    x.access().resize(nbins+1);
    y.access().resize(nbins,2);
    e.access().resize(nbins,sqrt(2.0));
    for (int i =0; i < nbins+1; ++i)
    {
      x.access()[i] = x0+i*deltax;
    }
    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(nhist,nbins+1,nbins);
    for (int i=0; i< nhist; i++)
    {
      retVal->setX(i,x);
      retVal->setData(i,y,e);
    }

    return retVal;
  }

  static WorkspaceSingleValue_sptr CreateWorkspaceSingleValue(double value)
  {
    WorkspaceSingleValue_sptr retVal(new WorkspaceSingleValue(value,sqrt(value)));
    return retVal;
  }

  /** Create event workspace with:
   * 500 pixels
   * 1000 histogrammed bins.
   */
  static EventWorkspace_sptr CreateEventWorkspace()
  {
    return CreateEventWorkspace(500,1001,100,1000);
  }

  /** Create event workspace
   */
  static EventWorkspace_sptr CreateEventWorkspace(int numPixels,
    int numBins, int numEvents = 100, double x0=0.0, double binDelta=1.0,
    int eventPattern = 1, int start_at_pixelID = 0)
  {
    //add one to the number of bins as this is histogram
    numBins++;

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numPixels,1,1);
    
    //Make fake events
    if (eventPattern) // 0 == no events
    {
      for (int pix= start_at_pixelID+0; pix < start_at_pixelID+numPixels; pix++)
      {
        for (int i=0; i<numEvents; i++)
        {
          if (eventPattern == 1) // 0, 1 diagonal pattern
            retVal->getEventListAtPixelID(pix) += TofEvent((pix+i+0.5)*binDelta, 1); 
          else if (eventPattern == 2) // solid 2
          {
            retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, 1); 
            retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, 1); 
          }
          else if (eventPattern == 3) // solid 1
          {
            retVal->getEventListAtPixelID(pix) += TofEvent((i+0.5)*binDelta, 1);
          }
        }
      }
    }
    retVal->doneLoadingData();

   //Create the x-axis for histogramming.
    MantidVecPtr x1;
    MantidVec& xRef = x1.access();
    xRef.resize(numBins);
    for (int i = 0; i < numBins; ++i)
    {
      xRef[i] = x0+i*binDelta;
    }

    //Set all the histograms at once.
    retVal->setAllX(x1);

    return retVal;
  }


  /** Create event workspace
   */
  static EventWorkspace_sptr CreateGroupedEventWorkspace(std::vector< std::vector<int> > groups,
      int numBins, double binDelta=1.0)
  {

    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(1,2,1);

    for (int g=0; g < groups.size(); g++)
    {
      std::vector<int> dets = groups[g];
      for (std::vector<int>::iterator it = dets.begin(); it != dets.end(); it++)
      {
        for (int i=0; i < numBins; i++)
          retVal->getOrAddEventList(g) += TofEvent((i+0.5)*binDelta, 1);
        retVal->getOrAddEventList(g).addDetectorID( *it );
      }
    }

    retVal->doneAddingEventLists();

   //Create the x-axis for histogramming.
    MantidVecPtr x1;
    MantidVec& xRef = x1.access();
    double x0=0;
    xRef.resize(numBins);
    for (int i = 0; i < numBins; ++i)
    {
      xRef[i] = x0+i*binDelta;
    }

    //Set all the histograms at once.
    retVal->setAllX(x1);

    return retVal;
  }




  //not strictly creating a workspace, but really helpfull to see what one contains
  static void DisplayDataY(const MatrixWorkspace_sptr ws)
  {
      const int numHists = ws->getNumberHistograms();
      for (int i = 0; i < numHists; ++i)
      {
        std::cout << "Histogram " << i << " = ";
        for (int j = 0; j < ws->blocksize(); ++j)
        {  
          std::cout <<ws->readY(i)[j]<<" ";
        }
        std::cout<<std::endl;
      }
  }
  static void DisplayData(const MatrixWorkspace_sptr ws)
  {
    DisplayDataX(ws);
  }
  //not strictly creating a workspace, but really helpfull to see what one contains
  static void DisplayDataX(const MatrixWorkspace_sptr ws)
  {
      const int numHists = ws->getNumberHistograms();
      for (int i = 0; i < numHists; ++i)
      {
        std::cout << "Histogram " << i << " = ";
        for (int j = 0; j < ws->blocksize(); ++j)
        {  
          std::cout <<ws->readX(i)[j]<<" ";
        }
        std::cout<<std::endl;
      }
  }
  //not strictly creating a workspace, but really helpfull to see what one contains
  static void DisplayDataE(const MatrixWorkspace_sptr ws)
  {
      const int numHists = ws->getNumberHistograms();
      for (int i = 0; i < numHists; ++i)
      {
        std::cout << "Histogram " << i << " = ";
        for (int j = 0; j < ws->blocksize(); ++j)
        {  
          std::cout <<ws->readE(i)[j]<<" ";
        }
        std::cout<<std::endl;
      }
  }

};

#endif /*WORKSPACECREATIONHELPER_H_*/
