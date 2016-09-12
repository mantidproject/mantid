import h5py
import numpy as np 
import subprocess
import shutil
import hashlib

from mantid.kernel import logger

class IOmodule(object):
    """
    Class for ABINS I/O HDF file operations.
    """
    def __init__(self, input_filename=None, group_name=None):

        if  isinstance(input_filename, str):

            self._input_filename = input_filename
            try:
                self._hash_input_filename = self.calculateHash()
            except IOError as err:
                logger.error(str(err))

            # extract name of file from its path.
            begin = 0
            while input_filename.find("/") != -1:
                begin = input_filename.find("/") + 1
                input_filename = input_filename[begin:]

            if input_filename == "": raise ValueError("Name of the file cannot be an empty string!")

        else: raise ValueError("Invalid name of hdf file. String was expected!")

        if  isinstance(group_name, str): self._group_name = group_name
        else: raise ValueError("Invalid name of the group. String was expected!")

        core_name = input_filename[0:input_filename.find(".")]
        self._hdf_filename = core_name + ".hdf5" # name of hdf file
        self._attributes = {} # attributes for group
        self._data = {} # data  for group; they are expected to be numpy arrays or
                        # complex data sets which have the form of Python dictionaries or list of Python
                        # dictionaries

        # Fields which have a form of empty dictionaries have to be set by an inheriting class.


    def validData(self):
        """
        Checks if input DFT file and content of HDF file are consistent.
        @return: True if consistent, otherwise False.
        """
        _saved_hash = self.load(list_of_attributes=["hash"])

        return self._hash_input_filename == _saved_hash["attributes"]["hash"]


    def loadData(self):
        """
        Method which loads data from an hdf file. Method which has to be implemented by an inheriting class.
        """
        return None


    def calculateData(self):
        """
        Method which evaluates data in case loading failed. Method which has to be implemented by an inheriting class.
        """
        return None


    def getData(self):
        """
        Method to obtain data
        @return: obtained data
        """
        _data = None

        try:

            self.validData()
            _data = self.loadData()
            logger.notice(str(_data) + " has been loaded from the HDF file.")

        except (IOError, ValueError) as err:

            logger.notice("Warning: "+ str(err) + " Data has to be calculated.")
            _data = self.calculateData()
            logger.notice(str(_data) + " has been calculated.")

        return _data


    def eraseHDFfile(self):
        """
        Erases content of hdf file.
        """

        with h5py.File(self._hdf_filename, 'w') as hdf_file:
            pass


    def addAttribute(self, name=None, value=None):
        """
        Adds attribute to the dictionary with other attributes.
        @param name: name of the attribute
        @param value: value of the attribute. More about attributes at: http://docs.h5py.org/en/latest/high/attr.html
        """
        self._attributes[name] = value


    def addFileAttributes(self):
        """
        Adds file attributes: filename and hash of file to the collection of all attributes.
        @return:
        """
        self.addAttribute("hash", self._hash_input_filename)
        self.addAttribute("filename", self._input_filename)


    def addData(self, name=None, value=None):
        """
        Adds data  to the dictionary with the collection of other datasets.
        @param name: name of dataset
        @param value: value of dataset. Numpy array is expected or complex data sets which have the form of Python
                      dictionaries or list of Python dictionaries. More about dataset at:
                      http://docs.h5py.org/en/latest/high/dataset.html
        """
        self._data[name] = value


    def _save_attributes(self, group=None):
        """
        Saves attributes to an hdf file.
        @param group: group to which attributes should be saved.
        """
        for name in self._attributes:
            if isinstance(self._attributes[name], (np.int64, int, np.float64, float, str, bytes)):
                group.attrs[name] = self._attributes[name]
            else:
                raise ValueError("Invalid value of attribute. String, int or bytes was expected! (invalid type : %s)" %type(self._attributes[name]))


    def _recursively_save_structured_data_to_group(self, hdf_file=None, path=None, dic=None):
        """
        Helper function for saving structured data into an hdf file.
        @param hdf_file: hdf file object
        @param path: absolute name of the group
        @param dic:  dictionary to be added
        """

        for key, item in dic.items():
            folder = path + key
            if isinstance(item, (np.int64, int, np.float64, float, str, bytes)):
                if folder in hdf_file:
                    del hdf_file[folder]
                hdf_file[folder] = item
            elif isinstance(item, np.ndarray):
                if folder in hdf_file:
                    del hdf_file[folder]
                hdf_file.create_dataset(name=folder, data=item, compression="gzip", compression_opts=9)
            elif isinstance(item, dict):
                self._recursively_save_structured_data_to_group(hdf_file=hdf_file, path=folder + '/', dic=item)
            else:
                raise ValueError('Cannot save %s type'%type(item))


    def _save_data(self, hdf_file=None, group=None):
        """
        Saves  data in the form of numpy array, dictionary or list of dictionaries. In case data in group already exist
        it will be overridden.
        @param hdf_file: hdf file object to which data should be saved
        @param group: group to which data should be saved.

        """

        for item in self._data:
            # case data to save is a simple numpy array
            if isinstance(self._data[item], np.ndarray):
                if item in group: del group[item]
                group.create_dataset(name=item, data=self._data[item], compression="gzip", compression_opts=9)
            # case data to save has form of list
            elif isinstance(self._data[item], list):
                num_el = len(self._data[item])
                for el in range(num_el):
                    self._recursively_save_structured_data_to_group(hdf_file=hdf_file, path=group.name + "/" + item + "/%s/" % el, dic=self._data[item][el])
            # case data has a form of dictionary
            elif isinstance(self._data[item], dict):
                self._recursively_save_structured_data_to_group(hdf_file=hdf_file, path=group.name + "/" + item + "/", dic=self._data[item])
            else:
                raise ValueError('Invalid structured dataset. Cannot save %s type'%type(item))


    def save(self):
        """
        Saves datasets and attributes to an hdf file.
        """

        with h5py.File(self._hdf_filename, 'a') as hdf_file:
            if not self._group_name in hdf_file:
                hdf_file.create_group(self._group_name)
            group = hdf_file[self._group_name]

            if len(self._attributes.keys())>0: self._save_attributes(group=group)
            if len(self._data.keys())>0: self._save_data(hdf_file=hdf_file, group=group)

        # Repack if possible to reclaim disk space
        try:
            subprocess.check_call(["h5repack","-i%s"%self._hdf_filename, "-otemphgfrt.hdf5"])
            shutil.move("temphgfrt.hdf5", self._hdf_filename)
        except OSError:
            pass # repacking failed: no h5repack installed in the system... but we proceed


    def _list_of_str(self, list_str=None):
        """
        Checks if all elements of the list are strings.
        @param list_str: list to check
        @return: True if each entry in the list is a string, otherwise False
        """
        if list_str is None:
            return False

        if not  (isinstance(list_str, list) and all([isinstance(list_str[item], str) for item in range(len(list_str))])):
            raise ValueError("Invalid list of items to load!")

        return True


    def _load_attributes(self, list_of_attributes=None, group=None):
        """
        Loads collection of attributes from the given group.
        @param list_of_attributes:
        @param group:
        @return: dictionary with attributes
        """

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


    def _load_datasets(self, hdf_file=None, list_of_datasets=None, group=None):
        """
        Loads structured dataset which has a form of Python dictionary directly from an hdf file.
        @param hdf_file: hdf file object from which data should be loaded
        @param list_of_datasets:  list with names of  datasets to be loaded
        @param group:
        @return:
        """

        results={}
        for item in list_of_datasets:
            results[item] = self._load_dataset(hdf_file=hdf_file, name=item, group=group)

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
        @param item: converts unicode to item
        @return: converted element
        """
        assert isinstance(item, unicode)
        return str(item).replace("u'", "'")


    def _convert_unicode_to_str(self, objectToCheck=None):
        """
        Converts unicode to Python str, works for nested dicts and lists (recursive algorithm).

        @param objectToCheck: dictionary, or list with names which should be converted from unicode to string.
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


    def _load_dataset(self, hdf_file=None, name=None, group=None):
        """
        Loads one structured dataset.
        @param hdf_file:  hdf file object from which structured dataset should be loaded.
        @param name:  name of dataset
        @param group: name of the main group
        @return:
        """
        if not isinstance(name, str): raise ValueError("Invalid name of the dataset!")

        if name in group:
           _hdf_group = group[name]
        else:
            raise ValueError("Invalid name of the dataset!")

        if isinstance(_hdf_group, h5py._hl.dataset.Dataset):
            return _hdf_group.value
        elif all([self._get_subgrp_name(path=_hdf_group[el].name).isdigit() for el in _hdf_group.keys()]):
            _structured_dataset_list = []
            # here we make an assumption about keys which have a numeric values; we assume that always : 1, 2, 3... Max
            _num_keys = len(_hdf_group.keys())
            for item in range(_num_keys):
                _structured_dataset_list.append(self._recursively_load_dict_contents_from_group(hdf_file=hdf_file, path=_hdf_group.name+"/%s"%item))
            return self._convert_unicode_to_str(objectToCheck=_structured_dataset_list)
        else:
            return self._convert_unicode_to_str(objectToCheck=self._recursively_load_dict_contents_from_group(hdf_file=hdf_file, path=_hdf_group.name+"/"))


    def _recursively_load_dict_contents_from_group(self, hdf_file=None, path=None):
        """
        Loads structure dataset which has form of Python dictionary.
        @param hdf_file:  hdf file object from which dataset is loaded
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


    def load(self, list_of_attributes=None, list_of_datasets=None):
        """
        Loads all necessary data.
        @param list_of_attributes: list of attributes to load (list of strings with names of attributes)
        @param list_of_datasets: list of datasets to load. It is a list of strings with names of datasets.
                                       Datasets have a form of numpy arrays. Datasets can also have a form of Python
                                       dictionary or list of Python dictionaries.
        @return: dictionary with both datasets and attributes

        """

        results = {}
        with h5py.File(self._hdf_filename, 'r') as hdf_file:

            if not self._group_name in hdf_file: raise ValueError("No group %s in hdf file!"% self._group_name)

            group = hdf_file[self._group_name]

            if self._list_of_str(list_str=list_of_attributes):
                results["attributes"] = self._load_attributes(list_of_attributes=list_of_attributes, group=group)

            if self._list_of_str(list_str=list_of_datasets):
                results["datasets"] = self._load_datasets(hdf_file=hdf_file,
                                                          list_of_datasets=list_of_datasets,
                                                          group=group)

        return results


    def calculateHash(self):
        """
        This method calculates hash of the phonon file according to SHA-2 algorithm from hashlib library: sha512.
        @return: string representation of hash for phonon file which contains only hexadecimal digits
        """

        buf = 65536  # chop content of phonon file into 64kb chunks to minimize memory consumption for hash creation
        sha = hashlib.sha512()

        with open(self._input_filename, 'rU') as f:
            while True:
                data = f.read(buf)
                if not data:
                    break
                sha.update(data)

        return sha.hexdigest()