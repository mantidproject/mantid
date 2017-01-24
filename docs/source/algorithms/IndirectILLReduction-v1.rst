.. algorithm::

.. summary::

.. alias::

.. properties::

.. warning::

   This algorithm is deprecated (20-Nov-2016). Please, use :ref:`IndirectILLReductionQENS <algm-IndirectILLReductionQENS>` instead.

Description
-----------

A workflow algorithm to perform a data reduction for Indirect ILL instruments.

Note that currently only IN16B is supported.

Mirror Mode
~~~~~~~~~~~

When IN16B records data in mirror mode the spectra for the acceleration and
deceleration phase of the Doppler drive are recorded separately, the result is
each spectra containing two regions for the same energy range.

Enabling MirrorMode on this algorithm will split the data for each spectrum into
two separate spectra, these form the "left" and "right" workspaces that are
reduced independently and then summed.

Workflow
--------

.. diagram:: IndirectILLReduction-v1_wkflw.dot

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

    Reduced workspace has 18 spectra
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
    Reduced workspace has 18 spectra
    Reduced left workspace has 18 spectra
    Reduced right workspace has 18 spectra

.. categories::

.. sourcelink::
