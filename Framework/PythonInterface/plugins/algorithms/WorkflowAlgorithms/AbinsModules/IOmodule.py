import h5py
import numpy

class IOmodule(object):
    """
    Class for ABINS I/O operations.
    """
    def __init__(self):

        self._hdf_filename = None # name of hdf file
        self._group_name = None # name of group from hdf file.
        self._attributes = None # attributes for group
        self._datasets = None # datasets for group are expected to be numpy arrays

        # All fields have to be set by inheriting class.

    def _prepare_HDF_file(self, file_name=None, group_name=None):
        """
        Set up hdf file.
        @param file_name: name of input file from ab-initio calculations (CASTEP: foo.phonon)
        @param group_name: name of directory in which data in hdf file will be saved
        """
        core_name = file_name[0:file_name.find(".")]
        self._hdf_filename = core_name + ".hdf5"
        self._group_name = group_name
        self._attributes = {}
        self._datasets = {}

        hdf_file = h5py.File(self._hdf_filename, 'a')
        if not self._group_name in hdf_file:
            hdf_file.create_group(self._group_name)
        hdf_file.close()

    def addAttribute(self, name=None, value=None):
        """
        Add attribute to the dictionary with other attributes.
        @param name: name of the attribute
        @param value: value of the attribute. More about attributes at: http://docs.h5py.org/en/latest/high/attr.html
        """
        self._attributes[name] = value

    def addDataset(self, name=None, value=None):
        """
        Add dataset to the dictionary with other datasets.
        @param name: name of dataset
        @param value: value of dataset. Numpy array is expected. More about dataset at:
        http://docs.h5py.org/en/latest/high/dataset.html
        """
        self._datasets[name] = value

    def _save_attributes(self, group=None):
        """
        Save attributes to hdf file.
        @param group: group to which attributes should be saved.
        """
        for name in self._attributes:
            group.attrs[name] = self._attributes[name]

    def _save_datasets(self, group=None):
        """
        Save datasets to hdf file.
        @param group: group to which datasets should be saved.
        """
        for name in self._datasets:
            if name in group:
                del group[name]
                group.create_dataset(name=name, data=self._datasets[name], compression="gzip", compression_opts=9)
            else:
                group.create_dataset(name=name,data=self._datasets[name], compression="gzip", compression_opts=9)

    def save(self):
        """
        Save data and attributes to hdf5 file.
        """
        hdf_file = h5py.File(self._hdf_filename, 'a')
        group=hdf_file[self._group_name]
        self._save_attributes(group=group)
        self._save_datasets(group=group)
        hdf_file.close()

    def _load_attribute(self, name=None, group=None):
        """
        Load attribute.
        @param group: group in hdf file
        @param name: name of attribute
        @return:  value of attribute
        """
        if not name in group.attrs:
            raise ValueError("Attribute %s in not present in %s file!" % (name, self._hdf_filename))
        else:
            return group.attrs[name]

    def _load_dataset(self, name=None, group=None):
        """
        Load dataset.
        @param group: group in hdf file
        @param name: name of dataset
        @return: value of dataset
        """
        if not name in group:
            raise ValueError("Dataset %s in not present in %s file!" % (name, self._hdf_filename))
        else:
            return numpy.copy(group[name])

    def load(self, list_of_attributes=None, list_of_datasets=None):
        """

        @param list_of_attributes: list of attributes to load (list of strings with names of attributes)
        @param list_of_datasets: list of datasets to load (list of strings with names of datasets)
        @return: dictionary with both datasets and attributes
        """
        hdf_file = h5py.File(self._hdf_filename, 'a')

        # check if input parameters  are valid
        if not (isinstance(list_of_attributes, list) or
                isinstance(list_of_datasets,list) or
                all([isinstance(list_of_attributes[name], str) for name in list_of_attributes]) or
                all([isinstance(list_of_datasets[name], str) for name in list_of_datasets])):
            raise ValueError("Invalid list of parameters!")

        group = hdf_file[self._group_name]
        results={"attributes":{}, "datasets":{}}

        for item in list_of_attributes:
            results["attributes"][item]=self._load_attribute(name=item, group=group)

        for item in list_of_datasets:
            results["datasets"][item]=self._load_dataset(name=item, group=group)

        hdf_file.close()
        return results



