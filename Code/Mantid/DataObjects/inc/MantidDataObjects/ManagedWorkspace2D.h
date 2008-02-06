#ifndef MANTID_DATAOBJECTS_MANAGEDWORKSPACE2D_H_
#define MANTID_DATAOBJECTS_MANAGEDWORKSPACE2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "Workspace2D.h"
#include "ManagedDataBlock2D.h"
#include <fstream>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
//#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

using namespace boost::multi_index;

namespace Mantid
{
namespace DataObjects
{
/** The ManagedWorkspace2D allows the framework to handle 2D datasets that are too
    large to fit in the available system memory by making use of a temporary file. 
    It is a specialisation of Workspace2D.
    
    The optional configuration property ManagedWorkspace.DataBlockSize sets the size
    (in bytes) of the blocks used to internally buffer data. The default is 1MB.

    @author Russell Taylor, Tessella Support Services plc
    @date 22/01/2008
    
    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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
class DLLExport ManagedWorkspace2D : public Workspace2D
{

  /** An MRU (most recently used) list keeps record of the last n
   *  inserted items, listing first the newer ones. Care has to be
   *  taken when a duplicate item is inserted: instead of letting it
   *  appear twice, the MRU list relocates it to the first position.
   *  This class has been taken one of the examples given in the
   *  Boost.MultiIndex documentation (<http://www.boost.org/libs/multi_index/doc/reference/index.html>)
   */
  class mru_list
  {    
  public:
    mru_list(const std::size_t &max_num_items_, ManagedWorkspace2D &out);

    void insert(ManagedDataBlock2D* item);
    void clear();

  private:
    /// typedef for the container holding the list
    typedef multi_index_container<
      ManagedDataBlock2D*,
      indexed_by<
        sequenced<>,
        hashed_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ManagedDataBlock2D,unsigned int,minIndex)>
//        ordered_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(ManagedDataBlock2D,int,minIndex)>
      >
    > item_list;
    
    /// The most recently used list
    item_list il;
    /// The length of the list
    const std::size_t max_num_items;
    /// Reference to the containing class
    ManagedWorkspace2D &outer;
    
  public:
    /// Import the multi index container iterator
    typedef item_list::const_iterator const_iterator;
    /// An iterator pointing to the beginning of the list
    const_iterator begin() const
    {
      return il.begin();
    }
    /// An iterator pointing one past the end of the list
    const_iterator end() const
    {
      return il.end();
    } 
  };

  friend class mru_list;
  
public:
	ManagedWorkspace2D();
	virtual void init(const unsigned int &NVectors, const unsigned int &XLength, const unsigned int &YLength);
	virtual ~ManagedWorkspace2D();

  virtual const std::string id() const {return "ManagedWorkspace2D";}
	
  /// Returns the histogram number
  virtual const int getHistogramNumber() const;
  virtual void setHistogramNumber(int const) {}  // Does nothing

  virtual void setX(const int histnumber, const std::vector<double>&);
  virtual void setX(const int histnumber, const Histogram1D::RCtype&);
  virtual void setX(const int histnumber, const Histogram1D::RCtype::ptr_type&);
  virtual void setData(const int histnumber, const std::vector<double>&);
  virtual void setData(const int histnumber, const std::vector<double>&, const std::vector<double>&);
  virtual void setData(const int histnumber, const std::vector<double>&, const std::vector<double>&,
                       const std::vector<double>&);
  virtual void setData(int const histnumber, const Histogram1D::RCtype&);
  virtual void setData(int const histnumber, const Histogram1D::RCtype&, const Histogram1D::RCtype&);
  virtual void setData(int const histnumber, const Histogram1D::RCtype&, const Histogram1D::RCtype&, 
                       const Histogram1D::RCtype&);
  virtual void setData(int const histnumber, const Histogram1D::RCtype::ptr_type&, const Histogram1D::RCtype::ptr_type&);
  virtual void setData(int const histnumber, const Histogram1D::RCtype::ptr_type&, const Histogram1D::RCtype::ptr_type&,
                       const Histogram1D::RCtype::ptr_type&);  

  //section required for iteration
  virtual int size() const;
  virtual int blocksize() const;

  virtual std::vector<double>& dataX(const int index);
  virtual std::vector<double>& dataY(const int index);
  virtual std::vector<double>& dataE(const int index);
  virtual std::vector<double>& dataE2(const int index);
  virtual const std::vector<double>& dataX(int const index) const;
  virtual const std::vector<double>& dataY(int const index) const;
  virtual const std::vector<double>& dataE(int const index) const;
  virtual const std::vector<double>& dataE2(int const index) const;
	
private:
  // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
  /// Private copy constructor
  ManagedWorkspace2D(const ManagedWorkspace2D&);
  /// Private copy assignment operator
  ManagedWorkspace2D& operator=(const ManagedWorkspace2D&);
    
  ManagedDataBlock2D* getDataBlock(const int index) const;
  
  /// The number of vectors in the workspace
  unsigned int m_noVectors;
  /// The number of vectors in each data block
  unsigned int m_vectorsPerBlock;
  /// The length of the X vector in each Histogram1D. Must all be the same. 
  unsigned int m_XLength;
  /// The length of the Y & E vectors in each Histogram1D. Must all be the same. 
  unsigned int m_YLength;

  /// The most-recently-used list of buffered data blocks
  mutable mru_list m_bufferedData;

  /// The name of the temporary file
  std::string m_filename;
  /// The stream handle to the temporary file used to store the data
  mutable std::fstream m_datafile;

  /// Static instance count. Used to ensure temporary filenames are distinct.
  static int g_uniqueID;
  
  /// Static reference to the logger class
  static Kernel::Logger &g_log;
};

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_MANAGEDWORKSPACE2D_H_*/
