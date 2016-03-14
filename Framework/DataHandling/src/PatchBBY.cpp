#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/PatchBBY.h"
#include "MantidKernel/PropertyWithValue.h"

#include <Poco/TemporaryFile.h>

namespace Mantid {
namespace DataHandling {

// register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PatchBBY)

enum TYPE {
  TYPE_INT,
  TYPE_DBL,
};

struct PropertyInfo {
  const char *const Group;
  const char *const Name;
  TYPE const Type;
};

// consts
static char const *const HistoryStr = "History.log";
static char const *const FilenameStr = "Filename";
static PropertyInfo PatchableProperties[] = {
    {"Group1", "Bm1Counts", TYPE_INT},
    {"Group1", "AttPos", TYPE_DBL},

    {"Group2", "MasterChopperFreq", TYPE_DBL},
    {"Group2", "T0ChopperFreq", TYPE_DBL},
    {"Group2", "T0ChopperPhase", TYPE_DBL},

    //{ "Group3", "L1", TYPE_DBL},
    {"Group3", "L2Det", TYPE_DBL},
    {"Group3", "LTofDet", TYPE_DBL},

    {"Group4", "L2CurtainL", TYPE_DBL},
    {"Group4", "L2CurtainR", TYPE_DBL},
    {"Group4", "L2CurtainU", TYPE_DBL},
    {"Group4", "L2CurtainD", TYPE_DBL},

    {"Group5", "CurtainL", TYPE_DBL},
    {"Group5", "CurtainR", TYPE_DBL},
    {"Group5", "CurtainU", TYPE_DBL},
    {"Group5", "CurtainD", TYPE_DBL},
};

// load nx dataset
template <class T>
bool loadNXDataSet(NeXus::NXEntry &entry, const std::string &path, T &value) {
  try {
    NeXus::NXDataSetTyped<T> dataSet = entry.openNXDataSet<T>(path);
    dataSet.load();

    value = *dataSet();
    return true;
  } catch (std::runtime_error &) {
    return false;
  }
}

/**
 * Initialise the algorithm. Declare properties which can be set before
 * execution (input) or
 * read from after the execution (output).
 */
void PatchBBY::init() {
  // Specify file extensions which can be associated with a specific file.
  std::vector<std::string> exts;

  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  exts.clear();
  exts.push_back(".tar");
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      FilenameStr, "", API::FileProperty::Load, exts),
                  "The filename of the stored data to be patched");

  // patchable properties
  for (auto itr = std::begin(PatchableProperties);
       itr != std::end(PatchableProperties); ++itr) {
    switch (itr->Type) {
    case TYPE_INT:
      declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<int>>(
                          itr->Name, EMPTY_INT(), Kernel::Direction::Input),
                      "Optional");
      break;

    case TYPE_DBL:
      declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<double>>(
                          itr->Name, EMPTY_DBL(), Kernel::Direction::Input),
                      "Optional");
      break;
    }

    setPropertyGroup(itr->Name, itr->Group);
  }

  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<bool>>(
                      "Reset", false, Kernel::Direction::Input),
                  "Optional");
}
/**
 * Execute the algorithm.
 */
void PatchBBY::exec() {
  // get the name of the data file
  std::string filename = getPropertyValue(FilenameStr);
  ANSTO::Tar::File tarFile(filename);
  if (!tarFile.good())
    throw std::invalid_argument("invalid BBY file");

  size_t hdfFiles = 0;
  size_t binFiles = 0;
  size_t logFiles = 0;
  size_t logSize = 0;

  // open file (and select hisotry file if it exists)
  const std::vector<std::string> &files = tarFile.files();
  for (auto itr = files.begin(); itr != files.end(); ++itr) {
    auto len = itr->length();

    if ((len > 4) && (itr->find_first_of("\\/", 0, 2) == std::string::npos)) {
      if ((itr->compare(0, 3, "BBY") == 0) && (itr->rfind(".hdf") == len - 4))
        hdfFiles++;
      else if (itr->rfind(".bin") == len - 4)
        binFiles++;
      else if (itr->compare(HistoryStr) == 0) {
        if (std::distance(itr, files.end()) != 1)
          throw std::invalid_argument(
              "invalid BBY file (history has to be at the end)");

        logFiles++;
        tarFile.select(itr->c_str());
        logSize = tarFile.selected_size();
      }
    }
  }

  // check if it's valid
  if ((hdfFiles != 1) || (binFiles != 1) || (logFiles > 1))
    throw std::invalid_argument("invalid BBY file");

  // read existing history
  std::string logContent;
  if (logFiles != 0) {
    logContent.resize(logSize);
    tarFile.read(&logContent[0], logSize);
  }

  // create new content
  std::ostringstream logContentNewBuffer;
  int tmp_int;
  double tmp_dbl;
  for (auto itr = std::begin(PatchableProperties);
       itr != std::end(PatchableProperties); ++itr) {
    auto property_value = getProperty(itr->Name);
    // if (!isEmpty(property_value))
    switch (itr->Type) {
    case TYPE_INT:
      tmp_int = property_value;
      if (tmp_int != EMPTY_INT()) // !!!
        logContentNewBuffer << itr->Name << " = " << tmp_int << std::endl;
      break;

    case TYPE_DBL:
      tmp_dbl = property_value;
      if (tmp_dbl != EMPTY_DBL()) // !!!
        logContentNewBuffer << itr->Name << " = " << tmp_dbl << std::endl;
      break;
    }
  }

  // load original values from hdf
  bool reset = getProperty("Reset");
  if (reset) {
    const auto file_it =
        std::find_if(files.cbegin(), files.cend(), [](const std::string &file) {
          return file.rfind(".hdf") == file.length() - 4;
        });
    if (file_it != files.end()) {
      // extract hdf file into tmp file
      Poco::TemporaryFile hdfFile;
      boost::shared_ptr<FILE> handle(fopen(hdfFile.path().c_str(), "wb"),
                                     fclose);
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

        if (loadNXDataSet(entry, "monitor/bm1_counts", tmp_int32))
          logContentNewBuffer << "bm1_counts"
                              << " = " << tmp_int32 << std::endl;
        if (loadNXDataSet(entry, "instrument/att_pos", tmp_float))
          logContentNewBuffer << "att_pos"
                              << " = " << tmp_float << std::endl;

        if (loadNXDataSet(entry, "instrument/master_chopper_freq", tmp_float))
          logContentNewBuffer << "master_chopper_freq"
                              << " = " << tmp_float << std::endl;
        if (loadNXDataSet(entry, "instrument/t0_chopper_freq", tmp_float))
          logContentNewBuffer << "t0_chopper_freq"
                              << " = " << tmp_float << std::endl;
        if (loadNXDataSet(entry, "instrument/t0_chopper_phase", tmp_float))
          logContentNewBuffer << "t0_chopper_phase"
                              << " = " << (tmp_float < 999.0 ? tmp_float : 0.0)
                              << std::endl;

        if (loadNXDataSet(entry, "instrument/L2_det", tmp_float))
          logContentNewBuffer << "L2_det"
                              << " = " << tmp_float << std::endl;
        if (loadNXDataSet(entry, "instrument/Ltof_det", tmp_float))
          logContentNewBuffer << "Ltof_det"
                              << " = " << tmp_float << std::endl;

        if (loadNXDataSet(entry, "instrument/L2_curtainl", tmp_float))
          logContentNewBuffer << "L2_curtainl"
                              << " = " << tmp_float << std::endl;
        if (loadNXDataSet(entry, "instrument/L2_curtainr", tmp_float))
          logContentNewBuffer << "L2_curtainr"
                              << " = " << tmp_float << std::endl;
        if (loadNXDataSet(entry, "instrument/L2_curtainu", tmp_float))
          logContentNewBuffer << "L2_curtainu"
                              << " = " << tmp_float << std::endl;
        if (loadNXDataSet(entry, "instrument/L2_curtaind", tmp_float))
          logContentNewBuffer << "L2_curtaind"
                              << " = " << tmp_float << std::endl;

        if (loadNXDataSet(entry, "instrument/detector/curtainl", tmp_float))
          logContentNewBuffer << "curtainl"
                              << " = " << tmp_float << std::endl;
        if (loadNXDataSet(entry, "instrument/detector/curtainr", tmp_float))
          logContentNewBuffer << "curtainr"
                              << " = " << tmp_float << std::endl;
        if (loadNXDataSet(entry, "instrument/detector/curtainu", tmp_float))
          logContentNewBuffer << "curtainu"
                              << " = " << tmp_float << std::endl;
        if (loadNXDataSet(entry, "instrument/detector/curtaind", tmp_float))
          logContentNewBuffer << "curtaind"
                              << " = " << tmp_float << std::endl;
      }
    }
  }

  std::string logContentNew = logContentNewBuffer.str();
  if (logContentNew.size() == 0)
    throw std::invalid_argument("nothing to patch");

  // merge log content
  logContent.append(logContentNew);

  // append patches to file
  tarFile.close();
  if (!ANSTO::Tar::File::append(filename, HistoryStr, logContent.c_str(),
                                logContent.size()))
    throw std::runtime_error("unable to patch");
}

} // DataHandling
} // Mantid
