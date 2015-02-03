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

#include <Poco/TemporaryFile.h>

namespace Mantid {
namespace DataHandling {
// register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadBBY);

// consts
static const size_t HISTO_BINS_X = 240;
static const size_t HISTO_BINS_Y = 256;
// 100 = 40 + 20 + 40
static const size_t Progress_LoadBinFile = 40;
static const size_t Progress_ReserveMemory = 20;

using ANSTO::EventVector_pt;

class BbyDetectorBankFactory {
private:
  // fields
  const Geometry::Instrument_sptr m_instrument;
  const Geometry::Object_sptr m_pixelShape;
  const size_t m_xPixelCount;
  const size_t m_yPixelCount;
  const double m_pixelWidth;
  const double m_pixelHeight;
  const Kernel::V3D m_center;

public:
  // construction
  BbyDetectorBankFactory(Geometry::Instrument_sptr instrument,
                         Geometry::Object_sptr pixelShape, size_t xPixelCount,
                         size_t yPixelCount, double pixelWidth,
                         double pixelHeight, const Kernel::V3D &center);

  // methods
  void createAndAssign(size_t startIndex, const Kernel::V3D &pos,
                       const Kernel::Quat &rot) const;
};

/**
 * Return the confidence value that this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadBBY::confidence(Kernel::FileDescriptor &descriptor) const {
  if (descriptor.extension() != ".tar")
    return 0;

  ANSTO::Tar::File file(descriptor.filename());
  if (!file.good())
    return 0;

  size_t hdfFiles = 0;
  size_t binFiles = 0;
  const std::vector<std::string> &subFiles = file.files();
  for (auto itr = subFiles.begin(); itr != subFiles.end(); ++itr) {
    auto len = itr->length();
    if ((len > 4) && (itr->find_first_of("\\/", 0, 2) == std::string::npos)) {
      if ((itr->rfind(".hdf") == len - 4) && (itr->compare(0, 3, "BBY") == 0))
        hdfFiles++;
      else if (itr->rfind(".bin") == len - 4)
        binFiles++;
    }
  }

  return (hdfFiles == 1) && (binFiles == 1) ? 50 : 0;
}
/**
 * Initialise the algorithm. Declare properties which can be set before
 * execution (input) or
 * read from after the execution (output).
 */
void LoadBBY::init() {
  // Specify file extensions which can be associated with a BBY file.
  std::vector<std::string> exts;

  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  exts.clear();
  exts.push_back(".tar");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
      "The input filename of the stored data");

  // mask
  exts.clear();
  exts.push_back(".xml");
  declareProperty(
      new API::FileProperty("Mask", "", API::FileProperty::OptionalLoad, exts),
      "The input filename of the mask data");

  // offsets
  exts.clear();
  exts.push_back(".csv");
  declareProperty(
    new API::FileProperty("TubeOffsets", "", API::FileProperty::OptionalLoad, exts),
    "The input filename of the tube offset data");
    
  declareProperty(new API::WorkspaceProperty<API::IEventWorkspace>(
      "OutputWorkspace", "", Kernel::Direction::Output));

  declareProperty(new Kernel::PropertyWithValue<double>(
                      "FilterByTofMin", 0, Kernel::Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the minimum accepted value in microseconds. Keep "
                  "blank to load all events.");
  declareProperty(new Kernel::PropertyWithValue<double>(
                      "FilterByTofMax", 50000000, Kernel::Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the maximum accepted value in microseconds. Keep "
                  "blank to load all events.");
  declareProperty(
      new Kernel::PropertyWithValue<double>("FilterByTimeStart", EMPTY_DBL(),
                                            Kernel::Direction::Input),
      "Optional: To only include events after the provided start time, in "
      "seconds (relative to the start of the run).");
  declareProperty(
      new Kernel::PropertyWithValue<double>("FilterByTimeStop", EMPTY_DBL(),
                                            Kernel::Direction::Input),
      "Optional: To only include events before the provided stop time, in "
      "seconds (relative to the start of the run).");

  std::string grpOptional = "Optional";
  setPropertyGroup("FilterByTofMin", grpOptional);
  setPropertyGroup("FilterByTofMax", grpOptional);
  setPropertyGroup("FilterByTimeStart", grpOptional);
  setPropertyGroup("FilterByTimeStop", grpOptional);
}
/**
 * Execute the algorithm.
 */
void LoadBBY::exec() {

  // Delete the output workspace name if it existed
  std::string outName = getPropertyValue("OutputWorkspace");
  if (API::AnalysisDataService::Instance().doesExist(outName))
    API::AnalysisDataService::Instance().remove(outName);

  // Get the name of the data file.
  std::string filename = getPropertyValue("Filename");
  ANSTO::Tar::File file(filename);
  if (!file.good())
    return;

  // load mask file
  bool maskFileLoaded = false;
  std::vector<bool> mask =
      createMaskVector(getPropertyValue("Mask"), maskFileLoaded);
  
  // load tube offsets
  bool offsetFileLoaded = false;
  std::vector<int> offsets = createOffsetVector(getPropertyValue("TubeOffsets"),
                                                offsetFileLoaded);
  for (size_t x = 0; x != HISTO_BINS_X; x++) {
    int offset = offsets[x];
    if (offset != 0) {
      maskFileLoaded = true;
          
      size_t s0 = HISTO_BINS_Y * x;
      if (offset > 0) {
        for (int y = 0; y != offset; y++)
          mask[s0 + (size_t)y] = false;
      }
      else { // if (offset < 0)
        for (size_t y = HISTO_BINS_Y + offset; y != HISTO_BINS_Y; y++)
          mask[s0 + y] = false;
      }
    }
  }

  size_t nBins = 1;
  double tofMinBoundary = getProperty("FilterByTofMin");
  double tofMaxBoundary = getProperty("FilterByTofMax");

  // "loading neutron counts", "creating neutron event lists" and "loading
  // neutron events"
  API::Progress prog(this, 0.0, 1.0, Progress_LoadBinFile +
                                         Progress_ReserveMemory +
                                         Progress_LoadBinFile);
  prog.doReport("creating instrument");

  // create workspace
  DataObjects::EventWorkspace_sptr eventWS =
      boost::make_shared<DataObjects::EventWorkspace>();

  eventWS->initialize(HISTO_BINS_Y * HISTO_BINS_X,
                      nBins + 1, // number of TOF bin boundaries
                      nBins);

  // set the units
  eventWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  eventWS->setYUnit("Counts");

  // set title
  const std::vector<std::string> &subFiles = file.files();
  for (auto itr = subFiles.begin(); itr != subFiles.end(); ++itr)
    if (itr->compare(0, 3, "BBY") == 0) {
      std::string title = *itr;

      if (title.rfind(".hdf") == title.length() - 4)
        title.resize(title.length() - 4);

      if (title.rfind(".nx") == title.length() - 3)
        title.resize(title.length() - 3);

      eventWS->setTitle(title);
      break;
    }

  // set auxiliaries
  eventWS->mutableRun().addProperty("Filename", filename);
  // eventWS->mutableRun().addProperty("run_number", 1);
  // eventWS->mutableRun().addProperty("run_start", "1991-01-01T00:00:00", true
  // );
  // eventWS->mutableRun().addProperty("duration", duration[0], units);

  // create instrument
  Geometry::Instrument_sptr instrument = createInstrument(file);
  eventWS->setInstrument(instrument);

  // load events

  size_t numberHistograms = eventWS->getNumberHistograms();

  std::vector<EventVector_pt> eventVectors(numberHistograms, NULL);
  std::vector<size_t> eventCounts(numberHistograms, 0);
  std::vector<detid_t> detIDs = instrument->getDetectorIDs();

  // count total events per pixel to reserve necessary memory
  ANSTO::EventCounter eventCounter(eventCounts, mask, offsets, HISTO_BINS_Y);
  loadEvents(prog, "loading neutron counts", file, tofMinBoundary,
             tofMaxBoundary, eventCounter);

  // prepare event storage
  ANSTO::ProgressTracker progTracker(prog, "creating neutron event lists",
                                     numberHistograms, Progress_ReserveMemory);
  for (size_t i = 0; i != numberHistograms; ++i) {
    DataObjects::EventList &eventList = eventWS->getEventList(i);

    eventList.setSortOrder(
        DataObjects::PULSETIME_SORT); // why not PULSETIME[TOF]_SORT ?
    eventList.reserve(eventCounts[i]);

    detid_t id = detIDs[i];
    eventList.setDetectorID(id);
    eventList.setSpectrumNo(id);

    DataObjects::getEventsFrom(eventList, eventVectors[i]);

    progTracker.update(i);
  }
  progTracker.complete();

  ANSTO::EventAssigner eventAssigner(eventVectors, mask, offsets,
                                     HISTO_BINS_Y);
  loadEvents(prog, "loading neutron events", file, tofMinBoundary,
             tofMaxBoundary, eventAssigner);

  Kernel::cow_ptr<MantidVec> axis;
  MantidVec &xRef = axis.access();
  xRef.resize(2, 0.0);
  xRef[0] = std::max(0.0, eventCounter.tofMin() - 1); // just to make sure the bins hold it all
  xRef[1] = eventCounter.tofMax() + 1;
  eventWS->setAllX(axis);

  if (maskFileLoaded) {
    // count total number of masked pixels
    size_t maskedPixels = 0;
    for (auto itr = mask.begin(); itr != mask.end(); ++itr)
      if (!*itr)
        maskedPixels++;

    // create list of masked pixels
    std::vector<size_t> maskIndexList(maskedPixels);
    size_t pixelIndex = 0;
    size_t maskedPixelIndex = 0;
    for (auto itr = mask.begin(); itr != mask.end(); ++itr, ++pixelIndex)
      if (!*itr)
        maskIndexList[maskedPixelIndex++] = pixelIndex;

    API::IAlgorithm_sptr maskingAlg = createChildAlgorithm("MaskDetectors");
    maskingAlg->setProperty("Workspace", eventWS);
    maskingAlg->setProperty("WorkspaceIndexList", maskIndexList);
    maskingAlg->executeAsChildAlg();
  }

  setProperty("OutputWorkspace", eventWS);
}

// instrument creation
Geometry::Instrument_sptr LoadBBY::createInstrument(ANSTO::Tar::File &tarFile) {
  // instrument
  Geometry::Instrument_sptr instrument =
      boost::make_shared<Geometry::Instrument>("BILBY");
  instrument->setDefaultViewAxis("Z-");

  // source
  Geometry::ObjComponent *source =
      new Geometry::ObjComponent("Source", instrument.get());
  instrument->add(source);
  instrument->markAsSource(source);

  //// chopper
  // Geometry::ObjComponent *chopperPoint = new
  // Geometry::ObjComponent("Chopper", instrument.get());
  // instrument->add(chopper);
  // instrument->markAsChopperPoint(chopper);

  // sample
  Geometry::ObjComponent *samplePos =
      new Geometry::ObjComponent("Sample", instrument.get());
  instrument->add(samplePos);
  instrument->markAsSamplePos(samplePos);

  double L2_det_value = 33.15616015625;
  double L1_chopper_value = 18.47258984375;
  // double L1_source_value    =  9.35958984375;

  double L2_curtainl_value = 23.28446093750;
  double L2_curtainr_value = 23.28201953125;
  double L2_curtainu_value = 24.28616015625;
  double L2_curtaind_value = 24.28235937500;

  double D_det_value = (8.4 + 2.0) / (2 * 1000);

  double D_curtainl_value = 0.3816;
  double D_curtainr_value = 0.4024;
  double D_curtainu_value = 0.3947;
  double D_curtaind_value = 0.3978;

  // extract hdf file
  int64_t fileSize = 0;
  const std::vector<std::string> &files = tarFile.files();
  for (auto itr = files.begin(); itr != files.end(); ++itr)
    if (itr->rfind(".hdf") == itr->length() - 4) {
      tarFile.select(itr->c_str());
      fileSize = tarFile.selected_size();
      break;
    }

  if (fileSize != 0) {
    // create tmp file
    Poco::TemporaryFile hdfFile;
    boost::shared_ptr<FILE> handle(fopen(hdfFile.path().c_str(), "wb"), fclose);
    if (handle) {
      // copy content
      char buffer[4096];
      size_t bytesRead;
      while (0 != (bytesRead = tarFile.read(buffer, sizeof(buffer))))
        fwrite(buffer, bytesRead, 1, handle.get());
      handle.reset();

      NeXus::NXRoot root(hdfFile.path());
      NeXus::NXEntry entry = root.openFirstEntry();

      float tmp;
      const double toMeters = 1.0 / 1000;

      if (loadNXDataSet(tmp, entry, "instrument/L2_det"))
        L2_det_value = tmp * toMeters;
      if (loadNXDataSet(tmp, entry, "instrument/Ltof_det"))
        L1_chopper_value = tmp * toMeters - L2_det_value;
      // if (loadNXDataSet(tmp, entry, "instrument/L1"))
      //  L1_source_value = tmp * toMeters;

      if (loadNXDataSet(tmp, entry, "instrument/L2_curtainl"))
        L2_curtainl_value = tmp * toMeters;
      if (loadNXDataSet(tmp, entry, "instrument/L2_curtainr"))
        L2_curtainr_value = tmp * toMeters;
      if (loadNXDataSet(tmp, entry, "instrument/L2_curtainu"))
        L2_curtainu_value = tmp * toMeters;
      if (loadNXDataSet(tmp, entry, "instrument/L2_curtaind"))
        L2_curtaind_value = tmp * toMeters;

      if (loadNXDataSet(tmp, entry, "instrument/detector/curtainl"))
        D_curtainl_value = tmp * toMeters;
      if (loadNXDataSet(tmp, entry, "instrument/detector/curtainr"))
        D_curtainr_value = tmp * toMeters;
      if (loadNXDataSet(tmp, entry, "instrument/detector/curtainu"))
        D_curtainu_value = tmp * toMeters;
      if (loadNXDataSet(tmp, entry, "instrument/detector/curtaind"))
        D_curtaind_value = tmp * toMeters;
    }
  }

  source->setPos(0.0, 0.0, -L1_chopper_value);
  samplePos->setPos(0.0, 0.0, 0.0);

  // create a component for the detector

  size_t xPixelCount = HISTO_BINS_X / 6;
  size_t yPixelCount = HISTO_BINS_Y;
  size_t pixelCount = xPixelCount * yPixelCount;

  // dimensions of the detector (height is in y direction, width is in x
  // direction)
  double width = 336.0 / 1000;  // meters
  double height = 640.0 / 1000; // meters
  double angle = 10.0;          // degree

  // we assumed that individual pixels have the same size and shape of a cuboid
  // with dimensions:
  double pixel_width = width / static_cast<double>(xPixelCount);
  double pixel_height = height / static_cast<double>(yPixelCount);

  // Create size strings for shape creation
  std::string pixel_width_str =
      boost::lexical_cast<std::string>(pixel_width / 2);
  std::string pixel_height_str =
      boost::lexical_cast<std::string>(pixel_height / 2);
  std::string pixel_depth_str =
      "0.00001"; // Set the depth of a pixel to a very small number

  // Define shape of a pixel as an XML string. See
  // http://www.mantidproject.org/HowToDefineGeometricShape for details on
  // shapes in Mantid.
  std::string detXML =
    "<cuboid id=\"pixel\">"
      "<left-front-bottom-point   x=\"+"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\"0\"  />"
      "<left-front-top-point      x=\"+"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\""+pixel_depth_str+"\"  />"
      "<left-back-bottom-point    x=\"-"+pixel_width_str+"\" y=\"-"+pixel_height_str+"\" z=\"0\"  />"
      "<right-front-bottom-point  x=\"+"+pixel_width_str+"\" y=\"+"+pixel_height_str+"\" z=\"0\"  />"
    "</cuboid>";

  // Create a shape object which will be shared by all pixels.
  Geometry::Object_sptr pixelShape =
      Geometry::ShapeFactory().createShape(detXML);

  // create detector banks
  BbyDetectorBankFactory factory(
      instrument, pixelShape, xPixelCount, yPixelCount, pixel_width,
      pixel_height, Kernel::V3D(0, (height - pixel_height) / 2, 0));

  // curtain l
  factory.createAndAssign(0 * pixelCount,
                          Kernel::V3D(+D_curtainl_value, 0, L2_curtainl_value),
                          Kernel::Quat(0, Kernel::V3D(0, 0, 1)) *
                              Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));

  // curtain r
  factory.createAndAssign(1 * pixelCount,
                          Kernel::V3D(-D_curtainr_value, 0, L2_curtainr_value),
                          Kernel::Quat(180, Kernel::V3D(0, 0, 1)) *
                              Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));

  // curtain u
  factory.createAndAssign(2 * pixelCount,
                          Kernel::V3D(0, +D_curtainu_value, L2_curtainu_value),
                          Kernel::Quat(90, Kernel::V3D(0, 0, 1)) *
                              Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));

  // curtain d
  factory.createAndAssign(3 * pixelCount,
                          Kernel::V3D(0, -D_curtaind_value, L2_curtaind_value),
                          Kernel::Quat(-90, Kernel::V3D(0, 0, 1)) *
                              Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));

  // back 1 (left)
  factory.createAndAssign(4 * pixelCount,
                          Kernel::V3D(+D_det_value, 0, L2_det_value),
                          Kernel::Quat(0, Kernel::V3D(0, 0, 1)));

  // back 2 (right)
  factory.createAndAssign(5 * pixelCount,
                          Kernel::V3D(-D_det_value, 0, L2_det_value),
                          Kernel::Quat(180, Kernel::V3D(0, 0, 1)));

  return instrument;
}

// load nx dataset
template <class T>
bool LoadBBY::loadNXDataSet(T &value, NeXus::NXEntry &entry,
                            const std::string &path) {
  try {
    // if (entry.isValid(path)) {
    NeXus::NXDataSetTyped<T> dataSet = entry.openNXDataSet<float>(path);
    dataSet.load();

    value = *dataSet();
    return true;
    //}
  } catch (std::runtime_error &) {
  }
  return false;
}

// read counts/events from binary file
template <class Counter>
void LoadBBY::loadEvents(API::Progress &prog, const char *progMsg,
                         ANSTO::Tar::File &file, const double tofMinBoundary,
                         const double tofMaxBoundary, Counter &counter) {
  prog.doReport(progMsg);

  // select bin file
  int64_t fileSize = 0;
  const std::vector<std::string> &files = file.files();
  for (auto itr = files.begin(); itr != files.end(); ++itr)
    if (itr->rfind(".bin") == itr->length() - 4) {
      file.select(itr->c_str());
      fileSize = file.selected_size();
      break;
    }

  // for progress notifications
  ANSTO::ProgressTracker progTracker(prog, progMsg, fileSize,
                                     Progress_LoadBinFile);

  unsigned int x = 0; // 9 bits [0-239] tube number
  unsigned int y = 0; // 8 bits [0-255] position along tube

  // uint v = 0; // 0 bits [     ]
  // uint w = 0; // 0 bits [     ] energy
  unsigned int dt = 0;
  double tof = 0.0;

  if ((fileSize == 0) || !file.skip(128))
    return;

  int state = 0;
  unsigned int c;
  while ((c = (unsigned int)file.read_byte()) != (unsigned int)-1) {

    bool event_ended = false;
    switch (state) {
    case 0:
      x = (c & 0xFF) >> 0; // set bit 1-8
      break;

    case 1:
      x |= (c & 0x01) << 8; // set bit 9
      y = (c & 0xFE) >> 1;  // set bit 1-7
      break;

    case 2:
      event_ended = (c & 0xC0) != 0xC0;
      if (!event_ended)
        c &= 0x3F;

      y |= (c & 0x01) << 7; // set bit 8
      dt = (c & 0xFE) >> 1; // set bit 1-5(7)
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
        tof = 0.0;
      } else if ((x >= HISTO_BINS_X) || (y >= HISTO_BINS_Y)) {
      } else {
        // conversion from 100 nanoseconds to 1 microsecond
        tof += ((int)dt) * 0.1;

        if ((tofMinBoundary <= tof) && (tof <= tofMaxBoundary))
          counter.addEvent(x, y, tof);
      }

      progTracker.update(file.selected_position());
    }
  }
}

// load mask file
std::vector<bool> LoadBBY::createMaskVector(const std::string &filename,
                                            bool &fileLoaded) {
  std::vector<bool> result(HISTO_BINS_X * HISTO_BINS_Y, true);

  std::ifstream input(filename.c_str());
  if (input.good()) {
    std::string line;
    while (std::getline(input, line)) {

      auto i0 = line.find("<detids>");
      auto iN = line.find("</detids>");

      if ((i0 != std::string::npos) && (iN != std::string::npos) && (i0 < iN)) {

        line = line.substr(i0 + 8, iN - i0 - 8);
        std::stringstream ss(line);

        std::string item;
        while (std::getline(ss, item, ',')) {
          auto k = item.find('-');

          size_t p0, p1;
          if (k != std::string::npos) {
            p0 = boost::lexical_cast<size_t>(item.substr(0, k));
            p1 = boost::lexical_cast<size_t>(
                item.substr(k + 1, item.size() - k - 1));

            if (p0 > p1)
              std::swap(p0, p1);
          } else {
            p0 = boost::lexical_cast<size_t>(item);
            p1 = p0;
          }

          if (p0 < result.size()) {

            if (p1 >= result.size())
              p1 = result.size() - 1;

            while (p0 <= p1)
              result[p0++] = false;
          }
        }
      }
    }
    fileLoaded = true;
  } else {
    fileLoaded = false;
  }

  return result;
}

// load tube offset file
std::vector<int> LoadBBY::createOffsetVector(const std::string &filename,
                                             bool &fileLoaded) {
  std::vector<int> result(HISTO_BINS_X, 0);
      
  std::ifstream input(filename.c_str());
  if (input.good()) {
    std::string line;
    while (std::getline(input, line)) {
      auto i1 = line.find_first_of(",;");
      if (i1 == std::string::npos)
        continue;

      auto i2 = line.find_first_of(",;", i1 + 1);
      if (i2 == std::string::npos)
        i2 = line.size();
          
      size_t index = boost::lexical_cast<size_t>(line.substr(0, i1));
      int offset = boost::lexical_cast<int>(line.substr(i1 + 1, line.size() - i1 - 1));

      if (index < HISTO_BINS_X)
        result[index] = offset;
    }
    fileLoaded = true;
  }
  else {
    fileLoaded = false;
  }
      
  return result;
}

// DetectorBankFactory
BbyDetectorBankFactory::BbyDetectorBankFactory(
    Geometry::Instrument_sptr instrument, Geometry::Object_sptr pixelShape,
    size_t xPixelCount, size_t yPixelCount, double pixelWidth,
    double pixelHeight, const Kernel::V3D &center)
    : m_instrument(instrument), m_pixelShape(pixelShape),
      m_xPixelCount(xPixelCount), m_yPixelCount(yPixelCount),
      m_pixelWidth(pixelWidth), m_pixelHeight(pixelHeight), m_center(center) {}
void BbyDetectorBankFactory::createAndAssign(size_t startIndex,
                                             const Kernel::V3D &pos,
                                             const Kernel::Quat &rot) const {
  // create a RectangularDetector which represents a rectangular array of pixels
  Geometry::RectangularDetector *bank = new Geometry::RectangularDetector(
      "bank",
      m_instrument.get()); // ??? possible memory leak!? "new" without "delete"

  bank->initialize(m_pixelShape,
                   // x
                   (int)m_xPixelCount, 0, m_pixelWidth,
                   // y
                   (int)m_yPixelCount, 0, m_pixelHeight,
                   // indices
                   (int)startIndex, true, (int)m_yPixelCount);

  for (size_t x = 0; x < m_xPixelCount; ++x)
    for (size_t y = 0; y < m_yPixelCount; ++y)
      m_instrument->markAsDetector(bank->getAtXY((int)x, (int)y).get());

  Kernel::V3D center(m_center);
  rot.rotate(center);

  bank->rotate(rot);
  bank->translate(pos - center);
}
} // namespace
} // namespace