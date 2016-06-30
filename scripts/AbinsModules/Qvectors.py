import numpy as np


class Qvectors(object):
    """

    """
    _all_formats = ["scalars", "vectors"]

    def __init__(self, q_format=None, vectors=None):


        if q_format not in self._all_formats:
            raise ValueError("Invalid format of Q vectors!")
        self.q_format = q_format

        if not isinstance(vectors, np.ndarray):
            raise ValueError("Invalid value of Q vectors. Numpy array was expected!")
        self.vectors = vectors





