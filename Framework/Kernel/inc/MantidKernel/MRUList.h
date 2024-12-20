// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <mutex>

#ifndef Q_MOC_RUN
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#endif

namespace Mantid {
namespace Kernel {
/** An MRU (most recently used) list keeps record of the last n
    inserted items, listing first the newer ones. Care has to be
    taken when a duplicate item is inserted: instead of letting it
    appear twice, the MRU list relocates it to the first position.
    This class has been largely taken from one of the examples given in the
    Boost.MultiIndex documentation
   (<http://www.boost.org/libs/multi_index/doc/reference/index.html>)
 */
template <class T> class MANTID_KERNEL_DLL MRUList {
private:
  /// hideous typedef for the container holding the list
  using item_list = typename boost::multi_index::multi_index_container<
      std::shared_ptr<T>,
      boost::multi_index::indexed_by<boost::multi_index::sequenced<>,
                                     boost::multi_index::hashed_unique<::boost::multi_index::const_mem_fun<
                                         T, std::uintptr_t, &T::hashIndexFunction>>>>;

  /// This typedef makes an ordered item list (you access it by the 1st index)
  using ordered_item_list = typename boost::multi_index::nth_index<item_list, 1>::type;

  /// The most recently used list
  mutable item_list il;
  /// The length of the list
  const std::size_t max_num_items;

public:
  //---------------------------------------------------------------------------------------------
  /** Constructor
   *  @param max_num_items_ :: The length of the list
   */
  MRUList(const std::size_t &max_num_items_) : max_num_items(max_num_items_) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor. Default to 100 items.
   */
  MRUList() : max_num_items(100) {}

  //---------------------------------------------------------------------------------------------
  /** Destructor
   */
  ~MRUList() { this->clear(); }

  //---------------------------------------------------------------------------------------------
  /** Insert an item into the list. If it's already in the list, it's moved to
   *the top.
   *  If it's a new item, it's put at the top and the last item in the list is
   *written to file and dropped.
   *
   *  @param item :: The item, of type T, to put in the list
   *  @return pointer to an item that is being dropped from the MRU. The calling
   *code can
   *     do stuff to it (save it) and needs to delete it. NULL if nothing needs
   *to be dropped.
   */
  std::shared_ptr<T> insert(std::shared_ptr<T> item) {
    std::lock_guard<std::mutex> _lock(m_mutex);
    auto p = this->il.push_front(std::move(item));

    if (!p.second) {
      /* duplicate item */
      this->il.relocate(this->il.begin(), p.first); /* put in front */
      return nullptr;
    }

    bool exceeding_size;
    exceeding_size = this->il.size() > max_num_items;

    if (exceeding_size) {
      std::shared_ptr<T> toWrite;
      /* keep the length <= max_num_items */
      // This is dropping an item - you may need to write it to disk (if it's
      // changed) and delete
      // but this is left up to the calling class to do,
      // by returning the to-be-dropped item pointer.
      toWrite = std::move(this->il.back());
      this->il.pop_back();
      return toWrite;
    }
    return nullptr;
  }

  //---------------------------------------------------------------------------------------------
  /// Delete all the T's pointed to by the list, and empty the list itself
  void clear() {
    std::lock_guard<std::mutex> _lock(m_mutex);
    this->il.clear();
  }

  //---------------------------------------------------------------------------------------------
  /** Delete the T at the given index. Will also delete the object itself.
   *  @param index :: the key (index) for this T that you want to remove from
   * the MRU.
   */
  void deleteIndex(const uintptr_t index) {
    std::lock_guard<std::mutex> _lock(m_mutex);

    auto it = il.template get<1>().find(index);
    if (it != il.template get<1>().end()) {
      il.template get<1>().erase(it);
    }
  }

  //---------------------------------------------------------------------------------------------
  /// Size of the list
  size_t size() const { return il.size(); }

  //---------------------------------------------------------------------------------------------
  /** Find an element of the list from the key of the index
   *  @param index :: The index value to search the list for
   *  @return The object found, or NULL if not found.
   */
  T *find(const uintptr_t index) const {
    std::lock_guard<std::mutex> _lock(m_mutex);

    auto it = il.template get<1>().find(index);
    if (it == il.template get<1>().end()) {
      return nullptr;
    } else {
      return it->get();
    }
  }

private:
  /// Private, unimplemented copy constructor
  MRUList(MRUList &);
  /// Private, unimplemented copy assignment operator
  MRUList &operator=(MRUList &);

  /// Mutex for modifying the MRU list
  mutable std::mutex m_mutex;
};

} // namespace Kernel
} // namespace Mantid
