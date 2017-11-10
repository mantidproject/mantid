.. Documentation master file
   It contains a hidden root toctree directive so that Sphinx
   has an internal index of all of the pages and doesn't
   produce a plethora of warnings about most documents not being in
   a toctree.
   See http://sphinx-doc.org/tutorial.html#defining-document-structure

.. _contents:

.. image:: images/Mantid_Logo_Transparent.png
   :alt: The logo for the Mantid Project
   :align: center

====================
Mantid Documentation
====================

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 1

   algorithms/index
   algorithms/*
   concepts/index
   development/index
   interfaces/index
   fitfunctions/*
   fitminimizers/index
   techniques/index
   api/index
   release/index


This is the documentation for Mantid |release|.

**Sections:**

.. image:: images/mantid.png
   :alt: A preying mantis with arms upraised
   :width: 200px
   :align: right

* `Algorithms <algorithms/index.html>`_
* `Concepts <concepts/index.html>`_
* `Interfaces <interfaces/index.html>`_
* `Fit Functions <fitfunctions/index.html>`_
* `Fit Minimizers <fitminimizers/index.html>`_
* `Techniques <techniques/index.html>`_
* `API <api/index.html>`_
    - `Python <api/python/index.html>`_
    - `C++ <http://doxygen.mantidproject.org/>`_ (Doxygen)
* `Release Notes <release/index.html>`_


