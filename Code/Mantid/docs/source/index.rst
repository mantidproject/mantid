.. Documentation master file
   It contains a hidden root toctree directive so that Sphinx
   has an internal index of all of the pages and doesn't
   produce a plethora of warnings about most documents not being in
   a toctree.
   See http://sphinx-doc.org/tutorial.html#defining-document-structure

.. _contents:

===============
 Documentation
===============

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 1

   algorithms/*
   concepts/index
   fitfunctions/*
   api/index

This is the documentation for Mantid |release|.

**Parts:**

* `Algorithms <algorithms/index.html>`_
* `Concepts <concepts/index.html>`_
* `Fit Functions <fitfunctions/index.html>`_
* `API <api/index.html>`_
    - `Python <api/python/index.html>`_
