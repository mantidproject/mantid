#include "MantidDataObjects/PeakColumn.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Strings.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace DataObjects
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   * @param peaks :: vector of peaks
   * @param name :: name for the column
   */
  PeakColumn::PeakColumn(std::vector<Peak> & peaks, std::string name) :
      peaks(peaks)
  {
    setName(name);
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PeakColumn::~PeakColumn()
  {
  }


  /// Returns typeid for the data in the column
  const std::type_info& PeakColumn::get_type_info()const
  {
    return typeid(double);
  }

  /// Returns typeid for the pointer type to the data element in the column
  const std::type_info& PeakColumn::get_pointer_type_info()const
  {
    return typeid(double*);
  }

  //-------------------------------------------------------------------------------------
  /** Prints out the column string at the given row index.
   *
   * @param s :: stream to output
   * @param index :: row index
   */
  void PeakColumn::print(size_t index, std::ostream& s) const
  {
    Peak & peak = peaks[index];

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
    else
      s << peak.getValueByColName(m_name);
  }

  //-------------------------------------------------------------------------------------
  /** Read in some text and convert to a number in the PeaksWorkspace
   *
   * @param text :: string to read
   * @param index :: index of the peak to modify
   */
  void PeakColumn::read(size_t index, const std::string & text)
  {
    // Don't modify read-only ones
    if (this->getReadOnly())
      return;

    // Avoid going out of bounds
    if (size_t(index) >= peaks.size())
      return;

    // Reference to the peak in the workspace
    Peak & peak = peaks[index];

    // Convert to a double
    double val = 0;
    int success = Strings::convert(text, val);
    int ival = static_cast<int>(val);

    if (success == 0)
    {
      g_log.error() << "Could not convert string '" << text << "' to a number.\n";
      return;
    }

    if      (m_name == "h")
      peak.setH(val);
    else if (m_name == "k")
      peak.setK(val);
    else if (m_name == "l")
      peak.setL(val);
    else if (m_name == "RunNumber")
      peak.setRunNumber(ival);
    else
      throw std::runtime_error("Unexpected column " + m_name + " being set.");
  }


  //-------------------------------------------------------------------------------------
  /** @return true if the column is read-only */
  bool PeakColumn::getReadOnly() const
  {
    if (
        (m_name == "h") || (m_name == "k") || (m_name == "l") ||
        (m_name == "RunNumber")
      )
      return false;
    else
      // Default to true for most columns
      return true;
  }


  //-------------------------------------------------------------------------------------
  /// Specialized type check
  bool PeakColumn::isBool()const
  {
    return false;
  }

  /// Must return overall memory size taken by the column.
  long int PeakColumn::sizeOfData()const
  {
    return sizeof(double) * static_cast<long int>(peaks.size());
  }


  /// Sets the new column size.
  void PeakColumn::resize(size_t /*count*/)
  {
    throw std::runtime_error("Not implemented.");
  }

  /// Inserts an item.
  void PeakColumn::insert(size_t /*index*/)
  {
    throw std::runtime_error("Not implemented.");
  }

  /// Removes an item.
  void PeakColumn::remove(size_t /*index*/)
  {
    throw std::runtime_error("Not implemented.");
  }

  /// Pointer to a data element
  void* PeakColumn::void_pointer(size_t /*index*/)
  {
    throw std::runtime_error("void_pointer() not implemented. Looks to be unused?");
  }

  /// Pointer to a data element
  const void* PeakColumn::void_pointer(size_t /*index*/) const
  {
    throw std::runtime_error("const version of void_pointer() not implemented. Looks to be unused?");
  }

  PeakColumn* PeakColumn::clone() const
  {
    PeakColumn* temp = new PeakColumn(this->peaks, this->m_name);
    return temp;
  }

  double PeakColumn::toDouble(size_t /*index*/)const
  {
    throw std::runtime_error("toDouble() not implemented.");
  }

  void PeakColumn::fromDouble(size_t /*index*/, double /*value*/)
  {
    throw std::runtime_error("fromDouble() not implemented.");
  }

} // namespace Mantid
} // namespace DataObjects

