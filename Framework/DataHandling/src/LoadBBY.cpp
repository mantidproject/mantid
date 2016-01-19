#include "MantidDataHandling/LoadBBY.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/NexusClasses.h"

#include <Poco/TemporaryFile.h>
#include <math.h>
#include <stdio.h>

namespace Mantid {
namespace DataHandling {

// register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadBBY)

// consts
static const size_t HISTO_BINS_X = 240;
static const size_t HISTO_BINS_Y = 256;
// 100 = 40 + 20 + 40
static const size_t Progress_LoadBinFile = 48;
static const size_t Progress_ReserveMemory = 4;
static const size_t Progress_Total =
    2 * Progress_LoadBinFile + Progress_ReserveMemory;

static char const *const FilenameStr = "Filename";
static char const *const MaskStr = "Mask";

static char const *const PeriodMasterStr = "PeriodMaster";
static char const *const PeriodSlaveStr = "PeriodSlave";
static char const *const PhaseSlaveStr = "PhaseSlave";

static char const *const FilterByTofMinStr = "FilterByTofMin";
static char const *const FilterByTofMaxStr = "FilterByTofMax";

using ANSTO::EventVector_pt;

template <typename TYPE>
void AddSinglePointTimeSeriesProperty(API::LogManager &logManager,
                                      const std::string &time,
                                      const std::string &name,
                                      const TYPE value) {
  // create time series property and add single value
  auto p = new Kernel::TimeSeriesProperty<TYPE>(name);
  p->addValue(time, value);

  // add to log manager
  logManager.addProperty(p);
}

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
  // Specify file extensions which can be associated with a specific file.
  std::vector<std::string> exts;

  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  exts.clear();
  exts.push_back(".tar");
  declareProperty(
      new API::FileProperty(FilenameStr, "", API::FileProperty::Load, exts),
      "The input filename of the stored data");

  // mask
  exts.clear();
  exts.push_back(".xml");
  declareProperty(
      new API::FileProperty(MaskStr, "", API::FileProperty::OptionalLoad, exts),
      "The input filename of the mask data");

  // OutputWorkspace
  declareProperty(new API::WorkspaceProperty<API::IEventWorkspace>(
      "OutputWorkspace", "", Kernel::Direction::Output));

  // FilterByTofMin
  declareProperty(new Kernel::PropertyWithValue<double>(
                      FilterByTofMinStr, 0, Kernel::Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the minimum accepted value in microseconds. Keep "
                  "blank to load all events.");

  // FilterByTofMax
  declareProperty(new Kernel::PropertyWithValue<double>(
                      FilterByTofMaxStr, EMPTY_DBL(), Kernel::Direction::Input),
                  "Optional: To exclude events that do not fall within a range "
                  "of times-of-flight. "
                  "This is the maximum accepted value in microseconds. Keep "
                  "blank to load all events.");

  // FilterByTimeStart
  declareProperty(
      new Kernel::PropertyWithValue<double>("FilterByTimeStart", EMPTY_DBL(),
                                            Kernel::Direction::Input),
      "Optional: To only include events after the provided start time, in "
      "seconds (relative to the start of the run).");

  // FilterByTimeStop
  declareProperty(
      new Kernel::PropertyWithValue<double>("FilterByTimeStop", EMPTY_DBL(),
                                            Kernel::Direction::Input),
      "Optional: To only include events before the provided stop time, in "
      "seconds (relative to the start of the run).");

  // period and phase
  declareProperty(new Kernel::PropertyWithValue<double>(
                      PeriodMasterStr, EMPTY_DBL(), Kernel::Direction::Input),
                  "Optional");
  declareProperty(new Kernel::PropertyWithValue<double>(
                      PeriodSlaveStr, EMPTY_DBL(), Kernel::Direction::Input),
                  "Optional");
  declareProperty(new Kernel::PropertyWithValue<double>(
                      PhaseSlaveStr, EMPTY_DBL(), Kernel::Direction::Input),
                  "Optional");

  std::string grpOptional = "Filters";
  setPropertyGroup(FilterByTofMinStr, grpOptional);
  setPropertyGroup(FilterByTofMaxStr, grpOptional);
  setPropertyGroup("FilterByTimeStart", grpOptional);
  setPropertyGroup("FilterByTimeStop", grpOptional);

  std::string grpPhaseCorrection = "Phase Correction";
  setPropertyGroup(PeriodMasterStr, grpPhaseCorrection);
  setPropertyGroup(PeriodSlaveStr, grpPhaseCorrection);
  setPropertyGroup(PhaseSlaveStr, grpPhaseCorrection);
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
  std::string filename = getPropertyValue(FilenameStr);
  ANSTO::Tar::File tarFile(filename);
  if (!tarFile.good())
    throw std::invalid_argument("invalid BBY file");

  // region of intreset
  std::vector<bool> roi = createRoiVector(getPropertyValue(MaskStr));

  double tofMinBoundary = getProperty(FilterByTofMinStr);
  double tofMaxBoundary = getProperty(FilterByTofMaxStr);

  API::Progress prog(this, 0.0, 1.0, Progress_Total);
  prog.doReport("creating instrument");

  // create workspace
  DataObjects::EventWorkspace_sptr eventWS =
      boost::make_shared<DataObjects::EventWorkspace>();

  eventWS->initialize(HISTO_BINS_Y * HISTO_BINS_X,
                      2, // number of TOF bin boundaries
                      1);

  // set the units
  eventWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  eventWS->setYUnit("Counts");

  // set title
  const std::vector<std::string> &subFiles = tarFile.files();
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

  // create instrument
  InstrumentInfo instrumentInfo;

  // Geometry::Instrument_sptr instrument =
  createInstrument(tarFile, /* ref */ instrumentInfo);
  // eventWS->setInstrument(instrument);

  // load events
  size_t numberHistograms = eventWS->getNumberHistograms();

  std::vector<EventVector_pt> eventVectors(numberHistograms, NULL);
  std::vector<size_t> eventCounts(numberHistograms, 0);

  // phase correction
  Kernel::Property *periodMasterProperty =
      getPointerToProperty(PeriodMasterStr);
  Kernel::Property *periodSlaveProperty = getPointerToProperty(PeriodSlaveStr);
  Kernel::Property *phaseSlaveProperty = getPointerToProperty(PhaseSlaveStr);

  double periodMaster;
  double periodSlave;
  double phaseSlave;

  if (periodMasterProperty->isDefault() || periodSlaveProperty->isDefault() ||
      phaseSlaveProperty->isDefault()) {

    if (!periodMasterProperty->isDefault() ||
        !periodSlaveProperty->isDefault() || !phaseSlaveProperty->isDefault()) {
      throw std::invalid_argument("Please specify PeriodMaster, PeriodSlave "
                                  "and PhaseSlave or none of them.");
    }

    // if values have not been specified in loader then use values from hdf file
    periodMaster = instrumentInfo.period_master;
    periodSlave = instrumentInfo.period_slave;
    phaseSlave = instrumentInfo.phase_slave;
  } else {
    periodMaster = getProperty(PeriodMasterStr);
    periodSlave = getProperty(PeriodSlaveStr);
    phaseSlave = getProperty(PhaseSlaveStr);

    if ((periodMaster < 0.0) || (periodSlave < 0.0))
      throw std::invalid_argument(
          "Please specify a positive value for PeriodMaster and PeriodSlave.");
  }

  double period = periodSlave;
  double shift = -1.0 / 6.0 * periodMaster - periodSlave * phaseSlave / 360.0;

  // count total events per pixel to reserve necessary memory
  ANSTO::EventCounter eventCounter(roi, HISTO_BINS_Y, period, shift,
                                   tofMinBoundary, tofMaxBoundary, eventCounts);

  loadEvents(prog, "loading neutron counts", tarFile, eventCounter);

  // prepare event storage
  ANSTO::ProgressTracker progTracker(prog, "creating neutron event lists",
                                     numberHistograms, Progress_ReserveMemory);

  for (size_t i = 0; i != numberHistograms; ++i) {
    DataObjects::EventList &eventList = eventWS->getEventList(i);

    eventList.setSortOrder(DataObjects::PULSETIME_SORT);
    eventList.reserve(eventCounts[i]);

    eventList.setDetectorID(Mantid::detid_t(i));
    eventList.setSpectrumNo(Mantid::detid_t(i));

    DataObjects::getEventsFrom(eventList, eventVectors[i]);

    progTracker.update(i);
  }
  progTracker.complete();

  ANSTO::EventAssigner eventAssigner(roi, HISTO_BINS_Y, period, shift,
                                     tofMinBoundary, tofMaxBoundary,
                                     eventVectors);

  loadEvents(prog, "loading neutron events", tarFile, eventAssigner);

  Kernel::cow_ptr<MantidVec> axis;
  MantidVec &xRef = axis.access();
  xRef.resize(2, 0.0);
  xRef[0] = std::max(
      0.0,
      floor(eventCounter.tofMin())); // just to make sure the bins hold it all
  xRef[1] = eventCounter.tofMax() + 1;
  eventWS->setAllX(axis);

  // count total number of masked bins
  size_t maskedBins = 0;
  for (size_t i = 0; i != roi.size(); i++)
    if (!roi[i])
      maskedBins++;

  if (maskedBins > 0) {
    // create list of masked bins
    std::vector<size_t> maskIndexList(maskedBins);
    size_t maskIndex = 0;

    for (size_t i = 0; i != roi.size(); i++)
      if (!roi[i])
        maskIndexList[maskIndex++] = i;

    API::IAlgorithm_sptr maskingAlg = createChildAlgorithm("MaskDetectors");
    maskingAlg->setProperty("Workspace", eventWS);
    maskingAlg->setProperty("WorkspaceIndexList", maskIndexList);
    maskingAlg->executeAsChildAlg();
  }

  // set log values
  API::LogManager &logManager = eventWS->mutableRun();

  logManager.addProperty("filename", filename);
  logManager.addProperty("att_pos", static_cast<int>(instrumentInfo.att_pos));
  logManager.addProperty("frame_count",
                         static_cast<int>(eventCounter.numFrames()));
  logManager.addProperty("period", period);

  // currently beam monitor counts are not available, instead number of frames
  // times period is used
  logManager.addProperty(
      "bm_counts", static_cast<double>(eventCounter.numFrames()) * period /
                       1.0e6); // static_cast<double>(instrumentInfo.bm_counts)

  // currently
  Kernel::time_duration duration =
      boost::posix_time::microseconds(static_cast<boost::int64_t>(
          static_cast<double>(eventCounter.numFrames()) * period));

  Kernel::DateAndTime start_time("2000-01-01T00:00:00");
  Kernel::DateAndTime end_time(start_time + duration);

  logManager.addProperty("start_time", start_time.toISO8601String());
  logManager.addProperty("end_time", end_time.toISO8601String());

  std::string time_str = start_time.toISO8601String();
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L1_chopper_value",
                                   instrumentInfo.L1_chopper_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_det_value",
                                   instrumentInfo.L2_det_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_curtainl_value",
                                   instrumentInfo.L2_curtainl_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_curtainr_value",
                                   instrumentInfo.L2_curtainr_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_curtainu_value",
                                   instrumentInfo.L2_curtainu_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "L2_curtaind_value",
                                   instrumentInfo.L2_curtaind_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "D_det_value",
                                   instrumentInfo.D_det_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "D_curtainl_value",
                                   instrumentInfo.D_curtainl_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "D_curtainr_value",
                                   instrumentInfo.D_curtainr_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "D_curtainu_value",
                                   instrumentInfo.D_curtainu_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "D_curtaind_value",
                                   instrumentInfo.D_curtaind_value);
  AddSinglePointTimeSeriesProperty(logManager, time_str, "curtain_rotation",
                                   10.0);

  API::IAlgorithm_sptr loadInstrumentAlg =
      createChildAlgorithm("LoadInstrument");
  loadInstrumentAlg->setProperty("Workspace", eventWS);
  loadInstrumentAlg->setPropertyValue("InstrumentName", "BILBY");
  loadInstrumentAlg->setProperty("RewriteSpectraMap",
                                 Mantid::Kernel::OptionalBool(false));
  loadInstrumentAlg->executeAsChildAlg();

  setProperty("OutputWorkspace", eventWS);
}

// region of intreset
std::vector<bool> LoadBBY::createRoiVector(const std::string &maskfile) {
  std::vector<bool> result(HISTO_BINS_Y * HISTO_BINS_X, true);

  if (maskfile.length() == 0)
    return result;

  std::ifstream input(maskfile.c_str());
  if (!input.good())
    throw std::invalid_argument("invalid mask file");

  std::string line;
  while (std::getline(input, line)) {
    auto i0 = line.find("<detids>");
    auto iN = line.find("</detids>");

    if ((i0 != std::string::npos) && (iN != std::string::npos) && (i0 < iN)) {
      line = line.substr(i0 + 8, iN - i0 - 8); // 8 = len("<detids>")
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

  return result;
}

// instrument creation
Geometry::Instrument_sptr
LoadBBY::createInstrument(ANSTO::Tar::File &tarFile,
                          InstrumentInfo &instrumentInfo) {
  instrumentInfo.bm_counts = 0;
  instrumentInfo.att_pos = 0;

  instrumentInfo.period_master = 0.0;
  instrumentInfo.period_slave = 0.0;
  instrumentInfo.phase_slave = 0.0;

  instrumentInfo.L1_chopper_value = 18.47258984375;
  instrumentInfo.L2_det_value = 33.15616015625;

  instrumentInfo.L2_curtainl_value = 23.28446093750;
  instrumentInfo.L2_curtainr_value = 23.28201953125;
  instrumentInfo.L2_curtainu_value = 24.28616015625;
  instrumentInfo.L2_curtaind_value = 24.28235937500;

  instrumentInfo.D_det_value = (8.4 + 2.0) / (2 * 1000);

  instrumentInfo.D_curtainl_value = 0.3816;
  instrumentInfo.D_curtainr_value = 0.4024;
  instrumentInfo.D_curtainu_value = 0.3947;
  instrumentInfo.D_curtaind_value = 0.3978;

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
    // extract hdf file into tmp file
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

      float tmp_float;
      int32_t tmp_int32 = 0;
      const double toMeters = 1.0 / 1000;

      if (loadNXDataSet(entry, "monitor/bm1_counts", tmp_int32))
        instrumentInfo.bm_counts = tmp_int32;
      if (loadNXDataSet(entry, "instrument/att_pos", tmp_float))
        instrumentInfo.att_pos =
            static_cast<int32_t>(tmp_float + 0.5f); // [1.0, 2.0, ..., 5.0]

      if (loadNXDataSet(entry, "instrument/master_chopper_freq", tmp_float))
        instrumentInfo.period_master = 1.0 / tmp_float * 1.0e6;
      if (loadNXDataSet(entry, "instrument/t0_chopper_freq", tmp_float))
        instrumentInfo.period_slave = 1.0 / tmp_float * 1.0e6;
      if (loadNXDataSet(entry, "instrument/t0_chopper_phase", tmp_float))
        instrumentInfo.phase_slave = tmp_float < 999.0 ? tmp_float : 0.0;

      if (loadNXDataSet(entry, "instrument/L2_det", tmp_float))
        instrumentInfo.L2_det_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/Ltof_det", tmp_float))
        instrumentInfo.L1_chopper_value =
            tmp_float * toMeters - instrumentInfo.L2_det_value;
      // if (loadNXDataSet(entry, "instrument/L1", tmp_float))
      //  instrumentInfo.L1_source_value = tmp_float * toMeters;

      if (loadNXDataSet(entry, "instrument/L2_curtainl", tmp_float))
        instrumentInfo.L2_curtainl_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/L2_curtainr", tmp_float))
        instrumentInfo.L2_curtainr_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/L2_curtainu", tmp_float))
        instrumentInfo.L2_curtainu_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/L2_curtaind", tmp_float))
        instrumentInfo.L2_curtaind_value = tmp_float * toMeters;

      if (loadNXDataSet(entry, "instrument/detector/curtainl", tmp_float))
        instrumentInfo.D_curtainl_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/detector/curtainr", tmp_float))
        instrumentInfo.D_curtainr_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/detector/curtainu", tmp_float))
        instrumentInfo.D_curtainu_value = tmp_float * toMeters;
      if (loadNXDataSet(entry, "instrument/detector/curtaind", tmp_float))
        instrumentInfo.D_curtaind_value = tmp_float * toMeters;
    }
  }

  return Geometry::Instrument_sptr();

  /*
  // instrument
  Geometry::Instrument_sptr instrument =
  boost::make_shared<Geometry::Instrument>("BILBY");
  instrument->setDefaultViewAxis("Z-");

  // source
  Geometry::ObjComponent *source = new Geometry::ObjComponent("Source",
  instrument.get());
  instrument->add(source);
  instrument->markAsSource(source);

  // sample
  Geometry::ObjComponent *samplePos = new Geometry::ObjComponent("Sample",
  instrument.get());
  instrument->add(samplePos);
  instrument->markAsSamplePos(samplePos);

  source->setPos(0.0, 0.0, -instrumentInfo.L1_chopper_value);
  samplePos->setPos(0.0, 0.0, 0.0);

  // dimensions of the detector (height is in y direction, width is in x
  // direction)
  double width = 336.0 / 1000;  // meters
  double height = 640.0 / 1000; // meters
  double angle = 10.0;          // degree

  // raw data format
  size_t xPixelCount = HISTO_BINS_X / 6;
  size_t yPixelCount = HISTO_BINS_Y;

  // we assumed that individual pixels have the same size and shape of a cuboid:
  double pixel_width = width / static_cast<double>(xPixelCount);
  double pixel_height = height / static_cast<double>(yPixelCount);

  // final number of pixels
  size_t pixelCount = xPixelCount * yPixelCount;

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
      "<left-front-bottom-point   x=\"+"+pixel_width_str+"\"
  y=\"-"+pixel_height_str+"\" z=\"0\"  />"
      "<left-front-top-point      x=\"+"+pixel_width_str+"\"
  y=\"-"+pixel_height_str+"\" z=\""+pixel_depth_str+"\"  />"
      "<left-back-bottom-point    x=\"-"+pixel_width_str+"\"
  y=\"-"+pixel_height_str+"\" z=\"0\"  />"
      "<right-front-bottom-point  x=\"+"+pixel_width_str+"\"
  y=\"+"+pixel_height_str+"\" z=\"0\"  />"
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
                          Kernel::V3D(+instrumentInfo.D_curtainl_value, 0,
  instrumentInfo.L2_curtainl_value),
                          Kernel::Quat(0, Kernel::V3D(0, 0, 1)) *
                              Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));

  // curtain r
  factory.createAndAssign(1 * pixelCount,
                          Kernel::V3D(-instrumentInfo.D_curtainr_value, 0,
  instrumentInfo.L2_curtainr_value),
                          Kernel::Quat(180, Kernel::V3D(0, 0, 1)) *
                              Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));

  // curtain u
  factory.createAndAssign(2 * pixelCount,
                          Kernel::V3D(0, +instrumentInfo.D_curtainu_value,
  instrumentInfo.L2_curtainu_value),
                          Kernel::Quat(90, Kernel::V3D(0, 0, 1)) *
                              Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));

  // curtain d
  factory.createAndAssign(3 * pixelCount,
                          Kernel::V3D(0, -instrumentInfo.D_curtaind_value,
  instrumentInfo.L2_curtaind_value),
                          Kernel::Quat(-90, Kernel::V3D(0, 0, 1)) *
                              Kernel::Quat(angle, Kernel::V3D(0, 1, 0)));

  // back 1 (left)
  factory.createAndAssign(4 * pixelCount,
                          Kernel::V3D(+instrumentInfo.D_det_value, 0,
  instrumentInfo.L2_det_value),
                          Kernel::Quat(0, Kernel::V3D(0, 0, 1)));

  // back 2 (right)
  factory.createAndAssign(5 * pixelCount,
                          Kernel::V3D(-instrumentInfo.D_det_value, 0,
  instrumentInfo.L2_det_value),
                          Kernel::Quat(180, Kernel::V3D(0, 0, 1)));

  return instrument;
  */
}

// load nx dataset
template <class T>
bool LoadBBY::loadNXDataSet(NeXus::NXEntry &entry, const std::string &path,
                            T &value) {
  try {
    NeXus::NXDataSetTyped<T> dataSet = entry.openNXDataSet<T>(path);
    dataSet.load();

    value = *dataSet();
    return true;
  } catch (std::runtime_error &) {
    return false;
  }
}

// read counts/events from binary file
template <class EventProcessor>
void LoadBBY::loadEvents(API::Progress &prog, const char *progMsg,
                         ANSTO::Tar::File &tarFile,
                         EventProcessor &eventProcessor) {
  prog.doReport(progMsg);

  bool countsInFrame = false;

  // select bin file
  int64_t fileSize = 0;
  const std::vector<std::string> &files = tarFile.files();
  for (auto itr = files.begin(); itr != files.end(); ++itr)
    if (itr->rfind(".bin") == itr->length() - 4) {
      tarFile.select(itr->c_str());
      fileSize = tarFile.selected_size();
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

  if ((fileSize == 0) || !tarFile.skip(128))
    return;

  int state = 0;
  unsigned int c;
  while ((c = (unsigned int)tarFile.read_byte()) != (unsigned int)-1) {

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

        // only count frames that contain neutrons
        if (countsInFrame) {
          eventProcessor.endOfFrame();
          countsInFrame = false;
        }
      } else if ((x >= HISTO_BINS_X) || (y >= HISTO_BINS_Y)) {
      } else {
        // conversion from 100 nanoseconds to 1 microsecond
        tof += ((int)dt) * 0.1;

        eventProcessor.addEvent(x, y, tof);
        countsInFrame = true;
      }

      progTracker.update(tarFile.selected_position());
    }
  }

  if (countsInFrame)
    eventProcessor.endOfFrame();
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
      m_instrument.get()); // Bank gets registered with instrument component.
                           // instrument acts as sink and manages lifetime.

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

} // DataHandling
} // Mantid
