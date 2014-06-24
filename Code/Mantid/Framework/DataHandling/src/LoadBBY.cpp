#include "MantidDataHandling/LoadBBY.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "MantidNexus/NexusClasses.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <Poco/File.h>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace DataHandling
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_FILELOADER_ALGORITHM(LoadBBY);
    
    // Pointer to the vector of events
    typedef std::vector<DataObjects::TofEvent> * EventVector_pt;
    
    // read counts/events from binary file
    void LoadFile_Counts(const std::string &path, size_t width, size_t height, double tofMinBoundary, double tofMaxBoundary, std::vector<size_t> &counts) {
      std::ifstream fpi;
      fpi.open(path.c_str(), std::ios::in | std::ios::binary);

      unsigned int x = 0;   // 9 bits [0-239] tube number
      unsigned int y = 0;   // 8 bits [0-255] position along tube
      //uint v = 0; // 0 bits [     ]
      //uint w = 0; // 0 bits [     ] energy
      unsigned int dt = 0;
    
      int state = 0;
      fpi.seekg(128);
    
      char c0;
      while (fpi.get(c0).good()) {
        unsigned int c = c0;

        bool event_ended = false;
        switch (state) {
          case 0:
            x  = (c & 0xFF) << 0;   // set bit 1-8
            break;

          case 1:
            x |= (c & 0x01) << 8;   // set bit 9
            y  = (c & 0xFE) >> 1;   // set bit 1-7
            break;

          case 2:
            event_ended = (c & 0xC0) != 0xC0;
            if (!event_ended)
              c &= 0x3F;

            y |= (c & 0x01) << 7;   // set bit 8
            dt = (c & 0xFE) >> 1;   // set bit 1-5(7)
            break;

          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
            event_ended = (c & 0xC0) != 0xC0;
            if (!event_ended)
              c &= 0x3F;

            dt |= (c & 0xFF) << (5 + 6 * (state - 3)); // set bit 6...
            break;
        }
        state++;

        if (event_ended || (state == 8)) {
          state = 0;

          if ((x == 0) && (y == 0) && (dt == 0xFFFFFFFF)) {

          }
          else if ((x < width) && (y < height)) {
            // check range
            if ((dt >= tofMinBoundary) && (dt <= tofMaxBoundary))
              counts[height * (x) + y]++;
          }
        }
      }
    }
    void LoadFile_Events(const std::string &path, size_t width, size_t height, double tofMinBoundary, double tofMaxBoundary, std::vector<EventVector_pt> &eventVectors, double &shortest_tof, double &longest_tof) {
      double tofMin = std::numeric_limits<double>::max();
      double tofMax = std::numeric_limits<double>::min();

      std::ifstream fpi;
      fpi.open(path.c_str(), std::ios::in | std::ios::binary);

      unsigned int x = 0;   // 9 bits [0-239] tube number
      unsigned int y = 0;   // 8 bits [0-255] position along tube
      //uint v = 0; // 0 bits [     ]
      //uint w = 0; // 0 bits [     ] energy
      unsigned int dt = 0;
    
      int state = 0;
      fpi.seekg(128);
    
      char c0;
      while (fpi.get(c0).good()) {
        unsigned int c = c0;

        bool event_ended = false;
        switch (state) {
          case 0:
            x  = (c & 0xFF) << 0;   // set bit 1-8
            break;

          case 1:
            x |= (c & 0x01) << 8;   // set bit 9
            y  = (c & 0xFE) >> 1;   // set bit 1-7
            break;

          case 2:
            event_ended = (c & 0xC0) != 0xC0;
            if (!event_ended)
              c &= 0x3F;

            y |= (c & 0x01) << 7;   // set bit 8
            dt = (c & 0xFE) >> 1;   // set bit 1-5(7)
            break;

          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
            event_ended = (c & 0xC0) != 0xC0;
            if (!event_ended)
              c &= 0x3F;

            dt |= (c & 0xFF) << (5 + 6 * (state - 3)); // set bit 6...
            break;
        }
        state++;

        if (event_ended || (state == 8)) {
          state = 0;

          if ((x == 0) && (y == 0) && (dt == 0xFFFFFFFF)) {

          }
          else if ((x < width) && (y < height)) {
            // check range
            if ((dt >= tofMinBoundary) && (dt <= tofMaxBoundary)) {
              double tof = (double)dt;
#if !(defined(__INTEL_COMPILER)) && !(defined(__clang__))
              // This avoids a copy constructor call but is only available with GCC (requires variadic templates)
              eventVectors[height * (x) + y]->emplace_back(tof);
#else
              eventVectors[height * (x) + y]->push_back(tof);
#endif              
              if (tofMin > tof)
                tofMin = tof;
              if (tofMax < tof)
                tofMax = tof;
            }
          }
        }
      }

      if (tofMin > tofMax) {
        shortest_tof = 0.0;
        longest_tof  = 0.0;
      }
      else {
        shortest_tof = tofMin;
        longest_tof  = tofMax;
      }
    }

    // for geometry
    void AddDetectorBank(Geometry::Instrument_sptr instrument, size_t xPixelCount, size_t yPixelCount, size_t startIndex, Geometry::Object_sptr shape, double width, double height, const Kernel::V3D &pos, const Kernel::Quat &rot) {
      
      // Create a RectangularDetector which represents a rectangular array of pixels
      Geometry::RectangularDetector* bank = new Geometry::RectangularDetector("bank", instrument.get()); // possible memory leak!? "new" without "delete"

      // Initialise the detector specifying the sizes.
      double pixelWidth  = width  / static_cast<double>(xPixelCount);
      double pixelHeight = height / static_cast<double>(yPixelCount);

      bank->initialize(
        shape,
        // x
        (int)xPixelCount,
        0,
        pixelWidth,
        // y
        (int)yPixelCount,
        0,
        pixelHeight,
        // indices
        (int)startIndex,
        true,
        (int)yPixelCount);
      
      for (int x = 0; x < static_cast<int>(xPixelCount); ++x)
        for (int y = 0; y < static_cast<int>(yPixelCount); ++y)
          instrument->markAsDetector(bank->getAtXY(x, y).get());

      // Position the detector so the z axis goes through its centre
      Kernel::V3D center(width / 2, height / 2, 0);
      rot.rotate(center);

      bank->rotate(rot);
      bank->translate(pos - center);
    }

    /**
     * Return the confidence with with this algorithm can load the file
     * @param descriptor A descriptor for the file
     * @returns An integer specifying the confidence level. 0 indicates it will not be used
     */
    int LoadBBY::confidence(Kernel::FileDescriptor & descriptor) const
    {
      return descriptor.extension() == ".bin" ? 50 : 0; // just for testing
    }


    /**
     * Initialise the algorithm. Declare properties which can be set before execution (input) or 
     * read from after the execution (output).
     */
    void LoadBBY::init()
    {
      // Specify file extensions which can be associated with a BBY file.
      std::vector<std::string> exts;

      // Declare the Filename algorithm property. Mandatory. Sets the path to the file to load.
      exts.clear();
      exts.push_back(".bin");
      declareProperty(
        new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
        "The input filename of the stored data");

      exts.clear();
      exts.push_back(".hdf");
      declareProperty(
        new API::FileProperty("HdfReference", "", API::FileProperty::OptionalLoad, exts),
        "The input filename of the stored data");
      
      declareProperty(
        new API::WorkspaceProperty<API::IEventWorkspace>("OutputWorkspace", "", Kernel::Direction::Output));
      
      declareProperty(
        new PropertyWithValue<double>("FilterByTofMin", 0, Direction::Input),
        "Optional: To exclude events that do not fall within a range of times-of-flight. "\
        "This is the minimum accepted value in microseconds. Keep blank to load all events.");
      declareProperty(
        new PropertyWithValue<double>("FilterByTofMax", 50000000, Direction::Input),
        "Optional: To exclude events that do not fall within a range of times-of-flight. "\
        "This is the maximum accepted value in microseconds. Keep blank to load all events." );
      declareProperty(
        new PropertyWithValue<double>("FilterByTimeStart", EMPTY_DBL(), Direction::Input),
        "Optional: To only include events after the provided start time, in seconds (relative to the start of the run).");
      declareProperty(
        new PropertyWithValue<double>("FilterByTimeStop", EMPTY_DBL(), Direction::Input),
        "Optional: To only include events before the provided stop time, in seconds (relative to the start of the run).");

      std::string grp1 = "Optional";
      setPropertyGroup("FilterByTofMin", grp1);
      setPropertyGroup("FilterByTofMax", grp1);
      setPropertyGroup("FilterByTimeStart", grp1);
      setPropertyGroup("FilterByTimeStop", grp1);
    }

    /**
     * Execute the algorithm.
     */
    void LoadBBY::exec()
    {
      const size_t HISTO_BINS_X = 240; // 240;
      const size_t HISTO_BINS_Y = 256; // 256;

      size_t nHist = HISTO_BINS_Y * HISTO_BINS_X; // number of spectra/pixels in the dataset

      // Delete the output workspace name if it existed
      std::string outName = getPropertyValue("OutputWorkspace");
      if (AnalysisDataService::Instance().doesExist(outName))
        AnalysisDataService::Instance().remove(outName);

      // Get the name of the file.
      std::string filenameBin = getPropertyValue("Filename");
      std::string filenameHdf = getPropertyValue("HdfReference");
      
      size_t nBins  = 1;
      double tofMinBoundary = getProperty("FilterByTofMin");
      double tofMaxBoundary = getProperty("FilterByTofMax");

      // Create a workspace
      DataObjects::EventWorkspace_sptr eventWS(new DataObjects::EventWorkspace());

      eventWS->initialize(
          nHist,
          nBins + 1,  // number of TOF bin boundaries
          nBins);

      // Set the units
      eventWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      eventWS->setYUnit("Counts");
      eventWS->setYUnitLabel("Counts"); // ???
      
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

      //double cdd = 300;
      if (!filenameHdf.empty()) {
        NeXus::NXRoot root(filenameHdf);
        NeXus::NXEntry entry = root.openFirstEntry();

        //cdd = static_cast<double>(entry.getFloat("instrument/detector/cdd"));
        
        //char buffer[256];
        //sprintf_s(buffer, "cdd = %f", cdd);
        //MessageBoxA(nullptr, buffer, "Parameters", MB_OK | MB_ICONINFORMATION);
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

      // count total events per pixel to reserve necessary memory
      LoadFile_Counts(filenameBin, HISTO_BINS_X, HISTO_BINS_Y, tofMinBoundary, tofMaxBoundary, eventCounts);

      for (size_t i = 0; i != numberHistograms; ++i) {
        DataObjects::EventList& eventList = eventWS->getEventList(i);
        eventList.setSortOrder(DataObjects::PULSETIME_SORT); // why not PULSETIME[TOF]_SORT ?
        eventList.reserve(eventCounts[i]);
        DataObjects::getEventsFrom(eventList, eventVectors[i]);
      }
      
      double shortest_tof(0.0), longest_tof(0.0);
      LoadFile_Events(filenameBin, HISTO_BINS_X, HISTO_BINS_Y, tofMinBoundary, tofMaxBoundary, eventVectors, shortest_tof, longest_tof);

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

  }//namespace
}//namespace
