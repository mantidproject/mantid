#include "MantidDataObjects/PeakColumn.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"

#include <boost/variant/get.hpp>

using namespace Mantid::Kernel;

namespace Mantid {
namespace DataObjects {
namespace {
/// static logger
Kernel::Logger g_log("PeakColumn");

/// Number of items to keep around in the cell cache (see void_pointer())
size_t NCELL_ITEM_CACHED = 100;
/// Type lookup: key=name,value=type. Moved here from static inside typeFromName
/// to avoid the need for locks with the initialisation problem across multiple
/// threads
std::map<std::string, std::string> TYPE_INDEX;

/**
 * Returns a string type identifier from the given name
 * @param name :: The name of the column
 * @returns A string identifier for the column type
 */
const std::string typeFromName(const std::string &name) {
  // We should enter the critical section if the map has not been fully filled.
  // Be sure to keep the value tested against in sync with the number of inserts
  // below
  if (TYPE_INDEX.size() != 18) {
    PARALLEL_CRITICAL(fill_column_index_map) {
      if (TYPE_INDEX.empty()) // check again inside the critical block
      {
        // Assume double if not in this map
        TYPE_INDEX.emplace("DetID", "int");
        TYPE_INDEX.emplace("RunNumber", "int");
        TYPE_INDEX.emplace("h", "double");
        TYPE_INDEX.emplace("k", "double");
        TYPE_INDEX.emplace("l", "double");
        TYPE_INDEX.emplace("Wavelength", "double");
        TYPE_INDEX.emplace("Energy", "double");
        TYPE_INDEX.emplace("TOF", "double");
        TYPE_INDEX.emplace("DSpacing", "double");
        TYPE_INDEX.emplace("Intens", "double");
        TYPE_INDEX.emplace("SigInt", "double");
        TYPE_INDEX.emplace("BinCount", "double");
        TYPE_INDEX.emplace("BankName", "str");
        TYPE_INDEX.emplace("Row", "double");
        TYPE_INDEX.emplace("Col", "double");
        TYPE_INDEX.emplace("QLab", "V3D");
        TYPE_INDEX.emplace("QSample", "V3D");
        TYPE_INDEX.emplace("PeakNumber", "int");
        // If adding an entry, be sure to increment the size comparizon in the
        // first line
      }
    }
  }
  auto iter = TYPE_INDEX.find(name);
  if (iter != TYPE_INDEX.end()) {
    return iter->second;
  } else {
    throw std::runtime_error(
        "PeakColumn - Unknown column name: \"" + name +
        "\""
        "Peak column names/types must be explicitly marked in PeakColumn.cpp");
  }
}
}

//----------------------------------------------------------------------------------------------
/** Constructor
 * @param peaks :: vector of peaks
 * @param name :: name for the column
 */
PeakColumn::PeakColumn(std::vector<Peak> &peaks, const std::string &name)
    : m_peaks(peaks), m_oldRows() {
  this->m_name = name;
  this->m_type = typeFromName(name); // Throws if the name is unknown
  this->m_hklPrec = 2;
  const std::string key = "PeakColumn.hklPrec";
  int gotit = ConfigService::Instance().getValue(key, this->m_hklPrec);
  if (!gotit)
    g_log.information()
        << "In PeakColumn constructor, did not find any value for '" << key
        << "' from the Config Service. Using default: " << this->m_hklPrec
        << "\n";
}

/// Returns typeid for the data in the column
const std::type_info &PeakColumn::get_type_info() const {
  // This is horrible copy-and-paste with the method below. The whole thing
  // around columns could be much better implemented using templates & traits to
  // avoid this
  // type of thing!

  if (type() == "double") {
    return typeid(double);
  } else if (type() == "int") {
    return typeid(int);
  } else if (type() == "str") {
    return typeid(std::string);
  } else if (type() == "V3D") {
    return typeid(V3D);
  } else {
    throw std::runtime_error(
        "PeakColumn::get_type_info() - Unknown column type: " + m_name);
  }
}

/// Returns typeid for the pointer type to the data element in the column
const std::type_info &PeakColumn::get_pointer_type_info() const {
  if (type() == "double") {
    return typeid(double *);
  } else if (type() == "int") {
    return typeid(int *);
  } else if (type() == "str") {
    return typeid(std::string *);
  } else if (type() == "V3D") {
    return typeid(V3D *);
  } else {
    throw std::runtime_error("PeakColumn::get_pointer_type_info() -: " +
                             m_name);
  }
}

//-------------------------------------------------------------------------------------
/** Prints out the column string at the given row index.
 *
 * @param s :: stream to output
 * @param index :: row index
 */
void PeakColumn::print(size_t index, std::ostream &s) const {
  Peak &peak = m_peaks[index];
  s.imbue(std::locale("C"));
  std::ios::fmtflags fflags(s.flags());
  if (m_name == "RunNumber")
    s << peak.getRunNumber();
  else if (m_name == "DetID")
    s << peak.getDetectorID();
  else if (m_name == "BankName")
    s << peak.getBankName();
  else if (m_name == "QLab")
    s << peak.getQLabFrame();
  else if (m_name == "QSample")
    s << peak.getQSampleFrame();
  else if (m_name == "h") {
    s << std::fixed << std::setprecision(m_hklPrec) << peak.getH();
  } else if (m_name == "k") {
    s << std::fixed << std::setprecision(m_hklPrec) << peak.getK();
  } else if (m_name == "l") {
    s << std::fixed << std::setprecision(m_hklPrec) << peak.getL();
  } else if (m_name == "PeakNumber") {
    s << peak.getPeakNumber();
  } else
    s << peak.getValueByColName(m_name);
  s.flags(fflags);
}

//-------------------------------------------------------------------------------------
/** Read in some text and convert to a number in the PeaksWorkspace
 *
 * @param text :: string to read
 * @param index :: index of the peak to modify
 */
void PeakColumn::read(size_t index, const std::string &text) {
  // Don't modify read-only ones
  if (this->getReadOnly() || index >= m_peaks.size())
    return;

  // Convert to a double
  double val = 0;
  int success = Strings::convert(text, val);

  if (success == 0) {
    g_log.error() << "Could not convert string '" << text << "' to a number.\n";
    return;
  }
  setPeakHKLOrRunNumber(index, val);
}

/** Read in from stream and convert to a number in the PeaksWorkspace
 *
 * @param index :: index of the peak to modify
 * @param in :: input stream
 */
void PeakColumn::read(const size_t index, std::istringstream &in) {
  if (this->getReadOnly() || index >= m_peaks.size())
    return;

  double val;
  try {
    in >> val;
  } catch (std::exception &e) {
    g_log.error() << "Could not convert input to a number. " << e.what()
                  << '\n';
    return;
  }

  setPeakHKLOrRunNumber(index, val);
}

//-------------------------------------------------------------------------------------
/** @return true if the column is read-only */
bool PeakColumn::getReadOnly() const {
  return !((m_name == "h") || (m_name == "k") || (m_name == "l") ||
           (m_name == "RunNumber"));
}

//-------------------------------------------------------------------------------------
/// Specialized type check
bool PeakColumn::isBool() const { return false; }

bool PeakColumn::isNumber() const { return false; }

/// @returns overall memory size taken by the column.
long int PeakColumn::sizeOfData() const {
  return sizeof(double) * static_cast<long int>(m_peaks.size());
}

/**
 * Sets a new size for the column. Not implemented as this is controlled
 * by the PeaksWorkspace
 * @param count :: Count of new column size (unused)
 * @throw Exception::NotImplementedError
 */
void PeakColumn::resize(size_t count) {
  UNUSED_ARG(count);
  throw Exception::NotImplementedError("PeakColumn::resize - Peaks must be "
                                       "added through the PeaksWorkspace "
                                       "interface.");
}

/**
 * Inserts an item into the column. Not implemented as this is controlled by the
 * PeaksWorkspace
 * @param index :: The new index position (unused)
 * @throw Exception::NotImplementedError
 */
void PeakColumn::insert(size_t index) {
  UNUSED_ARG(index);
  throw Exception::NotImplementedError("PeakColumn::insert - Peaks must be "
                                       "inserted through the PeaksWorkspace "
                                       "interface.");
}

/**
 * Removes an item from the column. Not implemented as this is controlled by the
 * PeaksWorkspace
 * @param index :: The index position removed(unused)
 * @throw Exception::NotImplementedError
 */
void PeakColumn::remove(size_t index) {
  UNUSED_ARG(index);
  throw Exception::NotImplementedError("PeakColumn::remove - Peaks must be "
                                       "remove through the PeaksWorkspace "
                                       "interface.");
}

/**
 * Pointer to a data element in the PeaksWorkspace (non-const version)
 * @param index :: A row index pointing to the PeaksWorkspace
 * @returns A pointer to the data element at that index from this column
 */
void *PeakColumn::void_pointer(size_t index) {
  const PeakColumn *constThis = const_cast<const PeakColumn *>(this);
  return const_cast<void *>(constThis->void_pointer(index));
}

/**
 * Pointer to a data element in the PeaksWorkspace (const version)
 * @param index :: A row index pointing to the PeaksWorkspace
 * @returns A pointer to the data element at that index from this column
 */
const void *PeakColumn::void_pointer(size_t index) const {
  const Peak &peak = m_peaks[index];

  // The cell() api requires that the value exist somewhere in memory, however,
  // some of the values from a Peak are calculated on the fly so a reference
  // cannot be returned. Instead we cache a value for the last NCELL_ITEM_CACHED
  // accesses and return a reference to this

  m_oldRows.push_front(CacheValueType());
  if (m_oldRows.size() > NCELL_ITEM_CACHED) {
    m_oldRows.pop_back();
  }
  auto &value = m_oldRows.front(); // A reference to the actual stored variant

  if (type() == "double") {
    value = peak.getValueByColName(m_name); // Assign the value to the store
    return boost::get<double>(
        &value); // Given a pointer it will return a pointer
  } else if (m_name == "RunNumber") {
    value = peak.getRunNumber();
    return boost::get<int>(&value);
  } else if (m_name == "PeakNumber") {
    value = peak.getPeakNumber();
    return boost::get<int>(&value);
  } else if (m_name == "DetID") {
    value = peak.getDetectorID();
    return boost::get<int>(&value);
  } else if (m_name == "BankName") {
    value = peak.getBankName();
    return boost::get<std::string>(&value);
  } else if (m_name == "QLab") {
    value = peak.getQLabFrame();
    return boost::get<Kernel::V3D>(&value);
  } else if (m_name == "QSample") {
    value = peak.getQSampleFrame();
    return boost::get<Kernel::V3D>(&value);
  } else {
    throw std::runtime_error(
        "void_pointer() - Unknown peak column name or type: " + m_name);
  }
}

PeakColumn *PeakColumn::clone() const {
  auto temp = new PeakColumn(this->m_peaks, this->m_name);
  return temp;
}

double PeakColumn::toDouble(size_t /*index*/) const {
  throw std::runtime_error("PeakColumn::toDouble() not implemented, PeakColumn "
                           "is has no general write access");
}

void PeakColumn::fromDouble(size_t /*index*/, double /*value*/) {
  throw std::runtime_error("fromDouble() not implemented, PeakColumn is has no "
                           "general write access");
}

void PeakColumn::setPeakHKLOrRunNumber(const size_t index, const double val) {
  Peak &peak = m_peaks[index];
  if (m_name == "h")
    peak.setH(val);
  else if (m_name == "k")
    peak.setK(val);
  else if (m_name == "l")
    peak.setL(val);
  else if (m_name == "RunNumber")
    peak.setRunNumber(static_cast<int>(val));
  else
    throw std::runtime_error("Unexpected column " + m_name + " being set.");
}

} // namespace Mantid
} // namespace DataObjects
