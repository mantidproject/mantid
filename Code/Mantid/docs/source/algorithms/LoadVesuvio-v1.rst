.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The `Vesuvio <http://www.isis.stfc.ac.uk/instruments/vesuvio/vesuvio4837.html>`__ instrument at ISIS produces
`RAW <http://www.mantidproject.org/Raw File>` files in the standard format. However, due to the unique design
of the instrument the raw counts from the input files must be manipulated before they are considered useful.

This algorithm wraps calls to :ref:`LoadRaw <algm-LoadRaw>` and computes the counts (:math:`\mu s^{-1}`) using the
foil-cycling method described `here <http://m.iopscience.iop.org/0957-0233/23/4/045902/pdf/0957-0233_23_4_045902.pdf>`__.

The output is point data and not a histogram.

IP File
#######

There is an option to specify an ascii instrument parameter file to specify new detector positions along with a *t0* delay
time parameter for each detector.

Usage
-----

**Load a single file & spectrum with default difference:**

.. code-block:: python

   tof = LoadVesuvio("14188",SpectrumList=135)

   print "Number of spectra:", tof.getNumberHistograms()

Output::

   Number of spectra: 1

**Sum runs on single spectrum with default difference:**

.. code-block:: python

   tof = LoadVesuvio("14188-14193",SpectrumList=135)

   print "Number of spectra:", tof.getNumberHistograms()

Output::

   Number of spectra: 1

**Sum runs on a range of spectra with default difference:**

.. code-block:: python

   tof = LoadVesuvio("14188-14193",SpectrumList="135-142")

   print "Number of spectra:", tof.getNumberHistograms()

Output::

   Number of spectra: 8

**Sum runs and spectra on a range of spectra with default difference:**

.. code-block:: python

   tof = LoadVesuvio("14188-14193",SpectrumList="135-142", SumSpectra=True)

   print "Number of spectra:", tof.getNumberHistograms()

Output::

   Number of spectra: 1

**Sum runs and spectra on a range of spectra using single difference method:**

.. code-block:: python

   tof = LoadVesuvio("14188-14193",SpectrumList="135-142", SumSpectra=True,
                     Mode="SingleDifference")

   print "Number of spectra:", tof.getNumberHistograms()

Output::

   Number of spectra: 1

.. categories::
