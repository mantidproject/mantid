#ifndef ITEM_STRUCT_H
#define ITEM_STRUCT_H

#include <string>
#include <map>

/// structure to hold a dae item
template <typename T>
class item_struct
{
public:
  /// structure to hold a dae item
	struct item_t
	{
		const T* value; ///<array of type T
		bool det_average;	///< can be averaged over detectors via m_spec_array
		const int* dim0; ///< dimension zero array
		const int* dim1; ///< dimension one array
    ///Constructor
		item_t(const T* v, bool da, const int* d0, const int* d1) : value(v), det_average(da), dim0(d0), dim1(d1) {}
	};

private:
	typedef std::map<std::string, item_t> items_map_t;   ///<Type def of internal map of named items
	items_map_t m_items;  ///<internal map of named items
	unsigned long* m_spec_array; ///< length m_ndet; used for averaging values with det_average
	long m_ndet; ///<number of detectors
public:

  /** Adds an item
  @param name :: the item name
  @param value :: the item 
  @param det_average :: Detector average or not
  @param dim0 :: Diemnsion array zero
  @param dim1 :: Diemnsion array one
  @return 0 on success, -1 if it is a duplicate
  */
	int addItem(const std::string& name, const T* value, bool det_average = false, const int* dim0 = NULL, const int* dim1 = NULL)
	{
		std::pair<typename items_map_t::iterator, bool> insert_ret;
		insert_ret = m_items.insert(typename items_map_t::value_type(name, item_t(value, det_average, dim0, dim1)));
		if (!insert_ret.second)
		{
			return -1;   // duplicate
		}
		else
		{
			return 0;
		}
	}

  /** finds an item
  @param item_name :: the item name
  @param det_average :: Detector average or not
  @return The item pointer or NULL
  */
	const item_t* findItem(const std::string& item_name, bool det_average)
	{
		typename items_map_t::const_iterator iter;
		iter = m_items.find(item_name);
		if ( (iter != m_items.end()) && (iter->second.det_average == det_average) )
		{
			return &(iter->second);
		}
		else
		{
			return NULL;
		}
	}

	int getItem(const std::string& item_name, T& value);
	int getItem(const std::string& item_name, long* spec_array, int nspec, T* lVal);
	int getArrayItemSize(const std::string& item_name, int* dims_array, int& ndims);
	int getArrayItem(const std::string& item_name, long* spec_array, int nspec, T* larray);
	int getArrayItem(const std::string& item_name, T* larray);
};

#endif /* ITEM_STRUCT_H */
