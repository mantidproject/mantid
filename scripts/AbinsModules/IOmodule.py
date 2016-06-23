import h5py
import numpy

class IOmodule(object):
    """
    Class for ABINS I/O operations.
    """
    def __init__(self):

        self._hdf_filename = None # name of hdf file
        self._group_name = None # name of group from hdf file.
        self._attributes = {} # attributes for group
        self._numpy_datasets = {} # datasets for group are expected to be numpy arrays
        self._structured_datasets = {} # complex data sets

        # All fields have to be set by inheriting class.

    def _prepare_HDF_file(self, file_name=None, group_name=None):
        """
        Sets up hdf file.
        @param file_name: name of input file from ab-initio calculations (CASTEP: foo.phonon)
        @param group_name: name of directory in which data in hdf file will be saved
        """
        core_name = file_name[0:file_name.find(".")]
        self._hdf_filename = core_name + ".hdf5"
        self._group_name = group_name

        hdf_file = h5py.File(self._hdf_filename, 'a')
        if not self._group_name in hdf_file:
            hdf_file.create_group(self._group_name)
        hdf_file.close()

    def addAttribute(self, name=None, value=None):
        """
        Adds attribute to the dictionary with other attributes.
        @param name: name of the attribute
        @param value: value of the attribute. More about attributes at: http://docs.h5py.org/en/latest/high/attr.html
        """
        self._attributes[name] = value

    def addStructuredDataset(self, name=None, value=None):
        """
        Adds data in the form of list of dictionaries into the collection of datasets.
        @param name: name of dataset
        @param value: list of dictionaries to be s
        """
        self._structured_datasets[name]=value

    def addNumpyDataset(self, name=None, value=None):
        """
        Adds dataset in the for mof numpy array to the dictionary with the collection of other datasets.
        @param name: name of dataset
        @param value: value of dataset. Numpy array is expected. More about dataset at:
        http://docs.h5py.org/en/latest/high/dataset.html
        """
        self._numpy_datasets[name] = value

    def _save_attributes(self, group=None):
        """
        Saves attributes to hdf file.
        @param group: group to which attributes should be saved.
        """
        for name in self._attributes:
            group.attrs[name] = self._attributes[name]


    def _recursively_save_dict_contents_to_group(self, hdf_file=None, path=None, dic=None):
        """
        ....
        """

        if isinstance(dic, dict):
            for key, item in dic.items():
                if isinstance(item, (numpy.ndarray, numpy.int64, int, numpy.float64, str, bytes)):
                    folder = path + key
                    if folder in hdf_file:
                        del hdf_file[path + key]
                    hdf_file[folder] = item
                elif isinstance(item, dict):
                    self._recursively_save_dict_contents_to_group(hdf_file=hdf_file, path=path + key + '/', dic=item)
                else:
                    raise ValueError('Cannot save %s type'%type(item))
        else:
            hdf_file[path + dict] = dict

    def _save_structured_datasets(self, hdf_file=None, group_name=None):
        """
        Saves structured data in the form of dictionary or list of dictionaries.
        @param hdf_file: hdf file to which data should be saved
        @param group_name: name of the main group.

        """

        for item in self._structured_datasets:
            if isinstance(self._structured_datasets[item], list):
                num_el = len(self._structured_datasets[item])
                for el in range(num_el):
                    self._recursively_save_dict_contents_to_group(hdf_file=hdf_file, path=group_name + "/" + item + "/%s/" % el, dic=self._structured_datasets[item][el])
            elif isinstance(self._structured_datasets[item], dict):
                self._recursively_save_dict_contents_to_group(hdf_file=hdf_file, path=group_name + "/" + item, dic=self._structured_datasets[item])
            else:
                raise ValueError('Cannot save %s type'%type(item))


    def _save_numpy_datasets(self, group=None):
        """
        Saves datasets to hdf file.
        @param group: group to which datasets should be saved.
        """
        for name in self._numpy_datasets:
            if name in group:
                del group[name]
                group.create_dataset(name=name, data=self._numpy_datasets[name], compression="gzip", compression_opts=9)
            else:
                group.create_dataset(name=name, data=self._numpy_datasets[name], compression="gzip", compression_opts=9)

    def save(self):
        """
        Saves data and attributes to hdf5 file.
        """

        with h5py.File(self._hdf_filename, 'a') as hdf_file:
            group = hdf_file[self._group_name]

            if not self._structured_datasets is None: self._save_structured_datasets(hdf_file=hdf_file, group_name=self._group_name)
            if not self._attributes is None: self._save_attributes(group=group)
            if not self._numpy_datasets is None: self._save_numpy_datasets(group=group)


    def _list_of_str(self, list_str=None):
        """
        Checks if all elements of list are strings.
        @param list_str: list to check
        @return: True if each entry in the list is a string, otherwise False
        """
        return isinstance(list_str, list) and all([isinstance(list_str[item], str) for item in range(len(list_str))])

    def _load_attributes(self, list_of_attributes=None, group=None):
        """
        Loads collection of attributes from the given group.
        @param list_of_attributes:
        @param group:
        @return:
        """
        if not self._list_of_str(list_str=list_of_attributes): raise ValueError("Invalid list of attributes!")

        results = {}
        for item in list_of_attributes:
            results[item] = self._load_attribute(name=item, group=group)

        return results

    def _load_attribute(self, name=None, group=None):
        """
        Loads attribute.
        @param group: group in hdf file
        @param name: name of attribute
        @return:  value of attribute
        """
        if not name in group.attrs:
            raise ValueError("Attribute %s in not present in %s file!" % (name, self._hdf_filename))
        else:
            return group.attrs[name]


    def _load_datasets(self,list_of_datasets=None, group=None):
        """
        Loads collection datasets from the given group.
        @param group:  group in hdf file
        @param list_of_datasets: list with names of datasets to be loaded
        @return: dictionary with collection of datasets.
        """
        if not self._list_of_str(list_str=list_of_datasets): raise ValueError("Invalid list of datasets!")

        results = {}
        for item in list_of_datasets:
            results[item] = self._load_dataset(name=item, group=group)

        return results

    def _load_dataset(self, name=None, group=None):
        """
        Loads dataset.
        @param group: group in hdf file
        @param name: name of dataset
        @return: value of dataset
        """
        if not name in group:
            raise ValueError("Dataset %s in not present in %s file!" % (name, self._hdf_filename))
        else:
            return numpy.copy(group[name])

    def _load_structured_datasets(self, hdf_file=None, list_of_structured_datasets=None, group=None):
        """
        Loads structured dataset which has a form of python dictionary directly from hdf file.
        @param hdf_file: hdf file object from which data should be loaded
        @param list_of_structured_datasets:
        @param group:
        @return:
        """
        if not self._list_of_str(list_str=list_of_structured_datasets): raise ValueError("Invalid list of structured datasets!")

        results={}
        for item in list_of_structured_datasets:
            results[item] = self._load_structured_dataset(hdf_file=hdf_file, name=item, group=group)

        return results

    def _get_subgrp_name(self, path=None):
        """
        Extracts name of the particular subgroup from the absolute name.
        @param path: absolute  name of subgroup
        @return: name of subgroup
        """
        reversed_path = path[::-1]
        end = reversed_path.find("/")
        return reversed_path[:end]

    def _convert_unicode_to_string_core(self, item=None):
        """
        Convert atom element from unicode to str
        :param item: converts unicode to item
        :return:
        """
        assert isinstance(item, unicode)
        return str(item).replace("u'", "'")

    def _convert_unicode_to_str(self, objectToCheck=None):
        """
        Converts unicode to python str, works for nested dicts and lists (recursive algorithm).
        Works for list in which entry is a tuple of items or one item only (item= one string, one float, ...etc. )
        Works for dictionary in which value/keyword is a tuple of items  or one item only
        In case tuple is a  keyword of the dictionary it will be converted to python str with a
        tuple brackets  -> "( )" (keywords in the dictionary are expected to be be
        a string or one of the primitive type of python but not tuple),
        in case of tuple value in dictionary, it will be converted to tuple,
        all unicode will be removed from the entries of tuple.

        :param objectToCheck: Entry in dictionary read by read_txt_file in
                            which  unicode symbols should be converted to str.
        """

        if isinstance(objectToCheck, list):
            for i in range(len(objectToCheck)):
                objectToCheck[i] = self._convert_unicode_to_str(objectToCheck[i])

        elif isinstance(objectToCheck, dict):
            for item in objectToCheck:
                if isinstance(item, unicode):

                    decoded_item = self._convert_unicode_to_string_core(item)
                    item_dict = objectToCheck[item]
                    del objectToCheck[item]
                    objectToCheck[decoded_item] = item_dict
                    item = decoded_item

                objectToCheck[item] = self._convert_unicode_to_str(objectToCheck[item])

        # unicode element
        elif isinstance(objectToCheck, unicode):
            objectToCheck = self._convert_unicode_to_string_core(objectToCheck)

        return objectToCheck




    def _load_structured_dataset(self, hdf_file=None, name=None, group=None):
        """
        Loads one structured dataset.
        @param hdf_file:  hdf file object from which structured dataset should be loaded.
        @param name:  name of dataset
        @param group: name of the main group
        @return:
        """
        hdf_group= hdf_file[group][name]

        if all([self._get_subgrp_name(path=hdf_group[el].name).isdigit() for el in hdf_group.keys()]):
            _structured_dataset_list = []
            for item in hdf_group.keys():
                _structured_dataset_list.append(self._recursively_load_dict_contents_from_group(hdf_file=hdf_file, path=hdf_group.name+"/"+item))
            return self._convert_unicode_to_str(objectToCheck=_structured_dataset_list)
        else:
            return self._convert_unicode_to_str(objectToCheck=self._recursively_load_dict_contents_from_group(hdf_file=hdf_file, path=hdf_group.name+"/"))

    def _recursively_load_dict_contents_from_group(self, hdf_file=None, path=None):
        """
        Loads structure dataset which has form of python dictionary.
        @param hdf_file:  hdf file object from which data set is loaded
        @param path: path to dataset in hdf file
        @return: dictionary which was loaded from hdf file

        """
        ans = {}
        for key, item in hdf_file[path].items():
            if isinstance(item, h5py._hl.dataset.Dataset):
                ans[key] = item.value
            elif isinstance(item, h5py._hl.group.Group):
                ans[key] = self._recursively_load_dict_contents_from_group(hdf_file, path + key + '/')
        return ans


    def load(self, list_of_attributes=None, list_of_datasets=None, list_of_structured_datasets=None):
        """
        Loads all necessary data.
        @param list_of_attributes: list of attributes to load (list of strings with names of attributes)
        @param list_of_datasets: list of datasets to load (list of strings with names of datasets)
        @param list_of_structured_datasets:
        @return: dictionary with both datasets and attributes

        """
        results = {}
        with h5py.File(self._hdf_filename, 'r') as hdf_file:
            if self._group_name is None: raise ValueError("Name of group in the hdf file is not set!")
            if not self._group_name in hdf_file: raise ValueError("No group %s in hdf file!"% self._group_name)

            group = hdf_file[self._group_name]

            if not self._structured_datasets is None:
                results["structuredDatasets"] = self._load_structured_datasets(hdf_file=hdf_file,
                                                                                list_of_structured_datasets=list_of_structured_datasets,
                                                                                group=group)

            if not self._attributes is None:
                results["attributes"] = self._load_attributes(list_of_attributes=list_of_attributes, group=group)

            if not self._numpy_datasets is None:
                results["datasets"] = self._load_datasets(list_of_datasets=list_of_datasets, group=group)


        return results

