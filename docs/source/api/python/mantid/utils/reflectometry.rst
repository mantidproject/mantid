.. _mantid.utils.reflectometry:

Reflectometry Utilities
=======================


ORSO format IO
--------------

The `mantid.utils.reflectometry.orso_helper` module provides functionality for reading and saving
reflectivity data in the ORSO standard [#ORSO]_ format.
The orsopy library [#orsopy]_ is used to collect numerical data as well as metadata into meaningful data structures
which are later assembled and written out to file.
The file's format can be ASCII (``.ort``) or binary Nexus (``.orb``).

The `orso_helper` module defines classes such as `MantidORSOSaver`,  `MantidORSODataset`, and `MantidORSODataColumns`
to facilitate the creation, manipulation, and saving of ORSO datasets.
The `MantidORSOSaver` class manages the saving of datasets in ASCII or Nexus formats,
while the `MantidORSODataset` class constructs the dataset with necessary metadata and data columns.
Finally, the `MantidORSODataColumns` class handles the addition and management of data columns and their
associated metadata.


.. image:: /images/orso_helper.png
    :alt: Class diagram showing the main classes and their relationships
    :width: 50%

This module integrates with the rest of the Mantid framework to ensure that reflectivity is standardized
and shared in compliance with ORSO specifications.
For instance, the module provides the backend functionality for the implementation of algorithm
:ref:`algm-SaveISISReflectometryORSO`.


References
----------

.. [#ORSO] ORSO file format specification: `https://www.reflectometry.org/file_format/specification <https://www.reflectometry.org/file_format/specification>`_
.. [#orsopy] orsopy Python library: `https://orsopy.readthedocs.io/en/latest/ <https://orsopy.readthedocs.io/en/latest/>`_
