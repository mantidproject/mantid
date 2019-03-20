.. Documentation master file
   It contains a hidden root toctree directive so that Sphinx
   has an internal index of all of the pages and doesn't
   produce a plethora of warnings about most documents not being in
   a toctree.
   See http://sphinx-doc.org/tutorial.html#defining-document-structure

.. _contents:

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
   workbench/index


======
Mantid
======

The Mantid Framework has been created to visualise, manipulate and analyse neutron and muon scattering data. Mantid can be used though several Graphical User Interfaces (GUI) and programming languages, including:

* `Mantidplot <https://www.mantidproject.org/MantidPlot:_Help>`_, the original interface supporting a wide range of techniques and visualisation approaches.
* `Mantid Workbench <https://www.mantidproject.org/Mantid_4>`_, A newer, more stable, interface with growing supports for a list of techniques.
* Autoreduction services at specific facilities, including ISIS and the `SNS <https://monitor.sns.gov>`_.
* API's for :ref:`Python <pythonapi>` and `C++ <http://doxygen.mantidproject.org/>`_.

Documentation
=============

.. image:: images/mantid.png
   :alt: A preying mantis with arms upraised
   :width: 200px
   :align: right

This is the documentation for Mantid |release|.

**Getting Started:**
  * :ref:`mantid_basic_course`, takes you through installing and basic use of Mantid through Mantidplot, including loading, visualising and fitting data.
  * :ref:`training`, links to further self paced courses and learning resources.
  * :ref:`Mantid Matplotlib Plot Gallery and Examples <plotting>`, details of how to make pretty plots and manioukate them via Python.
    
**Reference Documentation:**
  * :ref:`Algorithms List`, specific details for all of our algorithms including descriptions of all inputs and parameters .
  * :ref:`workbench`, details on what's on Mantid Workbench and how to use some of the features.
  * :ref:`concepts contents`, deeper background information on the fundamental concepts within Mantid.
  * :ref:`interfaces contents`, descriptions of the bespoke technique interfaces available.
  * :ref:`fitting contents`, details of all the supported fitting functions and minimization approaches.
  * :ref:`techniques contents`, with further information relevant to some of the specific scientific techniques.
  * API's for :ref:`Python <pythonapi>` and `C++ <http://doxygen.mantidproject.org/>`_.
  * :ref:`release_notes`, a catalog of all of the release notes since v3.5.2.

**Other Help and Documentation:**
    * A `forum <http://forum.mantidproject.org/>`_, to ask for help, report issues, and discuss with other Mantid users.
    * A built in error-reporter. We really encourage you to use this, as it helps us to make Mantid more stable in future.
    * `Developer Documentation <http://developer.mantidproject.org/>`_
