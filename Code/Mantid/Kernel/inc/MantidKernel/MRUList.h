#ifndef MANTID_KERNEL_MRULIST_H
#define MANTID_KERNEL_MRULIST_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <fstream>
#include <valarray>

namespace Mantid
{
namespace Kernel
{
  /** An MRU (most recently used) list keeps record of the last n
      inserted items, listing first the newer ones. Care has to be
      taken when a duplicate item is inserted: instead of letting it
      appear twice, the MRU list relocates it to the first position.
      This class has been largely taken from one of the examples given in the
      Boost.MultiIndex documentation (<http://www.boost.org/libs/multi_index/doc/reference/index.html>)

      @author Russell Taylor, Tessella Support Services plc
      @date 27/02/2008

      Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  template<class T>
  class DLLExport MRUList
  {
  private:
    /// hideous typedef for the container holding the list
    typedef typename boost::multi_index::multi_index_container<
      T*,
      boost::multi_index::indexed_by<
        boost::multi_index::sequenced<>,
        boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_CONST_MEM_FUN(T,int,hashIndexFunction)>
      >
    > item_list;

    /// This typedef makes an ordered item list (you access it by the 1st index)
    typedef typename boost::multi_index::nth_index<item_list,1>::type ordered_item_list;

    /// The most recently used list
    mutable item_list il;
    /// The length of the list
    const std::size_t max_num_items;

  public:
    /** Constructor
     *  @param max_num_items_ The length of the list
     */
    MRUList(const std::size_t &max_num_items_)  :
      max_num_items(max_num_items_)
    {}

    /** Destructor
     */
    ~MRUList()
    {
      //std::cout << "MRUList destructor called!\n";
      this->clear();
    }

    /** Insert an item into the list. If it's already in the list, it's moved to the top.
     *  If it's a new item, it's put at the top and the last item in the list is written to file and dropped.
     *  @param item The item, of type T, to put in the list
     *  @return pointer to an item that is being dropped from the MRU. The calling code can
     *     do stuff to it (save it) and needs to delete it. NULL if nothing needs to be dropped.
     */
    T* insert(T* item)
    {
      std::pair<typename MRUList<T>::item_list::iterator,bool> p=this->il.push_front(item);

      if (!p.second)
      { /* duplicate item */
        this->il.relocate(this->il.begin(), p.first); /* put in front */
        return NULL;
      }
      else if (this->il.size()>max_num_items)
      { /* keep the length <= max_num_items */
        // This is dropping an item - you may need to write it to disk (if it's changed) and delete
        // but this is left up to the calling class to do,
        // by returning the to-be-dropped item pointer.
        T *toWrite = this->il.back();
        this->il.pop_back();
        return toWrite;
      }
      return NULL;
    }

    /// Delete all the T's pointed to by the list, and empty the list itself
    void clear()
    {
      for (typename MRUList<T>::item_list::iterator it = this->il.begin(); it != this->il.end(); ++it)
      {
        delete (*it);
      }
      this->il.clear();
    }

    /// Size of the list
    size_t size() const {return il.size();}

    /** Find an element of the list from the key of the index
     *  @param index The index value to search the list for
     *  @return The object found, or NULL if not found.
     */
    T* find(const unsigned int index) const
    {
      using namespace boost::multi_index;
      typename ordered_item_list::const_iterator it = il.get<1>().find(index);
      if (it == il.get<1>().end())
      {
        return NULL;
      }
      else
      {
        return *it;
      }
    }

  private:
    /// Private, unimplemented copy constructor
    MRUList(MRUList&);
    /// Private, unimplemented copy assignment operator
    MRUList& operator=(MRUList&);
  };

} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_MRULIST_H
