.. algorithm::

.. summary::

.. relatedalgorithms::

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

Information on Vesuvio operation
################################

Vesuvio as an instrument has the ability to analyse both forward and back scattering from the sample. In addition, when data is collected,
foils are put in front of, or remove from, the detectors. The foils are different for forward and back scattering. In forward scattering,
there are only 2 foils states, the foil is either in front of the detector (foil in) or not infront of the detector (foil out). In back
scattering there are 3 foil states. Like in forward scattering there is both a foil in and foil out state, but in addition to this there
is also a foil thick state which is where a thicker foil is placed in front of the detector.


Vesuvio allows for measurements to be taken in different periods:

- 2 period data is for only analysing forward scattering (2 foil states - foil out/foil in)

- 3 period data is for only analysing back scattering (3 foil states - foil out/foil in/ foil thick)

- 6 period data is for both backward and forward scattering (combination of forward and back scattering states)

When loading data from .RAW files using this algorithm, several differencing modes can be used:

- Single Difference: Foil In minus Foil Out (available in both forward and back scattering)

- Thick Difference : Foil Thick minus Foil Out (only available in back scattering - as forward scattering has no foil Thick)

- Double Difference: Foil In minus Foil Out combined with Foil Thick minus Foil Out (only available in back scattering - as forward scattering has no foil Thick)


From the above modes of operation on Vesuvio we can deduce a table of valid combinations of scattering and differencing for each period.
This is shown in the tables below:


2 period
########
Gold foils only move in forward scattering, so only differencing in forward scattering is valid.

+------------------------+-------------------+------------------+-------------------+
|       Scattering       | Single Difference | Thick Difference | Double Difference |
+========================+===================+==================+===================+
|        Forward         |       Valid       |       NEVER      |       NEVER       |
+------------------------+-------------------+------------------+-------------------+
|         Back           |     Not Valid     |     Not Valid    |     Not Valid     |
+------------------------+-------------------+------------------+-------------------+


3 period
########
Gold foils only move in back scattering, so only differencing in back scattering is valid.

+------------------------+-------------------+------------------+-------------------+
|       Scattering       | Single Difference | Thick Difference | Double Difference |
+========================+===================+==================+===================+
|        Forward         |     Not Valid     |       NEVER      |       NEVER       |
+------------------------+-------------------+------------------+-------------------+
|         Back           |       Valid       |       Valid      |       Valid       |
+------------------------+-------------------+------------------+-------------------+


6 period
########
All are valid (with the exclusion of Thick Difference and Double difference in forward scattering - never valid)

+------------------------+-------------------+------------------+-------------------+
|       Scattering       | Single Difference | Thick Difference | Double Difference |
+========================+===================+==================+===================+
|        Forward         |       Valid       |       NEVER      |       NEVER       |
+------------------------+-------------------+------------------+-------------------+
|         Back           |       Valid       |       Valid      |       Valid       |
+------------------------+-------------------+------------------+-------------------+


Usage
-----

**Load a single file & spectrum with default difference:**

.. code-block:: python

   tof = LoadVesuvio("14188",SpectrumList=135)

   print("Number of spectra: {}".format(tof.getNumberHistograms()))

Output::

   Number of spectra: 1

**Sum runs on single spectrum with default difference:**

.. code-block:: python

   tof = LoadVesuvio("14188-14193",SpectrumList=135)

   print("Number of spectra: {}".format(tof.getNumberHistograms()))

Output::

   Number of spectra: 1

**Sum runs on a range of spectra with default difference:**

.. code-block:: python

   tof = LoadVesuvio("14188-14193",SpectrumList="135-142")

   print("Number of spectra: {}".format(tof.getNumberHistograms()))

Output::

   Number of spectra: 8

**Sum runs and spectra on a range of spectra with default difference:**

.. code-block:: python

   tof = LoadVesuvio("14188-14193",SpectrumList="135-142", SumSpectra=True)

   print("Number of spectra: {}".format(tof.getNumberHistograms()))

Output::

   Number of spectra: 1

**Sum runs and spectra on a range of spectra using single difference method:**

.. code-block:: python

   tof = LoadVesuvio("14188-14193",SpectrumList="135-142", SumSpectra=True,
                     Mode="SingleDifference")

   print("Number of spectra: {}".format(tof.getNumberHistograms()))

Output::

   Number of spectra: 1

.. categories::

.. sourcelink::
