// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/PatchBBY.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidNexus/NexusClasses.h"

#include <Poco/String.h>
#include <Poco/TemporaryFile.h>

namespace Mantid::DataHandling {

// register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PatchBBY)

enum TYPE {
  TYPE_STR,
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
static char const *const EXTERNAL = "EXTERNAL";
static char const *const INTERNAL = "INTERNAL";
static PropertyInfo PatchableProperties[] = {
    {"Calibration", "Bm1Counts", TYPE_INT},
    {"Calibration", "AttPos", TYPE_DBL},

    {"Velocity Selector and Choppers", "MasterChopperFreq", TYPE_DBL},
    {"Velocity Selector and Choppers", "T0ChopperFreq", TYPE_DBL},
    {"Velocity Selector and Choppers", "T0ChopperPhase", TYPE_DBL},
    {"Velocity Selector and Choppers", "FrameSource", TYPE_STR},
    {"Velocity Selector and Choppers", "Wavelength", TYPE_DBL},

    {"Geometry Setup", "L1", TYPE_DBL},
    {"Geometry Setup", "LTofDet", TYPE_DBL},
    {"Geometry Setup", "L2Det", TYPE_DBL},

    {"Geometry Setup", "L2CurtainL", TYPE_DBL},
    {"Geometry Setup", "L2CurtainR", TYPE_DBL},
    {"Geometry Setup", "L2CurtainU", TYPE_DBL},
    {"Geometry Setup", "L2CurtainD", TYPE_DBL},

    {"Geometry Setup", "CurtainL", TYPE_DBL},
    {"Geometry Setup", "CurtainR", TYPE_DBL},
    {"Geometry Setup", "CurtainU", TYPE_DBL},
    {"Geometry Setup", "CurtainD", TYPE_DBL},
};

// load nx dataset
template <class T> bool loadNXDataSet(NeXus::NXEntry &entry, const std::string &path, T &value) {
  try {
    NeXus::NXDataSetTyped<T> dataSet = entry.openNXDataSet<T>(path);
    dataSet.load();

    value = *dataSet();
    return true;
  } catch (std::runtime_error &) {
    return false;
  }
}
bool loadNXString(const NeXus::NXEntry &entry, const std::string &path, std::string &value) {
  try {
    NeXus::NXChar buffer = entry.openNXChar(path);
    buffer.load();

    value = std::string(buffer(), buffer.dim0());
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
  exts.emplace_back(".tar");
  declareProperty(std::make_unique<API::FileProperty>(FilenameStr, "", API::FileProperty::Load, exts),
                  "The filename of the stored data to be patched");

  // patchable properties
  for (auto itr = std::begin(PatchableProperties); itr != std::end(PatchableProperties); ++itr) {
    switch (itr->Type) {
    case TYPE_INT:
      declareProperty(
          std::make_unique<Kernel::PropertyWithValue<int>>(itr->Name, EMPTY_INT(), Kernel::Direction::Input),
          "Optional");
      break;

    case TYPE_DBL:
      declareProperty(
          std::make_unique<Kernel::PropertyWithValue<double>>(itr->Name, EMPTY_DBL(), Kernel::Direction::Input),
          "Optional");
      break;

    case TYPE_STR:
      if (std::strcmp(itr->Name, "FrameSource") == 0) {
        std::array<std::string, 3> keys = {{"", EXTERNAL, INTERNAL}};
        declareProperty(
            std::make_unique<Kernel::PropertyWithValue<std::string>>(
                itr->Name, "", std::make_shared<Kernel::ListValidator<std::string>>(keys), Kernel::Direction::Input),
            "Optional");
      }
      break;
    }

    setPropertyGroup(itr->Name, itr->Group);
  }

  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("Reset", false, Kernel::Direction::Input),
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
      else if (*itr == HistoryStr) {
        if (std::distance(itr, files.end()) != 1)
          throw std::invalid_argument("invalid BBY file (history has to be at the end)");

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
  std::string tmp_str;
  for (auto itr = std::begin(PatchableProperties); itr != std::end(PatchableProperties); ++itr) {
    auto property_value = getProperty(itr->Name);

    // if (!isEmpty(property_value))
    switch (itr->Type) {
    case TYPE_INT:
      tmp_int = property_value;
      if (tmp_int != EMPTY_INT())
        logContentNewBuffer << itr->Name << " = " << tmp_int << '\n';
      break;

    case TYPE_DBL:
      tmp_dbl = property_value;
      if (tmp_dbl != EMPTY_DBL())
        logContentNewBuffer << itr->Name << " = " << tmp_dbl << '\n';
      break;

    case TYPE_STR:
      if (std::strcmp(itr->Name, "FrameSource") != 0)
        throw std::invalid_argument(std::string("not implemented: ") + itr->Name);

      tmp_str = static_cast<std::string>(property_value);
      if (!tmp_str.empty()) {
        if (Poco::icompare(tmp_str, EXTERNAL) == 0)
          logContentNewBuffer << itr->Name << " = " << EXTERNAL << '\n';
        else if (Poco::icompare(tmp_str, INTERNAL) == 0)
          logContentNewBuffer << itr->Name << " = " << INTERNAL << '\n';
        else
          throw std::invalid_argument("invalid: FrameSource");
      }
      break;
    }
  }

  // load original values from hdf
  bool reset = getProperty("Reset");
  if (reset) {
    const auto file_it = std::find_if(files.cbegin(), files.cend(),
                                      [](const std::string &file) { return file.rfind(".hdf") == file.length() - 4; });
    if (file_it != files.end()) {
      tarFile.select(file_it->c_str());
      // extract hdf file into tmp file
      Poco::TemporaryFile hdfFile;
      std::shared_ptr<FILE> handle(fopen(hdfFile.path().c_str(), "wb"), fclose);
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
          logContentNewBuffer << "Bm1Counts = " << tmp_int32 << '\n';
        if (loadNXDataSet(entry, "instrument/att_pos", tmp_float))
          logContentNewBuffer << "AttPos = " << tmp_float << '\n';

        if (loadNXString(entry, "instrument/detector/frame_source", tmp_str))
          logContentNewBuffer << "FrameSource = " << tmp_str << '\n';
        if (loadNXDataSet(entry, "instrument/nvs067/lambda", tmp_float))
          logContentNewBuffer << "Wavelength = " << tmp_float << '\n';

        if (loadNXDataSet(entry, "instrument/master_chopper_freq", tmp_float))
          logContentNewBuffer << "MasterChopperFreq = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/t0_chopper_freq", tmp_float))
          logContentNewBuffer << "T0ChopperFreq = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/t0_chopper_phase", tmp_float))
          logContentNewBuffer << "T0ChopperPhase = " << (tmp_float < 999.0 ? tmp_float : 0.0) << '\n';

        if (loadNXDataSet(entry, "instrument/L1", tmp_float))
          logContentNewBuffer << "L1 = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/Ltof_det", tmp_float))
          logContentNewBuffer << "LTofDet = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/L2_det", tmp_float))
          logContentNewBuffer << "L2Det = " << tmp_float << '\n';

        if (loadNXDataSet(entry, "instrument/L2_curtainl", tmp_float))
          logContentNewBuffer << "L2CurtainL = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/L2_curtainr", tmp_float))
          logContentNewBuffer << "L2CurtainR = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/L2_curtainu", tmp_float))
          logContentNewBuffer << "L2CurtainU = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/L2_curtaind", tmp_float))
          logContentNewBuffer << "L2CurtainD = " << tmp_float << '\n';

        if (loadNXDataSet(entry, "instrument/detector/curtainl", tmp_float))
          logContentNewBuffer << "CurtainL = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/detector/curtainr", tmp_float))
          logContentNewBuffer << "CurtainR = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/detector/curtainu", tmp_float))
          logContentNewBuffer << "CurtainU = " << tmp_float << '\n';
        if (loadNXDataSet(entry, "instrument/detector/curtaind", tmp_float))
          logContentNewBuffer << "CurtainD = " << tmp_float << '\n';
      }
    }
  }

  std::string logContentNew = logContentNewBuffer.str();
  if (logContentNew.empty())
    throw std::invalid_argument("nothing to patch");

  // merge log content
  logContent.append(logContentNew);

  // append patches to file
  tarFile.close();
  if (!ANSTO::Tar::File::append(filename, HistoryStr, logContent.c_str(), logContent.size()))
    throw std::runtime_error("unable to patch");
}

} // namespace Mantid::DataHandling
