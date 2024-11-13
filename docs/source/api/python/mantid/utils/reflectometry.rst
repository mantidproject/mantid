.. _mantid.utils.reflectometry:

Reflectometry Utilities
=======================


ORSO format IO
--------------

The `mantid.utils.reflectometry.orso_helper` module provides functionality for reading and saving
reflectivity data in ORSO (Open Reflectometry Standards Organization) format.
It defines classes such as `MantidORSOSaver`,  `MantidORSODataset`, and `MantidORSODataColumns`
to facilitate the creation, manipulation, and saving of ORSO datasets.
The `MantidORSOSaver` class manages the saving of datasets in ASCII or Nexus formats,
while the `MantidORSODataset` class constructs the dataset with necessary metadata and data columns.
Finally, the `MantidORSODataColumns` class handles the addition and management of data columns and their
associated metadata.
This module integrates with the Mantid framework to ensure that reflectivity data can be standardized
and shared in compliance with ORSO specifications.

.. image:: /images/orso_helper.png
    :alt: Class diagram showing the main classes and their relationships
    :width: 50%


The ``orso_helper`` module provides the backend functionality for the implementation of algorithm
:ref:`algm-SaveISISReflectometryORSO`.
