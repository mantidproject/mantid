#include "MantidDataObjects/PeakColumn.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace DataObjects
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   * @param peaks :: vector of peaks
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

  /** Prints out the column string at the given row index.
   *
   * @param s :: stream to output
   * @param index :: row index
   */
  void PeakColumn::print(std::ostream& s, size_t index) const
  {
    Peak & peak = peaks[index];

    if (m_name == "RunNumber")
      s << peak.getRunNumber();
    else if (m_name == "DetID")
      s << peak.getDetectorID();
    else if (m_name == "h")
      s << peak.getH();
    else if (m_name == "k")
      s << peak.getK();
    else if (m_name == "l")
      s << peak.getL();
    else if (m_name == "Wavelength")
      s << peak.getWavelength();
    else if (m_name == "Energy")
      s << peak.getInitialEnergy();
    else if (m_name == "TOF")
      s << peak.getTOF();
    else if (m_name == "DSpacing")
      s << peak.getDSpacing();
    else if (m_name == "Intens")
      s << peak.getIntensity();
    else if (m_name == "SigInt")
      s << peak.getSigmaIntensity();
    else if (m_name == "BinCount")
      s << peak.getBinCount();
    else if (m_name == "BankName")
      s << peak.getBankName();
    else if (m_name == "Row")
      s << peak.getRow();
    else if (m_name == "Col")
      s << peak.getCol();
    else if (m_name == "QLab")
      s << peak.getQLabFrame();
    else if (m_name == "QSample")
      s << peak.getQSampleFrame();
    else
      throw std::runtime_error("Unexpected column name");
  }

  /// Specialized type check
  bool PeakColumn::isBool()const
  {
    return false;
  }

  /// Must return overall memory size taken by the column.
  long int PeakColumn::sizeOfData()const
  {
    return sizeof(double) * peaks.size();
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

//    Peak & peak = peaks[index];
//
//    if (m_name == "h")
//      return &peak.m_H;
//    else if (m_name == "k")
//      return &peak.m_K;
//    else
//      throw std::runtime_error("Unexpected column name");
  }

} // namespace Mantid
} // namespace DataObjects

