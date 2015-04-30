.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

A workflow algorithm to perform a data reduction for Indirect ILL instruments.

Note that currently only IN16B is supported.

Usage
-----

**Example - Running IndirectILLReduction**

.. testcode:: ExIndirectILLReduction

    IndirectILLReduction(Run='ILLIN16B_034745.nxs',
                         RawWorkspace='raw_workspace',
                         ReducedWorkspace='reduced_workspace')

    print "Reduced workspace has %d spectra" % mtd['reduced_workspace'].getNumberHistograms()
    print "Raw workspace has %d spectra" % mtd['raw_workspace'].getNumberHistograms()

Output:

.. testoutput:: ExIndirectILLReduction

    Reduced workspace has 24 spectra
    Raw workspace has 2057 spectra

**Example - Running IndirectILLReduction in mirror mode**

.. testcode:: ExIndirectILLReductionMirrorMode

    IndirectILLReduction(Run='ILLIN16B_034745.nxs',
                         RawWorkspace='raw_workspace',
                         ReducedWorkspace='reduced_workspace',
                         LeftWorkspace='reduced_workspace_left',
                         RightWorkspace='reduced_workspace_right',
                         MirrorMode=True)

    print "Raw workspace has %d spectra" % mtd['raw_workspace'].getNumberHistograms()
    print "Reduced workspace has %d spectra" % mtd['reduced_workspace'].getNumberHistograms()
    print "Reduced left workspace has %d spectra" % mtd['reduced_workspace_left'].getNumberHistograms()
    print "Reduced right workspace has %d spectra" % mtd['reduced_workspace_right'].getNumberHistograms()

Output:

.. testoutput:: ExIndirectILLReductionMirrorMode

    Raw workspace has 2057 spectra
    Reduced workspace has 24 spectra
    Reduced left workspace has 24 spectra
    Reduced right workspace has 24 spectra

.. categories::
