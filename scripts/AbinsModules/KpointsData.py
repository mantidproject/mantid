import numpy as np

# ABINS modules
import Constants
from GeneralData import  GeneralData

class KpointsData(GeneralData):
    """
    Class for storing kpoints data. The Data  is arranged as a list of dictionaries.
    The list has the following form:

    data= [ {"frequencies": numpy.array, "atomic_displacements: numpy.array, "weight": numpy._float, "value":numpy.array},
            {"frequencies": numpy.array, "atomic_displacements: numpy.array, "weight": numpy._float, "value":numpy.array}
            .............................................................................................................
            .............................................................................................................
          ]

    Each entry in the list corresponds to one k-point. Each item in the list is a dictionary. The meaning of
    keys in each dictionary is as follows:

    "weight" - weight of k-point

    "value"  - value of k-point (numpy array of dimension 3)

    "frequencies" - frequencies for the given k-point

    "atomic_displacements - atomic displacements for the given k-point
    """

    def __init__(self, num_k=None, num_atoms=None):
        """
        @param num_k: total number of k-points (int)
        @param num_atoms: total number of atoms in the unit cell (int)
        """
        super(KpointsData, self).__init__()

        if isinstance(num_k, int) and num_k>0:
            self._num_k = num_k
        else:
            raise ValueError("Invalid number of k-points.")

        if isinstance(num_atoms, int) and num_atoms>0:
            self._num_freq = 3 * num_atoms # number of phonons for one k-point
            self._num_displacements = self._num_freq * num_atoms * 3 # number of displacements coordinates for one k-point
        else:
            raise ValueError("Invalid number of atoms.")


    def append(self, item=None):
        """
        Adds one item to the collection of k-points data. Each item corresponds to one k-point.

        @param item: item to be added.
        """
        if not isinstance(item, dict):
            raise ValueError("Each element of KpointsData should be a dictionary.")

        if not sorted(item.keys()) == sorted(Constants.all_keywords_k_data):
            raise ValueError("Invalid structure of the dictionary to be added.")

        weight = item["weight"]
        if not (isinstance(weight, float) and weight > 0):
            raise ValueError("Invalid value of weight.")

        value = item["value"]
        if not isinstance(value ,np.ndarray):
            raise ValueError("Value of k-point should be an array.")
        if value.shape[0] != 3:
            raise ValueError("Value of k-point should be an numpy array with three float elements.")
        if not all([isinstance(value[el], float) for el in range(value.shape[0])]):
            raise ValueError("All coordinates of each k-vector should be represented by a real numbers.")

        freq = item["frequencies"]
        if not isinstance(freq, np.ndarray):
            raise ValueError("Frequencies for the given k-point should have a form of a numpy array.")
        if len(freq.shape) != 1:
            raise ValueError("Frequencies for the given k-point should have a form of one dimentional numpy array.")

        if freq.size != self._num_freq:
            raise ValueError("Incorrect number of frequencies.")

        if not all([isinstance(freq[el], float) for el in range(freq.shape[0])]):
            raise ValueError("Wrong type of frequencies. Frequencies should have real values")

        displacements = item["atomic_displacements"]
        if not isinstance(displacements, np.ndarray):
            raise ValueError("Atomic displacements for the given k-point should have a form of a numpy array.")
        if not len(displacements.shape) == 2:
            raise ValueError("Atomic displacements for the given k-point should "
                             "have a form of the two dimentional numpy array.")
        if displacements.size != self._num_displacements:
            raise ValueError("Invalid number of displacements.")
        if displacements.shape[1] != 3:
            raise ValueError("Atomic displacements for the given k-point should have a form of the"
                             "two dimentional numpy arrays with the second dimension equal to three.")
        for el1 in range(displacements.shape[0]):
            if not (all([isinstance(displacements[el1][el2], float)   for el2 in range(displacements.shape[1])])):
                raise  ValueError("Atomic displacements should have a form of the two dimentional"
                                  " numpy arrays with real elements.")

        self._data.append(item)


    def set(self, items=None):
        """
        Sets a new value for the collection of k-points data.
        @param items: new value of the collection.

        """

        if not isinstance(items, list):
            raise ValueError("Items should have a form of a list.")

        self._data = []
        for item in items:
            self.append(item=item)


    def extract(self):

        if self._num_k == len(self._data):
            return self._data
        else:
            raise ValueError("Size of KpointsData is different then number of k-points.")
