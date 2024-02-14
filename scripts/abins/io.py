# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import hashlib
import io
import json
import os
import subprocess
import shutil
from typing import List, Optional

import h5py

import numpy as np
from pydantic import BaseModel, ConfigDict, Field, validate_call

import abins
from abins.constants import AB_INITIO_FILE_EXTENSIONS, BUF, HDF5_ATTR_TYPE
from mantid.kernel import logger, ConfigService


class IO(BaseModel):
    """
    Class for Abins I/O HDF file operations.
    """

    model_config = ConfigDict(strict=True)

    input_filename: str = Field(min_length=1)
    group_name: str = Field(min_length=1)
    setting: str = ""
    autoconvolution: int = 10
    temperature: float = None

    def model_post_init(self, __context):
        try:
            self._hash_input_filename = self.calculate_ab_initio_file_hash()
        except IOError as err:
            logger.error(str(err))
        except ValueError as err:
            logger.error(str(err))

        # extract name of file from the full path in the platform independent way
        filename = os.path.basename(self.input_filename)

        if filename.split(".")[-1] in AB_INITIO_FILE_EXTENSIONS:
            core_name = filename[0 : filename.rfind(".")]  # e.g. NaCl.phonon -> NaCl (core_name) -> NaCl.hdf5
        else:
            core_name = filename  # e.g. OUTCAR -> OUTCAR (core_name) -> OUTCAR.hdf5

        self._hdf_filename = os.path.join(self.get_save_dir_path(), core_name + ".hdf5")  # name of hdf file

        self._attributes = {}  # attributes for group

        # data  for group; they are expected to be numpy arrays or
        # complex data sets which have the form of Python dictionaries or list of Python
        # dictionaries
        self._data = {}

        # Fields which have a form of empty dictionaries have to be set by an inheriting class.

    @staticmethod
    def get_save_dir_path() -> str:
        return ConfigService.getString("defaultsave.directory")

    def _valid_hash(self):
        """
        Checks if input ab initio file and content of HDF file are consistent.
        :returns: True if consistent, otherwise False.
        """
        saved_hash = self.load(list_of_attributes=["hash"])
        return self._hash_input_filename == saved_hash["attributes"]["hash"]

    def _valid_setting(self):
        """
        Checks if setting matches content of HDF file.
        :returns: True if consistent, otherwise False.
        """
        saved_setting = self.load(list_of_attributes=["setting"])
        return self.setting == saved_setting["attributes"]["setting"]

    def _valid_autoconvolution(self):
        """
        Check if autoconvolution setting matches content of HDF file
        :returns: True if consistent, otherwise False
        """
        saved_autoconvolution = self.load(list_of_attributes=["autoconvolution"])
        return self.autoconvolution == saved_autoconvolution["attributes"]["autoconvolution"]

    def _valid_temperature(self):
        """
        Check if temperature setting matches content of HDF file

        If temperature is not important, set to None in Clerk
        constructor. (E.g. when loading phonon data from file.)

        :returns: True if consistent or temperature not set for Clerk, otherwise False
        """
        if self.temperature is None:
            return True

        else:
            from abins.constants import T_THRESHOLD

            saved_temperature = self.load(list_of_attributes=["temperature"])
            if saved_temperature["attributes"]["temperature"] == "None":
                return False

            else:
                return np.abs(self.temperature - saved_temperature["attributes"]["temperature"]) < T_THRESHOLD

    @classmethod
    def _close_enough(cls, previous, new):
        if isinstance(new, dict):
            for key, new_value in new.items():
                if key not in previous:
                    logger.warning(f"New key in advanced parameters: {key}")
                    return False
                elif not cls._close_enough(new_value, previous[key]):
                    logger.warning("Mismatched items from Abins advanced parameters:")
                    logger.warning(str((key, new_value, previous[key])))
                    return False
            return True

        if isinstance(new, (np.ndarray, float)):
            if previous is None:
                return False
            return np.allclose(previous, new)
        # Tuples are converted to list in caching
        elif isinstance(new, tuple):
            return cls._close_enough(list(new), previous)
        else:
            return previous == new

    def _valid_advanced_parameters(self):
        """
        Check if advanced parameters haven't changed.

        Compare JSON dict stored as data attribute with data in AbinsParams.
        :returns: True if they are the same, otherwise False

        """
        from abins.parameters import non_performance_parameters as current_advanced_parameters

        previous_advanced_parameters = self.load(list_of_attributes=["advanced_parameters"])
        previous_advanced_parameters = json.loads(previous_advanced_parameters["attributes"]["advanced_parameters"])

        diff = {
            key: (value, previous_advanced_parameters.get(key, None))
            for key, value in current_advanced_parameters.items()
            if not self._close_enough(previous_advanced_parameters.get(key, None), value)
        }
        diff.update({key: (None, value) for key, value in previous_advanced_parameters.items() if key not in current_advanced_parameters})

        if diff:
            sections = ("instruments", "sampling", "hdf_groups")

            for section in sections:
                if section in diff:
                    logger.information(f"Differences in Abins {section} parameters:")
                    new_group, old_group = diff[section]
                    for key in new_group:
                        if key not in old_group:
                            old_group[key] = "<key not present>"

                        if new_group[key] != old_group[key]:
                            logger.information(key)
                            logger.information("New values:")
                            logger.information(str(new_group[key]))
                            logger.information("Previous values:")
                            logger.information(str(old_group[key]))
            return False
        else:
            return True

    def get_previous_ab_initio_program(self):
        """
        :returns: name of ab initio program which  was used in the previous calculation.
        """
        return self.load(list_of_attributes=["ab_initio_program"])["attributes"]["ab_initio_program"]

    def check_previous_data(self):
        """
        Checks if currently used ab initio file is the same as in the previous calculations. Also checks if currently
        used parameters from AbinsParameters are the same as in the previous calculations.
        """
        if not self._valid_hash():
            raise ValueError("Different ab initio file  was used in the previous calculations.")

        if not self._valid_advanced_parameters():
            raise ValueError("Different advanced parameters were used in the previous calculations.")

        if not self._valid_setting():
            raise ValueError("Different instrument setting was used in the previous calculations")

        if not self._valid_autoconvolution():
            raise ValueError("Autoconvolution setting is not consistent with the previous calculations")

        if not self._valid_temperature():
            raise ValueError("Temperature setting is not consistent with the previous calculations.")

    def erase_hdf_file(self):
        """
        Erases content of hdf file.
        """

        with h5py.File(self._hdf_filename, "w") as hdf_file:
            hdf_file.close()

    @validate_call(config=ConfigDict(arbitrary_types_allowed=True, strict=True))
    def add_attribute(self, name: str, value: HDF5_ATTR_TYPE | None) -> None:
        """
        Adds attribute to the dictionary with other attributes.
        :param name: name of the attribute
        :param value: value of the attribute. More about attributes at: http://docs.h5py.org/en/latest/high/attr.html
        """
        self._attributes[name] = value

    def add_file_attributes(self):
        """
        Add attributes for input data filename, hash of file, advanced parameters to data for HDF5 file
        """
        self.add_attribute("hash", self._hash_input_filename)
        self.add_attribute("setting", self.setting)
        self.add_attribute("autoconvolution", self.autoconvolution)
        self.add_attribute("temperature", self.temperature)
        self.add_attribute("filename", self.input_filename)
        self.add_attribute("advanced_parameters", json.dumps(abins.parameters.non_performance_parameters))

    def add_data(self, name=None, value=None):
        """
        Adds data  to the dictionary with the collection of other datasets.
        :param name: name of dataset
        :param value: value of dataset. Numpy array is expected or complex data sets which have the form of Python
                      dictionaries or list of Python dictionaries. More about dataset at:
                      http://docs.h5py.org/en/latest/high/dataset.html
        """
        self._data[name] = value

    def _save_attributes(self, group=None):
        """
        Saves attributes to an hdf file.
        :param group: group to which attributes should be saved.
        """
        for name in self._attributes:
            if self._attributes[name] is None:
                group.attrs[name] = "None"
            else:
                group.attrs[name] = self._attributes[name]

    def _recursively_save_structured_data_to_group(self, hdf_file=None, path=None, dic=None):
        """
        Helper function for saving structured data into an hdf file.
        :param hdf_file: hdf file object
        :param path: absolute name of the group
        :param dic:  dictionary to be added
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
                self._recursively_save_structured_data_to_group(hdf_file=hdf_file, path=folder + "/", dic=item)
            else:
                raise ValueError("Cannot save %s type" % type(item))

    def _save_data(self, hdf_file=None, group=None):
        """
        Saves  data in the form of numpy array, dictionary or list of dictionaries. In case data in group already exist
        it will be overridden.
        :param hdf_file: hdf file object to which data should be saved
        :param group: group to which data should be saved.

        """

        for item in self._data:
            # case data to save is a simple numpy array
            if isinstance(self._data[item], np.ndarray):
                if item in group:
                    del group[item]
                group.create_dataset(name=item, data=self._data[item], compression="gzip", compression_opts=9)
            # case data to save has form of list
            elif isinstance(self._data[item], list):
                num_el = len(self._data[item])
                for el in range(num_el):
                    self._recursively_save_structured_data_to_group(
                        hdf_file=hdf_file, path=group.name + "/" + item + "/%s/" % el, dic=self._data[item][el]
                    )
            # case data has a form of dictionary
            elif isinstance(self._data[item], dict):
                self._recursively_save_structured_data_to_group(hdf_file=hdf_file, path=group.name + "/" + item + "/", dic=self._data[item])
            else:
                raise TypeError("Invalid structured dataset. Cannot save %s type" % type(item))

    def save(self):
        """
        Saves datasets and attributes to an hdf file.
        """

        with h5py.File(self._hdf_filename, "a") as hdf_file:
            if self.group_name not in hdf_file:
                hdf_file.create_group(self.group_name)
            group = hdf_file[self.group_name]

            if len(self._attributes.keys()) > 0:
                self._save_attributes(group=group)
            if len(self._data.keys()) > 0:
                self._save_data(hdf_file=hdf_file, group=group)

        # Repack if possible to reclaim disk space
        try:
            path = os.getcwd()
            temp_file = self._hdf_filename[self._hdf_filename.find(".")] + "temphgfrt.hdf5"

            subprocess.check_call(["h5repack" + " -i " + os.path.join(path, self._hdf_filename) + " -o " + os.path.join(path, temp_file)])

            shutil.move(os.path.join(path, temp_file), os.path.join(path, self._hdf_filename))
        except OSError:
            pass  # repacking failed: no h5repack installed in the system... but we proceed
        except IOError:
            pass
        except RuntimeError:
            pass

    @staticmethod
    @validate_call
    def _list_of_str(list_str: Optional[List[str]]) -> bool:
        """
        Checks if all elements of the list are strings.
        :param list_str: list to check
        :returns: True if each entry in the list is a string, otherwise False
        """
        if list_str is None:
            return False

        return True

    def _load_attributes(self, list_of_attributes=None, group=None):
        """
        Loads collection of attributes from the given group.
        :param list_of_attributes:
        :param group: name of group
        :returns: dictionary with attributes
        """

        results = {}
        for item in list_of_attributes:
            results[item] = self._load_attribute(name=item, group=group)

        return results

    def _load_attribute(self, name=None, group=None):
        """
        Loads attribute.
        :param group: group in hdf file
        :param name: name of attribute
        :returns:  value of attribute
        """
        if name not in group.attrs:
            raise ValueError("Attribute %s in not present in %s file." % (name, self._hdf_filename))
        else:
            return group.attrs[name]

    def _load_datasets(self, hdf_file=None, list_of_datasets=None, group=None):
        """
        Loads structured dataset which has a form of Python dictionary directly from an hdf file.
        :param hdf_file: hdf file object from which data should be loaded
        :param list_of_datasets:  list with names of  datasets to be loaded
        :param group: name of group
        :returns: dictionary with datasets
        """

        results = {}
        for item in list_of_datasets:
            results[item] = self._load_dataset(hdf_file=hdf_file, name=item, group=group)

        return results

    @staticmethod
    def _get_subgrp_name(path):
        """
        Extracts name of the particular subgroup from the absolute name.
        :param path: absolute  name of subgroup
        :returns: name of subgroup
        """
        reversed_path = path[::-1]
        end = reversed_path.find("/")
        return reversed_path[:end]

    @staticmethod
    def _encode_utf8_if_text(item):
        """
        Convert atom element from unicode to str
        but only in Python 2 where unicode handling is a mess

        :param item: item to convert to unicode str if Python 2 str
        :returns: laundered item
        """
        if isinstance(item, str):
            return item.encode("utf-8")
        else:
            return item

    @validate_call(config=ConfigDict(arbitrary_types_allowed=True))
    def _load_dataset(self, *, hdf_file: h5py.File, name: str, group: h5py.Group):
        """
        Loads one structured dataset.
        :param hdf_file:  hdf file object from which structured dataset should be loaded.
        :param name:  name of dataset
        :param group: name of the main group
        :returns: loaded dataset
        """
        if name in group:
            hdf_group = group[name]
        else:
            raise ValueError("Invalid name of the dataset.")

        # noinspection PyUnresolvedReferences,PyProtectedMember
        if isinstance(hdf_group, h5py.Dataset):
            return hdf_group[()]
        elif all([self._get_subgrp_name(hdf_group[el].name).isdigit() for el in hdf_group.keys()]):
            structured_dataset_list = []
            # here we require that numerical keys are always a sequence 0, 1, 2, 3 ...
            num_keys = len(hdf_group.keys())
            for item in range(num_keys):
                structured_dataset_list.append(
                    self._recursively_load_dict_contents_from_group(hdf_file=hdf_file, path=hdf_group.name + "/%s" % item)
                )
            return structured_dataset_list
        else:
            return self._recursively_load_dict_contents_from_group(hdf_file=hdf_file, path=hdf_group.name + "/")

    @classmethod
    def _recursively_load_dict_contents_from_group(cls, hdf_file=None, path=None):
        """
        Loads structure dataset which has form of Python dictionary.
        :param hdf_file:  hdf file object from which dataset is loaded
        :param path: path to dataset in hdf file
        :returns: dictionary which was loaded from hdf file

        """
        ans = {}
        for key, item in hdf_file[path].items():
            # noinspection PyUnresolvedReferences,PyProtectedMember,PyProtectedMember
            if isinstance(item, h5py._hl.dataset.Dataset):
                ans[key] = item[()]
            elif isinstance(item, h5py._hl.group.Group):
                ans[key] = cls._recursively_load_dict_contents_from_group(hdf_file, path + key + "/")
        return ans

    def load(self, list_of_attributes=None, list_of_datasets=None):
        """
        Loads all necessary data.
        :param list_of_attributes: list of attributes to load (list of strings with names of attributes)
        :param list_of_datasets: list of datasets to load. It is a list of strings with names of datasets.
                                       Datasets have a form of numpy arrays. Datasets can also have a form of Python
                                       dictionary or list of Python dictionaries.
        :returns: dictionary with both datasets and attributes

        """

        results = {}
        with h5py.File(self._hdf_filename, "r") as hdf_file:
            if self.group_name not in hdf_file:
                raise ValueError("No group %s in hdf file." % self.group_name)

            group = hdf_file[self.group_name]

            if self._list_of_str(list_str=list_of_attributes):
                results["attributes"] = self._load_attributes(list_of_attributes=list_of_attributes, group=group)

            if self._list_of_str(list_str=list_of_datasets):
                results["datasets"] = self._load_datasets(hdf_file=hdf_file, list_of_datasets=list_of_datasets, group=group)

        return results

    @staticmethod
    def _calculate_hash(filename=None):
        """
        Calculates hash  of a file defined by filename according to sha512 algorithm.

        :param filename: name of a file to calculate hash
        :type filename: str

        :returns: string representation of hash

        """
        hash_calculator = hashlib.sha512()

        # chop content of a file into chunks to minimize memory consumption for hash creation
        with io.open(file=filename, mode="rb", buffering=BUF, newline=None) as file_handle:
            while True:
                data = file_handle.read(BUF)
                if not data:
                    break
                hash_calculator.update(data)

        return hash_calculator.hexdigest()

    def get_input_filename(self):
        return self.input_filename

    def calculate_ab_initio_file_hash(self):
        """
        This method calculates hash of the file with vibrational or phonon data according to SHA-2 algorithm from
        hashlib library: sha512.
        :returns: string representation of hash for file with vibrational data which contains only hexadecimal digits
        """

        return self._calculate_hash(filename=self.input_filename)
