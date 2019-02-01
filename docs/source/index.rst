.. Documentation master file
   It contains a hidden root toctree directive so that Sphinx
   has an internal index of all of the pages and doesn't
   produce a plethora of warnings about most documents not being in
   a toctree.
   See http://sphinx-doc.org/tutorial.html#defining-document-structure

.. _contents:

====================
Mantid Documentation
====================

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 4

   tutorials/index
   algorithms/index
   algorithms/*
   concepts/index
   interfaces/index
   fitting/index
   fitting/fitfunctions/index
   fitting/fitfunctions/*
   fitting/fitminimizers/index
   techniques/index
   api/index
   plotting/index
   release/index


This is the documentation for Mantid |release|.

**Sections:**

.. image:: images/mantid.png
   :alt: A preying mantis with arms upraised
   :width: 200px
   :align: right

* :ref:`Algorithms List`
* :ref:`training`
    - :ref:`mantid_basic_course`
    - `Introduction To Python <http://www.mantidproject.org/Introduction_To_Python>`_
    - `Python in Mantid <http://www.mantidproject.org/Python_In_Mantid>`_
    - `Extending Mantid With Python <http://www.mantidproject.org/Extending_Mantid_With_Python>`_
* :ref:`concepts contents`
* :ref:`interfaces contents`
* :ref:`fitting contents`
    - :ref:`Fit Functions List`
    - :ref:`fitminimizers`
* :ref:`techniques contents`
* :ref:`api`
    - :ref:`Python <pythonapi>`
    - `C++ <http://doxygen.mantidproject.org/>`_ (Doxygen)
* :ref:`Mantid Matplotlib Plot Gallery and Examples <plotting>`
* :ref:`release_notes`
