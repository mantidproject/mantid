.. _TrainingMantidOverview:

=================
 Mantid Overview
=================

What is Mantid?
---------------

The Mantid Framework is used for data reduction and analysis of neutron and muon scattering data. Mantid can be used via a Graphical User Interface, MantidPlot, or it can be used purely from Python. Information on using both of these is given in the this training course. It is written in a mix of C++ and Python, C++ is generally used where performance is important, while parts written in Python provide code that is easier to understand.

.. figure:: /images/Training/Introduction/Mantid_structure.png
   :align: center
   :width: 400

The Mantid Collaboration
------------------------

The Mantid project is an international collaboration between `ISIS <https://www.isis.stfc.ac.uk/>`__ (2007), `Oak Ridge National Lab <https://www.ornl.gov/>`__ (2010), the `ESS <https://europeanspallationsource.se/>`__ (2015) and the `ILL <https://www.ill.eu/>`__ (2016). There are also a number of other contributing organisations that use Mantid such as the `MLZ <http://www.mlz-garching.de/>`__, the `PSI <https://www.psi.ch/sinq/>`__ and `ANSTO <http://www.ansto.gov.au/>`__.

.. figure:: /images/Training/Introduction/Mantid_collaboration.png
   :align: center
   :width: 600

MantidPlot
----------

MantidPlot exists to provide an easier way to work with the data for those who prefer an interface, but all data manipulation functionality is available from both. Python scripts can also be run inside MantidPlot from the script window or interactive terminal.

.. figure:: /images/Training/Introduction/MantidPlot.png
   :align: center
   :width: 800

Python
------

For information on running Mantid from stand-alone Python see the :ref:`TrainingPythonAndMantid` section. Once setup Mantid can be used by adding the following import to your Python script:

.. code-block:: python

    from mantid.simpleapi import *

New Workbench
-------------

A new workbench is in development for Mantid, which will initially be an optional alternative to MantidPlot, but will eventually replace it. The general behaviour will be similar to that of the current MantidPlot, but it will take approach closer to that of Matlab, and use Matplotlib for the plotting functionality.

|
