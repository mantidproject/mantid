#include "MantidDataHandling/PatchBBY.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/FileProperty.h"

namespace Mantid {
namespace DataHandling {

// register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PatchBBY)

enum TYPE {
  TYPE_INT,
  TYPE_DBL,
};

struct PropertyInfo {
  const char* const Group;
  const char* const Name;
  TYPE        const Type;
};

// consts
static char const* const HistoryStr = "History.log";
static char const* const FilenameStr = "Filename";
static PropertyInfo PatchableProperties[] = {
  { "Group1", "bm1_counts"         , TYPE_INT },
  { "Group1", "att_pos"            , TYPE_DBL },

  { "Group2", "master_chopper_freq", TYPE_DBL },
  { "Group2", "t0_chopper_freq"    , TYPE_DBL },
  { "Group2", "t0_chopper_phase"   , TYPE_DBL },
  
  //{ "Group3", "L1"                 , TYPE_DBL },
  { "Group3", "L2_det"             , TYPE_DBL },
  { "Group3", "Ltof_det"           , TYPE_DBL },
  
  { "Group4", "L2_curtainl"        , TYPE_DBL },
  { "Group4", "L2_curtainr"        , TYPE_DBL },
  { "Group4", "L2_curtainu"        , TYPE_DBL },
  { "Group4", "L2_curtaind"        , TYPE_DBL },
  
  { "Group5", "curtainl"           , TYPE_DBL },
  { "Group5", "curtainr"           , TYPE_DBL },
  { "Group5", "curtainu"           , TYPE_DBL },
  { "Group5", "curtaind"           , TYPE_DBL },
};

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
  declareProperty(
      new API::FileProperty(FilenameStr, "", API::FileProperty::Load, exts),
      "The filename of the stored data to be patched");
  
  // patchable properties
  for (auto itr = std::begin(PatchableProperties); itr != std::end(PatchableProperties); ++itr) {
    switch (itr->Type) {
    case TYPE_INT:
      declareProperty(
          new Kernel::PropertyWithValue<int>(itr->Name, EMPTY_INT(), Kernel::Direction::Input),
          "Optional");
      break;

    case TYPE_DBL:
      declareProperty(
          new Kernel::PropertyWithValue<double>(itr->Name, EMPTY_DBL(), Kernel::Direction::Input),
          "Optional");
      break;
    }

    setPropertyGroup(itr->Name, itr->Group);
  }
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
  const std::vector<std::string> &subFiles = tarFile.files();
  for (auto itr = subFiles.begin(); itr != subFiles.end(); ++itr) {
    auto len = itr->length();

    if ((len > 4) && (itr->find_first_of("\\/", 0, 2) == std::string::npos)) {
      if ((itr->compare(0, 3, "BBY") == 0) && (itr->rfind(".hdf") == len - 4))
        hdfFiles++;
      else if (itr->rfind(".bin") == len - 4)
        binFiles++;
      else if (itr->compare(HistoryStr) == 0) {
        if (std::distance(itr, subFiles.end()) != 1)
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

  // create new content
  std::ostringstream logContentNewBuffer;
  int tmp_int;
  double tmp_dbl;
  for (auto itr = std::begin(PatchableProperties); itr != std::end(PatchableProperties); ++itr) {
    auto property_value = getProperty(itr->Name);
    //if (!isEmpty(property_value))
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
    
  std::string logContentNew = logContentNewBuffer.str();
  if (logContentNew.size() == 0)
    throw std::invalid_argument("nothing to patch");
  
  // read existing history
  std::string logContent;
  if (logFiles == 0) {
    logContent = std::move(logContentNew);
  }
  else {
    logContent.resize(logSize);
    tarFile.read(&logContent[0], logSize);
    logContent.append(logContentNew);
  }
  // append patches to file
  tarFile.close();
  if (!ANSTO::Tar::File::append(
    filename,
    HistoryStr,
    logContent.c_str(),
    logContent.size()))
    throw std::runtime_error("unable to patch");
}

} // DataHandling
} // Mantid