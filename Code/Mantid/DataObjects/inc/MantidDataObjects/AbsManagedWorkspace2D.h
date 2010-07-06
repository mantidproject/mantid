#ifndef ABSMANAGEDWORKSPACE2D_H 
#define ABSMANAGEDWORKSPACE2D_H 

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/ManagedDataBlock2D.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidKernel/ConfigService.h"

#include <fstream>
#include <valarray>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Mantid
{
namespace DataObjects
{
  /** AbsManagedWorkspace2D

  This is an abstract class for a managed workspace. 
  Implementations must override init(..) which sets m_vectrosPerBlock and
  readDataBlock(..) and writeDataBlock(..)

  @author Roman Tolchenov, Tessella plc
  @date 28/07/2009

  Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  template<int NBlocks>
  class DLLExport AbsManagedWorkspace2D : public Workspace2D
  {

    /** An MRU (most recently used) list keeps record of the last n
    *  inserted items, listing first the newer ones. Care has to be
    *  taken when a duplicate item is inserted: instead of letting it
    *  appear twice, the MRU list relocates it to the first position.
    *  This class has been taken from one of the examples given in the
    *  Boost.MultiIndex documentation (<http://www.boost.org/libs/multi_index/doc/reference/index.html>)
    */
    class mru_list
    {
    public:
      mru_list(const std::size_t &max_num_items_, AbsManagedWorkspace2D &out);

      void insert(ManagedDataBlock2D* item);
      void clear();
      /// Size of the list
      size_t size() const {return il.size();}

    private:
      /// typedef for the container holding the list
      typedef boost::multi_index::multi_index_container<
        ManagedDataBlock2D*,
        boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ManagedDataBlock2D,int,minIndex)>
        >
      > item_list;

      /// The most recently used list
      item_list il;
      /// The length of the list
      const std::size_t max_num_items;
      /// Reference to the containing class
      AbsManagedWorkspace2D &outer;

    public:
      /// Import the multi index container iterator
      typedef item_list::nth_index<1>::type::const_iterator const_iterator;
      /// An iterator pointing to the beginning of the list sorted by minIndex
      const_iterator begin() const
      {
        return il.get<1>().begin();
      }
      /// An iterator pointing one past the end of the list sorted by minIndex
      const_iterator end() const
      {
        return il.get<1>().end();
      }
      /** Find an element of the list from the key of the minIndex
      *  @param minIndex The minIndex value to search the list for
      */
      const_iterator find(unsigned int minIndex) const
      {
        return il.get<1>().find(minIndex);
      }
    };

    friend class mru_list;

  public:
    AbsManagedWorkspace2D();
    virtual ~AbsManagedWorkspace2D();

    virtual const std::string id() const {return "AbsManagedWorkspace2D";}

    virtual void setX(const int histnumber, const Histogram1D::RCtype&);
    virtual void setX(const int histnumber, const Histogram1D::RCtype::ptr_type&);
    virtual void setData(int const histnumber, const Histogram1D::RCtype&);
    virtual void setData(int const histnumber, const Histogram1D::RCtype&, const Histogram1D::RCtype&);
    virtual void setData(int const histnumber, const Histogram1D::RCtype::ptr_type&, const Histogram1D::RCtype::ptr_type&);

    //section required for iteration
    virtual int size() const;
    virtual int blocksize() const;

    virtual MantidVec& dataX(const int index);
    virtual MantidVec& dataY(const int index);
    virtual MantidVec& dataE(const int index);
    virtual const MantidVec& dataX(int const index) const;
    virtual const MantidVec& dataY(int const index) const;
    virtual const MantidVec& dataE(int const index) const;
    virtual Kernel::cow_ptr<MantidVec> refX(const int index) const;

    /// Returns the size of physical memory the workspace takes
    long int getMemorySize() const = 0;
    virtual bool threadSafe() const { return false; }

  protected:

    virtual void init(const int &NVectors, const int &XLength, const int &YLength);
    /// Number of blocks in temporary storage
    int getNumberBlocks()const{return m_bufferedData.size();}

    /// Reads in a data block.
    virtual void readDataBlock(ManagedDataBlock2D *newBlock,int startIndex)const = 0;
    /// Saves the dropped data block to disk.
    virtual void writeDataBlock(ManagedDataBlock2D *toWrite) = 0;

    /// The number of vectors in each data block
    int m_vectorsPerBlock;
    /// The length of the X vector in each Histogram1D. Must all be the same.
    int m_XLength;
    /// The length of the Y & E vectors in each Histogram1D. Must all be the same.
    int m_YLength;
    /// The size in bytes of each vector
    size_t m_vectorSize;
    /// The size in bytes of one block
    size_t m_blockSize;

  private:
    // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
    /// Private copy constructor
    AbsManagedWorkspace2D(const AbsManagedWorkspace2D&);
    /// Private copy assignment operator
    AbsManagedWorkspace2D& operator=(const AbsManagedWorkspace2D&);

    virtual const int getHistogramNumberHelper() const;

    ManagedDataBlock2D* getDataBlock(const int index) const;

    /// The most-recently-used list of buffered data blocks
    mutable mru_list m_bufferedData;

    /// Static reference to the logger class
    static Kernel::Logger &g_log;
  };

// Get a reference to the logger
template<int NBlocks>
Kernel::Logger& AbsManagedWorkspace2D<NBlocks>::g_log = Kernel::Logger::get("AbsManagedWorkspace2D");


/// Constructor
template<int NBlocks>
AbsManagedWorkspace2D<NBlocks>::AbsManagedWorkspace2D() :
Workspace2D(), m_bufferedData(NBlocks, *this)
{
}

/** Sets the size of the workspace and sets up the temporary file
*  @param NVectors The number of vectors/histograms/detectors in the workspace
*  @param XLength The number of X data points/bin boundaries in each vector (must all be the same)
*  @param YLength The number of data/error points in each vector (must all be the same)
*  @throw std::runtime_error if unable to open a temporary file
*/
template<int NBlocks>
void AbsManagedWorkspace2D<NBlocks>::init(const int &NVectors, const int &XLength, const int &YLength)
{
  m_noVectors = NVectors;
  m_axes.resize(2);
  m_axes[0] = new API::RefAxis(XLength, this);
  m_axes[1] = new API::SpectraAxis(NVectors);
  m_XLength = XLength;
  m_YLength = YLength;

  m_vectorSize = ( m_XLength + ( 2*m_YLength ) ) * sizeof(double);

  // Define m_vectorsPerBlock in the init() of the derived class

}

/// Destructor. Clears the buffer and deletes the temporary file.
template<int NBlocks>
AbsManagedWorkspace2D<NBlocks>::~AbsManagedWorkspace2D()
{
  // delete all ManagedDataBlock2D's
  m_bufferedData.clear();
}

/// Get pseudo size
template<int NBlocks>
int AbsManagedWorkspace2D<NBlocks>::size() const
{
  return m_noVectors * blocksize();
}

/// Get the size of each vector
template<int NBlocks>
int AbsManagedWorkspace2D<NBlocks>::blocksize() const
{
  return (m_noVectors > 0) ? static_cast<int>(m_YLength) : 0;
}

/** Set the x values
*  @param histnumber Index of the histogram to be set
*  @param PA The data to enter
*/
template<int NBlocks>
void AbsManagedWorkspace2D<NBlocks>::setX(const int histnumber, const Histogram1D::RCtype& PA)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setX, histogram number out of range");

  getDataBlock(histnumber)->setX(histnumber, PA);
  return;
}

/** Set the x values
*  @param histnumber Index of the histogram to be set
*  @param Vec The data to enter
*/
template<int NBlocks>
void AbsManagedWorkspace2D<NBlocks>::setX(const int histnumber, const Histogram1D::RCtype::ptr_type& Vec)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setX, histogram number out of range");

  getDataBlock(histnumber)->setX(histnumber, Vec);
  return;
}

/** Set the data values
*  @param histnumber Index of the histogram to be set
*  @param PY The data to enter
*/
template<int NBlocks>
void AbsManagedWorkspace2D<NBlocks>::setData(const int histnumber, const Histogram1D::RCtype& PY)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY);
  return;
}

/** Set the data values
*  @param histnumber Index of the histogram to be set
*  @param PY The data to enter
*  @param PE The corresponding errors
*/
template<int NBlocks>
void AbsManagedWorkspace2D<NBlocks>::setData(const int histnumber, const Histogram1D::RCtype& PY,
                                    const Histogram1D::RCtype& PE)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE);
  return;
}

/** Set the data values
*  @param histnumber Index of the histogram to be set
*  @param PY The data to enter
*  @param PE The corresponding errors
*/
template<int NBlocks>
void AbsManagedWorkspace2D<NBlocks>::setData(const int histnumber, const Histogram1D::RCtype::ptr_type& PY,
                                    const Histogram1D::RCtype::ptr_type& PE)
{
  if ( histnumber<0 || histnumber>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::setData, histogram number out of range");

  getDataBlock(histnumber)->setData(histnumber, PY, PE);
  return;
}

/** Get the x data of a specified histogram
*  @param index The number of the histogram
*  @return A vector of doubles containing the x data
*/
template<int NBlocks>
MantidVec& AbsManagedWorkspace2D<NBlocks>::dataX(const int index)
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataX, histogram number out of range");

  return getDataBlock(index)->dataX(index);
}

/** Get the y data of a specified histogram
*  @param index The number of the histogram
*  @return A vector of doubles containing the y data
*/
template<int NBlocks>
MantidVec& AbsManagedWorkspace2D<NBlocks>::dataY(const int index)
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataY, histogram number out of range");

  return getDataBlock(index)->dataY(index);
}

/** Get the error data of a specified histogram
*  @param index The number of the histogram
*  @return A vector of doubles containing the error data
*/
template<int NBlocks>
MantidVec& AbsManagedWorkspace2D<NBlocks>::dataE(const int index)
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataE, histogram number out of range");

  return getDataBlock(index)->dataE(index);
}

/** Get the x data of a specified histogram
*  @param index The number of the histogram
*  @return A vector of doubles containing the x data
*/
template<int NBlocks>
const MantidVec& AbsManagedWorkspace2D<NBlocks>::dataX(const int index) const
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataX, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataX(index);
}

/** Get the y data of a specified histogram
*  @param index The number of the histogram
*  @return A vector of doubles containing the y data
*/
template<int NBlocks>
const MantidVec& AbsManagedWorkspace2D<NBlocks>::dataY(const int index) const
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataY, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataY(index);
}

/** Get the error data of a specified histogram
*  @param index The number of the histogram
*  @return A vector of doubles containing the error data
*/
template<int NBlocks>
const MantidVec& AbsManagedWorkspace2D<NBlocks>::dataE(const int index) const
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataE, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->dataE(index);
}

template<int NBlocks>
Kernel::cow_ptr<MantidVec> AbsManagedWorkspace2D<NBlocks>::refX(const int index) const
{
  if ( index<0 || index>=m_noVectors )
    throw std::range_error("AbsManagedWorkspace2D::dataX, histogram number out of range");

  return const_cast<const ManagedDataBlock2D*>(getDataBlock(index))->refX(index);
}

/** Returns the number of histograms.
For some reason Visual Studio couldn't deal with the main getHistogramNumber() method
being virtual so it now just calls this private (and virtual) method which does the work.
*/
template<int NBlocks>
const int AbsManagedWorkspace2D<NBlocks>::getHistogramNumberHelper() const
{
  return m_noVectors;
}

/** Get a pointer to the data block containing the data corresponding to a given index
*  @param index The index to search for
*  @return A pointer to the data block containing the index requested
*/
// not really a const method, but need to pretend it is so that const data getters can call it
template<int NBlocks>
ManagedDataBlock2D* AbsManagedWorkspace2D<NBlocks>::getDataBlock(const int index) const
{
  int startIndex = index - ( index%m_vectorsPerBlock );
  // Look to see if the data block is already buffered
  typename mru_list::const_iterator it = m_bufferedData.find(startIndex);
  if ( it != m_bufferedData.end() )
  {
    return *it;
  }

  // If not found, need to load block into memory and mru list
  ManagedDataBlock2D *newBlock = new ManagedDataBlock2D(startIndex, m_vectorsPerBlock, m_XLength, m_YLength);
  // Check whether datablock has previously been saved. If so, read it in.
  readDataBlock(newBlock,startIndex);
  m_bufferedData.insert(newBlock);
  return newBlock;
}

//----------------------------------------------------------------------
// mru_list member function definitions
//----------------------------------------------------------------------

/** Constructor
*  @param max_num_items_ The length of the list
*  @param out A reference to the containing class
*/
template<int NBlocks>
AbsManagedWorkspace2D<NBlocks>::mru_list::mru_list(const std::size_t &max_num_items_, AbsManagedWorkspace2D &out) :
max_num_items(max_num_items_),
outer(out)
{
}

/** Insert an item into the list. If it's already in the list, it's moved to the top.
*  If it's a new item, it's put at the top and the last item in the list is written to file and dropped.
*  @param item The CompressedDataBlock to put in the list
*/
template<int NBlocks>
void AbsManagedWorkspace2D<NBlocks>::mru_list::insert(ManagedDataBlock2D* item)
{
  std::pair<item_list::iterator,bool> p=il.push_front(item);

  if (!p.second)
  { /* duplicate item */
    il.relocate(il.begin(), p.first); /* put in front */
  }
  else if (il.size()>max_num_items)
  { /* keep the length <= max_num_items */
    // This is dropping an item - need to write it to disk (if it's changed) and delete
    ManagedDataBlock2D *toWrite = il.back();
    if ( toWrite->hasChanges() )
    {
      outer.writeDataBlock(toWrite);
    }
    il.pop_back();
    delete toWrite;
  }
}

/// Delete all the data blocks pointed to by the list, and empty the list itself
template<int NBlocks>
void AbsManagedWorkspace2D<NBlocks>::mru_list::clear()
{
  for (item_list::iterator it = il.begin(); it != il.end(); ++it)
  {
    delete *it;
  }
  il.clear();
}



} // namespace DataObjects
} // namespace Mantid

#endif /* ABSMANAGEDWORKSPACE2D_H */
