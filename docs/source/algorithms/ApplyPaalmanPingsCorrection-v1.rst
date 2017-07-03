.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Applies absorption corrections calculated in the Paalman and Pings
absorption factor format. The various partial absorption factors are
denoted as :math:`A_{a,b}` where the subscript :math:`a` denotes the
component the neutron is scattered from and :math:`b` denotes the
component where the neutron is absorbed. The various partial
absorption factors, and what their names must contain in the
``CorrectionsWorkspace`` group are detailed in the table below. If any
of the partial absorption factors are not supplied, they are assumed
to be one.

================ ============ ==================== ==============
    Symbol       Scatter From Absorbed By          Workspace Name
================ ============ ==================== ==============
:math:`A_{s,s}`  sample       sample               ``ass``
---------------- ------------ -------------------- --------------
:math:`A_{s,sc}` sample       sample and container ``assc``
---------------- ------------ -------------------- --------------
:math:`A_{c,c}`  container    container            ``acc``
---------------- ------------ -------------------- --------------
:math:`A_{c,sc}` container    sample and container ``acsc``
================ ============ ==================== ==============

This algorithm can be used to apply absorption corrections calculated
with either the :ref:`CylinderPaalmanPingsCorrection
<algm-CylinderPaalmanPingsCorrection>` and
:ref:`FlatPlatePaalmanPingsCorrection
<algm-FlatPlatePaalmanPingsCorrection>` algorithms as well as the
legacy indirect calculate corrections routine, providing that the
sample and container are first converted to wavelength and the
corrections are interpolated to match the sample as demonstrated in
the example below.

All workspaces are converted into wavelength using the appropriate
mode of :ref:`ConvertUnits <algm-ConvertUnits>`. Then
``CanShiftFactor`` is added to wavelength of the
``CanWorkspace``. Then the following equation is performed:

.. math:: I_s = \frac{1}{A_{s,sc}} \left( I_{sc}^E - I_c^E K_c \frac{A_{c,sc}}{A_{c,c}} \right)

The variables that are not defined above are

================ ======================== =======
Variable         Parameter Name           Default
================ ======================== =======
:math:`I_s`      ``OutputWorkspace``      N/A
---------------- ------------------------ -------
:math:`I_{sc}^E` ``SampleWorkspace``      N/A
---------------- ------------------------ -------
:math:`I_{c}^E`  ``CanWorkspace``         0
---------------- ------------------------ -------
:math:`K_c`      ``CanScaleFactor``       1
---------------- ------------------------ -------
:math:`A_{a,b}`  ``CorrectionsWorkspace`` 1
================ ======================== =======

The workflow diagrams below are another representation of the equation
above with the simplifications for when the various terms are missing
or one.

Workflow
--------

Depending on the input workspaces provided to the algorithm it may operate in
one of three ways, each of which is described on a separate workflow diagram.

Container Scale Only
====================

In the case where only a container workspace and no correction factors are
provided.

.. diagram:: ApplyPaalmanPingsCorrection-v1_canscaleonly_wkflw.dot

Sample Corrections Only
=======================

In the case where only correction factors and no container workspace is
provided.

.. diagram:: ApplyPaalmanPingsCorrection-v1_samplecorrectiononly_wkflw.dot

Full Corrections
================

In the case where both a container workspace and correction factors are
provided.

.. diagram:: ApplyPaalmanPingsCorrection-v1_fullcorrection_wkflw.dot

Usage
-----

**Example: using with legacy indirect corrections data**

.. testcode:: exSampleAndCanIRISLegacyCorrections

    # Load the sample and can
    sample_ws = Load('irs26176_graphite002_red.nxs')
    can_ws = Load('irs26173_graphite002_red.nxs')

    # Convert sample and container workspaces to wavelength
    sample_ws = ConvertUnits(InputWorkspace=sample_ws,
                             Target='Wavelength',
                             EMode='Indirect',
                             EFixed=1.845)
    can_ws = ConvertUnits(InputWorkspace=can_ws,
                          Target='Wavelength',
                          EMode='Indirect',
                          EFixed=1.845)

    # Load the corrections workspace
    corrections_ws = Load('irs26176_graphite002_cyl_Abs.nxs')

    # Interpolate each of the correction factor workspaces to match the
    # binning of the smaple
    # Required to use corrections from the old indirect calculate
    # corrections routines
    for factor_ws in corrections_ws:
        SplineInterpolation(WorkspaceToMatch=sample_ws,
                            WorkspaceToInterpolate=factor_ws,
                            OutputWorkspace=factor_ws,
                            OutputWorkspaceDeriv='')

    corr = ApplyPaalmanPingsCorrection(SampleWorkspace=sample_ws,
                                       CorrectionsWorkspace=corrections_ws,
                                       CanWorkspace=can_ws)

    print 'Corrected workspace has %d spectra over %d bins' % (
          corr.getNumberHistograms(), corr.blocksize())

    print 'Type of correction applied: %s' % (
          corr.getRun()['corrections_type'].value)

Output:

.. testoutput:: exSampleAndCanIRISLegacyCorrections

    Corrected workspace has 10 spectra over 1905 bins
    Type of correction applied: sample_and_can_corrections

Related Algorithms
------------------

:ref:`FlatPlatePaalmanPingsCorrection <algm-FlatPlatePaalmanPingsCorrection>`
calculates the partial absorption factors in flat plate geometry

:ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`
calculates the partial absorption factors in cylindrical plate geometry

References
----------

#. H. H. Paalman, and C. J. Pings. *Numerical Evaluation of X‐Ray
   Absorption Factors for Cylindrical Samples and Annular Sample Cells*,
   Journal of Applied Physics **33.8** (1962) 2635–2639
   `doi: 10.1063/1.1729034 <http://dx.doi.org/10.1063/1.1729034>`_

.. categories::

.. sourcelink::
