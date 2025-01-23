// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/PeakColumn.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Strings.h"

#include <boost/variant/get.hpp>

using namespace Mantid::Kernel;

namespace Mantid::DataObjects {
namespace {
/// static logger
Kernel::Logger g_log("PeakColumn");

/// Number of items to keep around in the cell cache (see void_pointer())
size_t NCELL_ITEM_CACHED = 100;

/**
 * Private implementation to wrap a map such that it can be
 * initialized in a thread-safe manner with a local static
 */
class ColumnNameToType {
public:
  ColumnNameToType() {
    // Assume double if not in this map
    m_type_index.emplace("DetID", "int");
    m_type_index.emplace("RunNumber", "int");
    m_type_index.emplace("h", "double");
    m_type_index.emplace("k", "double");
    m_type_index.emplace("l", "double");
    m_type_index.emplace("Wavelength", "double");
    m_type_index.emplace("Energy", "double");
    m_type_index.emplace("TOF", "double");
    m_type_index.emplace("DSpacing", "double");
    m_type_index.emplace("Intens", "double");
    m_type_index.emplace("SigInt", "double");
    m_type_index.emplace("Intens/SigInt", "double");
    m_type_index.emplace("BinCount", "double");
    m_type_index.emplace("BankName", "str");
    m_type_index.emplace("Row", "double");
    m_type_index.emplace("Col", "double");
    m_type_index.emplace("QLab", "V3D");
    m_type_index.emplace("QSample", "V3D");
    m_type_index.emplace("PeakNumber", "int");
    m_type_index.emplace("TBar", "double");
    m_type_index.emplace("IntHKL", "V3D");
    m_type_index.emplace("IntMNP", "V3D");
  }

  inline const auto &data() const { return m_type_index; }

private:
  std::unordered_map<std::string, std::string> m_type_index;
};

/**
 * Returns a string type identifier from the given name
 * @param name :: The name of the column
 * @returns A string identifier for the column type
 */
const std::string typeFromName(const std::string &name) {
  static ColumnNameToType typeIndex;
  auto iter = typeIndex.data().find(name);
  if (iter != typeIndex.data().end()) {
    return iter->second;
  } else {
    throw std::runtime_error("PeakColumn - Unknown column name: \"" + name +
                             "\""
                             "Peak column names/types must be explicitly marked in PeakColumn.cpp");
  }
}
} // namespace

//----------------------------------------------------------------------------------------------
/** Constructor
 * @param peaks :: vector of peaks
 * @param name :: name for the column
 */
template <class T>
PeakColumn<T>::PeakColumn(std::vector<T> &peaks, const std::string &name) : m_peaks(peaks), m_oldRows() {
  this->m_name = name;
  this->m_type = typeFromName(name); // Throws if the name is unknown
  const std::string key = "PeakColumn.hklPrec";
  auto hklPrec = ConfigService::Instance().getValue<int>(key);
  this->m_hklPrec = hklPrec.value_or(2);
  if (!hklPrec.has_value()) {
    g_log.information() << "In PeakColumn constructor, did not find any value for '" << key
                        << "' from the Config Service. Using default: " << this->m_hklPrec << "\n";
  }
}

/// Returns typeid for the data in the column
template <class T> const std::type_info &PeakColumn<T>::get_type_info() const {
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
    throw std::runtime_error("PeakColumn::get_type_info() - Unknown column type: " + m_name);
  }
}

/// Returns typeid for the pointer type to the data element in the column
template <class T> const std::type_info &PeakColumn<T>::get_pointer_type_info() const {
  if (type() == "double") {
    return typeid(double *);
  } else if (type() == "int") {
    return typeid(int *);
  } else if (type() == "str") {
    return typeid(std::string *);
  } else if (type() == "V3D") {
    return typeid(V3D *);
  } else {
    throw std::runtime_error("PeakColumn::get_pointer_type_info() -: " + m_name);
  }
}

//-------------------------------------------------------------------------------------
/** Prints out the column string at the given row index.
 *
 * @param s :: stream to output
 * @param index :: row index
 */
template <class T> void PeakColumn<T>::print(size_t index, std::ostream &s) const {
  T &peak = m_peaks[index];
  auto fullPeak = dynamic_cast<Peak *>(&peak); // additional printout for class Peak

  s.imbue(std::locale("C"));
  std::ios::fmtflags fflags(s.flags());
  if (m_name == "RunNumber")
    s << peak.getRunNumber();
  else if (m_name == "DetID" && fullPeak) {
    s << fullPeak->getDetectorID();
  } else if (m_name == "BankName" && fullPeak)
    s << fullPeak->getBankName();
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
  } else if (m_name == "IntHKL") {
    s << peak.getIntHKL();
  } else if (m_name == "IntMNP") {
    s << peak.getIntMNP();
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
template <class T> void PeakColumn<T>::read(size_t index, const std::string &text) {
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
template <class T> void PeakColumn<T>::read(const size_t index, std::istringstream &in) {
  if (this->getReadOnly() || index >= m_peaks.size())
    return;

  double val;
  try {
    in >> val;
  } catch (std::exception &e) {
    g_log.error() << "Could not convert input to a number. " << e.what() << '\n';
    return;
  }

  setPeakHKLOrRunNumber(index, val);
}

//-------------------------------------------------------------------------------------
/** @return true if the column is read-only */
template <class T> bool PeakColumn<T>::getReadOnly() const {
  return !((m_name == "h") || (m_name == "k") || (m_name == "l") || (m_name == "RunNumber"));
}

//-------------------------------------------------------------------------------------
/// Specialized type check
template <class T> bool PeakColumn<T>::isBool() const { return false; }

template <class T> bool PeakColumn<T>::isNumber() const { return false; }

/// @returns overall memory size taken by the column.
template <class T> long int PeakColumn<T>::sizeOfData() const {
  return sizeof(double) * static_cast<long int>(m_peaks.size());
}

/**
 * Sets a new size for the column. Not implemented as this is controlled
 * by the PeaksWorkspace
 * @param count :: Count of new column size (unused)
 * @throw Exception::NotImplementedError
 */
template <class T> void PeakColumn<T>::resize(size_t count) {
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
template <class T> void PeakColumn<T>::insert(size_t index) {
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
template <class T> void PeakColumn<T>::remove(size_t index) {
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
template <class T> void *PeakColumn<T>::void_pointer(size_t index) {
  const auto *constThis = const_cast<const PeakColumn *>(this);
  return const_cast<void *>(constThis->void_pointer(index));
}

/**
 * Pointer to a data element in the PeaksWorkspace (const version)
 * @param index :: A row index pointing to the PeaksWorkspace
 * @returns A pointer to the data element at that index from this column
 */
template <class T> const void *PeakColumn<T>::void_pointer(size_t index) const {
  const T &peak = m_peaks[index];
  auto fullPeak = dynamic_cast<const DataObjects::Peak *>(&peak); // additional methods available in class Peak

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
    return boost::get<double>(&value);      // Given a pointer it will return a pointer
  } else if (m_name == "RunNumber") {
    value = peak.getRunNumber();
    return boost::get<int>(&value);
  } else if (m_name == "PeakNumber") {
    value = peak.getPeakNumber();
    return boost::get<int>(&value);
  } else if (m_name == "DetID" && fullPeak) {
    value = fullPeak->getDetectorID();
    return boost::get<int>(&value);
  } else if (m_name == "BankName" && fullPeak) {
    value = fullPeak->getBankName();
    return boost::get<std::string>(&value);
  } else if (m_name == "QLab") {
    value = peak.getQLabFrame();
    return boost::get<Kernel::V3D>(&value);
  } else if (m_name == "QSample") {
    value = peak.getQSampleFrame();
    return boost::get<Kernel::V3D>(&value);
  } else if (m_name == "IntHKL") {
    value = peak.getIntHKL();
    return boost::get<Kernel::V3D>(&value);
  } else if (m_name == "IntMNP") {
    value = peak.getIntMNP();
    return boost::get<Kernel::V3D>(&value);
  } else {
    throw std::runtime_error("void_pointer() - Unknown peak column name or type: " + m_name);
  }
}

template <class T> PeakColumn<T> *PeakColumn<T>::clone() const {
  auto temp = new PeakColumn<T>(this->m_peaks, this->m_name);
  return temp;
}

template <class T> double PeakColumn<T>::toDouble(size_t /*index*/) const {
  throw std::runtime_error("PeakColumn::toDouble() not implemented, PeakColumn "
                           "is has no general write access");
}

template <class T> void PeakColumn<T>::fromDouble(size_t /*index*/, double /*value*/) {
  throw std::runtime_error("fromDouble() not implemented, PeakColumn is has no "
                           "general write access");
}

template <class T> void PeakColumn<T>::setPeakHKLOrRunNumber(const size_t index, const double val) {
  T &peak = m_peaks[index];
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

template class PeakColumn<Peak>;
template class PeakColumn<LeanElasticPeak>;

} // namespace Mantid::DataObjects
