#ifndef MANTID_DATAOBJECTS_PEAKCOLUMN_H_
#define MANTID_DATAOBJECTS_PEAKCOLUMN_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Column.h"
#include "MantidDataObjects/Peak.h"


namespace Mantid
{
namespace DataObjects
{

  /** PeakColumn : a Column sub-class used to display
   * peak information as a TableWorkspace.
   * 
   * The column holds a reference to a vector of Peak objects.
   * Values in the column are taken directly from those Peak objects.
   *
   * @author Janik Zikovsky
   * @date 2011-04-25 18:06:32.952258
   */
  class DLLExport PeakColumn : public Mantid::API::Column
  {

  public:
    PeakColumn(std::vector<Peak> & peaks, std::string name);
    virtual ~PeakColumn();
    

    /// Number of individual elements in the column.
    virtual size_t size() const
    { return peaks.size(); }

    /// Returns typeid for the data in the column
    virtual const std::type_info& get_type_info()const;

    /// Returns typeid for the pointer type to the data element in the column
    virtual const std::type_info& get_pointer_type_info()const;

    /// Prints
    virtual void print(std::ostream& s, size_t index) const;

    /// Specialized type check
    virtual bool isBool()const;

    /// Must return overall memory size taken by the column.
    virtual long int sizeOfData()const;

  protected:
    /// Sets the new column size.
    virtual void resize(size_t count);
    /// Inserts an item.
    virtual void insert(size_t index);
    /// Removes an item.
    virtual void remove(size_t index);
    /// Pointer to a data element
    virtual void* void_pointer(size_t index);

  private:
    /// Reference to the peaks object saved in the PeaksWorkspace.
    std::vector<Peak> & peaks;
  };


} // namespace Mantid
} // namespace DataObjects

#endif  /* MANTID_DATAOBJECTS_PEAKCOLUMN_H_ */
