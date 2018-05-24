.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a :ref:`PeaksWorkspace <PeaksWorkspace>` and a set of lattice parameters,
attempts to tag each peak with a HKL value by comparing d-spacings between
potential HKL matches and the peaks as well as angles between Q vectors.

Usage Notes
-----------

This algorithm does not generate a :ref:`UB matrix <Lattice>`, it will only index peaks.
Run :ref:`CalculateUMatrix <algm-CalculateUMatrix>` algorithm after executing
this algorithm in order to attach a :ref:`UB matrix <Lattice>` onto the sample. The
:ref:`CopySample <algm-CopySample>` algorithm will allow this :ref:`UB matrix <Lattice>`
to be transfered between workspaces.

Usage
-----

**Example - a simple example of IndexPeaks**

.. include:: ../usagedata-note.txt

.. testcode:: ExIndexSXPeaksSimple

   # Load Peaks
   ws=LoadIsawPeaks(Filename='TOPAZ_3007.peaks')
 
   # Run Algorithm
   IndexSXPeaks(PeaksWorkspace=ws, a=8.605819, b=11.935925, c=11.941813, alpha=107.429088, beta=98.752912, gamma=98.951193, dTolerance=0.15)

   # Print number of indexed peaks
   ws=FilterPeaks(InputWorkspace=ws, FilterVariable='h^2+k^2+l^2', FilterValue=0, Operator='>')
   print("Number of Indexed Peaks: {:d}".format(ws.getNumberPeaks()))

Output:

.. testoutput:: ExIndexSXPeaksSimple
       
   Number of Indexed Peaks: 43

Related Algorithms
------------------

:ref:`IndexPeaks <algm-IndexPeaks>`
will use UB inverse to index peaks given a PeaksWorkspace with a UB matrix stored with the sample

.. categories::

.. sourcelink::
