#include "MantidDataHandling/LoadFITS.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
//#include "MantidAPI/WorkspaceValidators.h"
//#include "MantidKernel/UnitFactory.h"
//#include "MantidGeometry/Instrument.h"
//#include "MantidGeometry/Instrument/RectangularDetector.h"
//#include "MantidGeometry/Objects/ShapeFactory.h"
//
//#include "MantidNexus/NexusClasses.h"
//
//#include <boost/math/special_functions/fpclassify.hpp>
//#include <Poco/File.h>
//#include <iostream>
//#include <fstream>
//#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace DataHandling
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_FILELOADER_ALGORITHM(LoadFITS);

  int LoadFITS::confidence(Kernel::FileDescriptor & descriptor) const
  {
      return (descriptor.extension() == ".fits" || descriptor.extension() == ".fit") ? 80 : 0; 
  }

  /**
  * Execute the algorithm.
  */
  void LoadFITS::exec()
  {
    // Delete the output workspace name if it existed
    std::string outName = getPropertyValue("OutputWorkspace");
    if (AnalysisDataService::Instance().doesExist(outName))
      AnalysisDataService::Instance().remove(outName);

    // Get the name of the file.
    std::string filenameBin = getPropertyValue("Filename");
      
    size_t nBins  = 1;
    double tofMinBoundary = 5;
    double tofMaxBoundary = 5;
      
    //// 100 for "loading neutron counts", 100 for "creating neutron event lists", 100 for "loading neutron events"
    //Progress prog(this, 0.0, 1.0, 100 + 100 + 100);
    //prog.doReport("creating instrument");

    // Create a workspace
    DataObjects::EventWorkspace_sptr eventWS(new DataObjects::EventWorkspace());

    eventWS->initialize(
        nHist,
        nBins + 1,  // number of TOF bin boundaries
        nBins);

    // Set the units
    eventWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    eventWS->setYUnit("Counts");

    // ???
    //eventWS->setYUnitLabel("Counts");      

    eventWS->setTitle("my title");
    eventWS->mutableRun().addProperty("Filename", filenameBin);
    //eventWS->mutableRun().addProperty("run_number", 1);
    //eventWS->mutableRun().addProperty("run_start", "1991-01-01T00:00:00", true );
    //eventWS->mutableRun().addProperty("duration", duration[0], units);

    // Build instrument geometry

    // Create a new instrument and set its name
    std::string instrumentname = "BILBY";
    Geometry::Instrument_sptr instrument(new Geometry::Instrument(instrumentname));
    eventWS->setInstrument(instrument);

    // Add dummy source and samplepos to instrument

    // Create an instrument component wich will represent the sample position.
    Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample",instrument.get());
    instrument->add(samplepos);
    instrument->markAsSamplePos(samplepos);
    // Put the sample in the centre of the coordinate system
    samplepos->setPos(0.0,0.0,0.0);

    // Create a component to represent the source
    Geometry::ObjComponent *source = new Geometry::ObjComponent("Source",instrument.get());
    instrument->add(source);
    instrument->markAsSource(source);

    // Read in the L1 value and place the source at (0,0,-L1)
    double l1 = 3949.1824;
    source->setPos(0.0,0.0,-1.0*l1);

    // Create a component for the detector.
      
    size_t xPixelCount = HISTO_BINS_X / 6;
    size_t yPixelCount = HISTO_BINS_Y;
    size_t pixelCount = xPixelCount * yPixelCount;
      
    // We assumed that these are the dimensions of the detector, and height is in y direction and width is in x direction
    double width  = 0.4; // meters
    double height = 1.0;

    // We assumed that individual pixels have the same size and shape of a cuboid with dimensions:
    double pixel_width  = width  / static_cast<double>(xPixelCount);
    double pixel_height = height / static_cast<double>(yPixelCount);

    // Create size strings for shape creation
    std::string pixel_width_str  = boost::lexical_cast<std::string>(pixel_width / 2);
    std::string pixel_height_str = boost::lexical_cast<std::string>(pixel_height / 2);
    std::string pixel_depth_str  = "0.00001"; // Set the depth of a pixel to a very small number

    // Define shape of a pixel as an XML string. See http://www.mantidproject.org/HowToDefineGeometricShape for details
    // on shapes in Mantid.
    std::string detXML =
      "<cuboid id=\"pixel\">"
        "<left-front-bottom-point   x=\"+"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\"0\"  />"
        "<left-front-top-point      x=\"+"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\""+pixel_depth_str+"\"  />"
        "<left-back-bottom-point    x=\"-"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\"0\"  />"
        "<right-front-bottom-point  x=\"+"+pixel_width_str+"\" y=\"+"+pixel_height_str+"\" z=\"0\"  />"
      "</cuboid>";

    // Create a shape object which will be shared by all pixels.
    Geometry::Object_sptr shape = Geometry::ShapeFactory().createShape(detXML);
      
    double detectorZ =  5;
    double angle     = 10;

    double curtZOffset = width / 2 * sin(angle * 3.14159265359 / 180);

    if (!filenameHdf.empty()) {
      NeXus::NXRoot root(filenameHdf);
      NeXus::NXEntry entry = root.openFirstEntry();
     
    }

    // 6 detector banks are available

    // curtain 1
    AddDetectorBank(
      instrument,
      xPixelCount,
      yPixelCount,
      0 * pixelCount,
      shape,
      width,
      height,
      Kernel::V3D(+(width + height) / 2, 0, detectorZ - curtZOffset),
      Kernel::Quat(  0, Kernel::V3D(0, 0, 1)) * Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));
      
    // curtain 2
    AddDetectorBank(
      instrument,
      xPixelCount,
      yPixelCount,
      1 * pixelCount,
      shape,
      width,
      height,
      Kernel::V3D(-(width + height) / 2, 0, detectorZ - curtZOffset),
      Kernel::Quat(180, Kernel::V3D(0, 0, 1)) * Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));
      
    // curtain 3
    AddDetectorBank(
      instrument,
      xPixelCount,
      yPixelCount,
      2 * pixelCount,
      shape,
      width,
      height,
      Kernel::V3D(0, +(width + height) / 2, detectorZ - curtZOffset),
      Kernel::Quat( 90, Kernel::V3D(0, 0, 1)) * Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));
      
    // curtain 4
    AddDetectorBank(
      instrument,
      xPixelCount,
      yPixelCount,
      3 * pixelCount,
      shape,
      width,
      height,
      Kernel::V3D(0, -(width + height) / 2, detectorZ - curtZOffset),
      Kernel::Quat(-90, Kernel::V3D(0, 0, 1)) * Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));
      
    // back 1
    AddDetectorBank(
      instrument,
      xPixelCount,
      yPixelCount,
      4 * pixelCount,
      shape,
      width,
      height,
      Kernel::V3D(-width / 2 - 0.05, 0, detectorZ),
      Kernel::Quat(180, Kernel::V3D(0, 0, 1)));
      
    // back 2
    AddDetectorBank(
      instrument,
      xPixelCount,
      yPixelCount,
      5 * pixelCount,
      shape,
      width,
      height,
      Kernel::V3D(+width / 2 + 0.05, 0, detectorZ),
      Kernel::Quat(  0, Kernel::V3D(0, 0, 1)));

    // load events
                  
    size_t numberHistograms = eventWS->getNumberHistograms();

    std::vector<EventVector_pt> eventVectors(numberHistograms, NULL);
    std::vector<size_t> eventCounts(numberHistograms, 0);
		std::vector<detid_t> detIDs = instrument->getDetectorIDs();

    // count total events per pixel to reserve necessary memory
    LoadFile_Counts(prog, filenameBin, HISTO_BINS_X, HISTO_BINS_Y, tofMinBoundary, tofMaxBoundary, eventCounts);
      
    // for progress notifications
    size_t progCount = 100;
    size_t progStep  = numberHistograms / progCount;
    size_t progNext  = progStep;
      
    auto progMsg = "creating neutron event lists";
    prog.doReport(progMsg);
    for (size_t i = 0; i != numberHistograms; ++i) {
      DataObjects::EventList& eventList = eventWS->getEventList(i);

      eventList.setSortOrder(DataObjects::PULSETIME_SORT); // why not PULSETIME[TOF]_SORT ?
      eventList.reserve(eventCounts[i]);
      eventList.setDetectorID(detIDs[i]);
			eventList.setSpectrumNo(detIDs[i]);  

      DataObjects::getEventsFrom(eventList, eventVectors[i]);

      if ((progNext <= i) && (progCount != 0)) {
        prog.report(progMsg);
        progNext += progStep;
        progCount--;
      }
    }
    if (progCount != 0)
      prog.reportIncrement(progCount, progMsg);
      
    double shortest_tof(0.0), longest_tof(0.0);
    LoadFile_Events(prog, filenameBin, HISTO_BINS_X, HISTO_BINS_Y, tofMinBoundary, tofMaxBoundary, eventVectors, shortest_tof, longest_tof);

    cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(2, 0.0);
    if (longest_tof != 0.0) {
      xRef[0] = std::max(0.0, shortest_tof - 1); // just to make sure the bins hold it all
      xRef[1] = longest_tof + 1;
    }
    eventWS->setAllX(axis);

    setProperty("OutputWorkspace", eventWS);
  }

  /**
  * Initialise the algorithm. Declare properties which can be set before execution (input) or 
  * read from after the execution (output).
  */
  void LoadFITS::init()
  {
    // Specify file extensions which can be associated with a BBY file.
    std::vector<std::string> exts;

    // Declare the Filename algorithm property. Mandatory. Sets the path to the file to load.
    exts.clear();
    exts.push_back(".fit");  
    declareProperty(
    new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
    "The input filename of the stored data");
      
    declareProperty(
    new API::WorkspaceProperty<API::IEventWorkspace>("OutputWorkspace", "", Kernel::Direction::Output));
  }


}
}