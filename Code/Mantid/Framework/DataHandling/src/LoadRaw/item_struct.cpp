#include "item_struct.h"
#include <stdlib.h>

#define FAILURE 1
#define SUCCESS 0

/** Gets an item
@param item_name :: the item name
@param value :: A pointer to the item
@return 0 on success
*/
template <typename T>
int item_struct<T>::getItem(const std::string& item_name, T& value)
{
	int n;
	long spec_no;
	// handle case of item_spectrum which means we average the array
	// item[ndet] for that particular spectrum
	n = item_name.find('_');
	if (n != std::string::npos)
	{
		spec_no = atol(item_name.c_str()+n+1);
		return getItem(item_name.substr(0,n), &spec_no, 1, &value);
	}
	else
	{
		spec_no = 0;
		return getItem(item_name, &spec_no, 0, &value);
	}
}


/** Gets an item
nspec number of 0 means no spec average
@param item_name :: the item name
@param spec_array :: The array of spectra numbers
@param nspec :: the number of spectra in the array
@param lVal :: A pointer to the item
@return 0 on success
*/
template <typename T>
int item_struct<T>::getItem(const std::string& item_name, long* spec_array, int nspec, T* lVal)
{
	int i, j, n;
	std::string sitem_name;
	std::string tmp_item;
	const T* pVal = NULL;
	const item_t* item;
	item = findItem(item_name, false);
	if (item != NULL)
	{
		for(j=0; j < (nspec == 0 ? 1 : nspec); j++)
		{
			lVal[j] = *(item->value);
		}
		return SUCCESS;
	}
	if (nspec == 0)
	{
		return FAILURE;
	}
	item = findItem(item_name, true);
	if (item != NULL)
	{
			if (item->dim1 == 0)
			{
				n = *(item->dim0);
			}
			else
			{
				n = *(item->dim0) * *(item->dim1);
			}
			if (n == m_ndet)
			{
				pVal = item->value;
			}
	}
	if (pVal == NULL)
	{
		return FAILURE;
	}
	for(j=0; j<nspec; j++)
	{
		lVal[j] = 0;
		n = 0;
		for(i=0; i<m_ndet; i++)
		{
			if (m_spec_array[i] == spec_array[j])
			{
				lVal[j] += pVal[i];
				n++;
			}
		}
		if (n > 0)
		{
			lVal[j] = lVal[j] / n;
		}
	}
	return SUCCESS;
}

/** Gets the size of an array of items
@param item_name :: the item name
@param dims_array :: The array of dimensions
@param ndims :: the number of dimensions in the array
@return 0 on success
*/
template <typename T>
int item_struct<T>::getArrayItemSize(const std::string& item_name, int* dims_array, int& ndims)
{
	const item_t* item;
	item = findItem(item_name, false);
	if (item == NULL)
	{
		item = findItem(item_name, true);
	}
	if (item != NULL)
	{
		if (item->dim1 == 0)
		{
			dims_array[0] = *(item->dim0);
			ndims = 1;
		}
		else
		{
			dims_array[0] = *(item->dim0);
			dims_array[1] = *(item->dim1);
			ndims = 2;
		}
		return SUCCESS;
	}
	return FAILURE;
}

/** Gets an array of items
@param item_name :: the item name
@param spec_array :: The array of spectra numbers
@param nspec :: the number of spectra in the array
@param larray :: The returned array
@return 0 on success
*/
template <typename T>
int item_struct<T>::getArrayItem(const std::string& item_name, long* spec_array, int nspec, T* larray)
{
	int i, j, k, n;
	const item_t* item;
	item = findItem(item_name, false);
	if (item == NULL)
	{
		item = findItem(item_name, true);
	}
	if (item != NULL)
	{
		if (item->dim1 == 0)
		{
			n = *(item->dim0);
		}
		else
		{
			n = *(item->dim0) * *(item->dim1);
		}
		for(k=0; k<nspec; k++)
		{
			for(j=0; j<n; j++)
			{
				larray[j + k * n] = item->value[j];
			}
		}
		return SUCCESS;
	}
	return FAILURE;
}

/** Gets an array of items
@param item_name :: the item name
@param larray :: The returned array
@return 0 on success
*/
template <typename T>
int item_struct<T>::getArrayItem(const std::string& item_name, T* larray)
{
	int n;
	long spec_no;
	n = item_name.find('_');
	if (n != std::string::npos)
	{
		spec_no = atol(item_name.c_str()+n+1);
		return getIntArrayItem(item_name.substr(0,n), &spec_no, 1, larray);
	}
	else
	{
		spec_no = 0;
		return getIntArrayItem(item_name, &spec_no, 1, larray);
	}
}

