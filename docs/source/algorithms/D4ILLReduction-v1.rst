.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the main algorithm performing the liquid diffraction reduction for data recorded with the ILL instrument D4. The algorithm's workflow diagram can be found below.

The raw data will be corrected for, in order:

1. Dead time, applied separately to monitor and detectors data
2. Imperfect banks placement, using both zero angle correction and ASCII input file correcting bank angle around the sample
3. Relative efficiency, using input ASCII file with one efficiency value per detector
4. Normalisation to a standard, either time or monitor counts.

The user has an option to not perform operations 2-4 by not providing relevant inputs or choosing `None` as the normalisation method.

The output are diffractograms as a function of scattering angle :math:`2\theta`, and momentum exchange :math:`q`. The output can be saved in the ASCII format, as .dat files, if requested.


Caching with ADS
----------------

This algorithm cleans-up the intermediate workspaces if `ClearCache` property is checked (`True` by default).

Default naming schemes are imposed to ensure smooth communication of workspace contents. While user can specify the name for the output :ref:`WorkspaceGroup <WorkspaceGroup>`,
the names of contents will consist of the name of the group as a prefix, `diffractogram` as the middle part, and the X-axis unit (either `q` or `2theta` as the suffix).


Saving output
-------------

When `ExportASCII` property is checked, the output workspaces are saved in the default save directory. Each of diffractograms is saved in a separate file with `.dat` extension.


Workflows
---------

General workflow
################

.. diagram:: D4ILLReduction-v1_wkflw.dot


.. include:: ../usagedata-note.txt

**Example - vanadium reduction at two positions**

.. testsetup:: ExD4ILLReductionVanadium

    config.setFacility('ILL')
    config.appendDataSearchSubDir('ILL/D4/')

.. testcode:: ExD4ILLReductionVanadium

    output_ws = 'vanadium_ws'

    D4ILLReduction(
      Run='387229:387230',
      OutputWorkspace=output_ws,
      NormaliseBy='Monitor',
      ExportAscii=False)

    tthAxis = mtd[output_ws][0].readX(0)
    print('{}: 2theta range: {:.3}...{:.3}A'.format(
          mtd[output_ws][0].name(), tthAxis[0], tthAxis[-1]))
    qAxis = mtd[output_ws][1].readX(0)
    print('{}: Q range: {:.3}...{:.3}A'.format(
          mtd[output_ws][1].name(), qAxis[0], qAxis[-1]))

Output:

.. testoutput:: ExD4ILLReductionVanadium

    vanadium_ws_diffractogram_2theta: 2theta range: 9.69...1.39e+02A
    vanadium_ws_diffractogram_q: Q range: 2.12...23.5A

.. testcleanup:: ExD4ILLReductionVanadium

    mtd.clear()

.. categories::

.. sourcelink::
