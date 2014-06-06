.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Source Code
-----------

The source code for the Python Algorithm may be viewed at:
`OSIRISDiffractionReduction.py <http://trac.mantidproject.org/mantid/browser/trunk/Code/Mantid/Framework/PythonInterface/plugins/algorithms/WorkflowAlgorithms/OSIRISDiffractionReduction.py>`__

The source code for the reducer class which is used may be viewed at:
`osiris\_diffraction\_reducer.py <http://trac.mantidproject.org/mantid/browser/trunk/Code/Mantid/scripts/Inelastic/osiris_diffraction_reducer.py>`__

Usage
-----

**Example - Running OSIRISDiffractionReduction.**

.. testcode:: ExOSIRISDiffractionReductionSimple

    ws = OSIRISDiffractionReduction(Sample="OSI89813.raw, OSI89814.raw",
                                    CalFile="osiris_041_RES10.cal",
                                    Vanadium="osi89757.raw, osi89758.raw")

    print "Created workspace with %d bins" % ws.blocksize()

Output:

.. testoutput:: ExOSIRISDiffractionReductionSimple

    Created workspace with 3867 bins

.. categories::
