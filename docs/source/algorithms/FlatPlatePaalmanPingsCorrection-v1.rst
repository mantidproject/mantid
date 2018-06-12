.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates absorption corrections for an infinite flat plate sample giving output in the
Paalman and Pings absorption factors:
:math:`A_{s,s}` (scattering and absorption in sample),
:math:`A_{s,sc}` (scattering in sample and absorption in sample and container),
:math:`A_{c,sc}` (scattering in container and absorption in sample and container)
and  :math:`A_{c,c}` (scattering and absorption in container).

Details of the analytical method used to calculate the correction factors is
available in `RAL Technical Report 74-103
<http://purl.org/net/epubs/work/64111>`__.

Restrictions on the input workspace
###################################

- The input workspaces must have a fully defined instrument.
- In the energy transfer modes other than **Efixed** they have to have X axis units of wavelength.

Efixed mode
###########

- In **Efixed** mode, the correction will be computed for a single wavelength number derived from the analyser or monochromator energy,
  which by default will be attempted to be read from the instrument parameters (named **Efixed**), but can also be overridden in the **Efixed** input property.
  In this case, the `NumberWavelengths` and `Interpolate` options will be ignored.

Workflow
--------

.. diagram:: FlatPlatePaalmanPingsCorrection-v1_wkflw.dot


Usage
-----

**Example:**

.. testcode:: exSampleAndCan

    # Create a sample workspace
    sample = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1,
                                   XUnit='Wavelength',
                                   XMin=6.8, XMax=7.9,
                                   BinWidth=0.1)

    # Copy and scale it to make a can workspace
    can = CloneWorkspace(InputWorkspace=sample)
    can = Scale(InputWorkspace=can, Factor=1.2)

    # Calculate absorption corrections
    corr = FlatPlatePaalmanPingsCorrection(SampleWorkspace=sample,
                                           SampleChemicalFormula='H2-O',
                                           SampleThickness=0.1,
                                           SampleAngle=45,
                                           CanWorkspace=can,
                                           CanChemicalFormula='V',
                                           CanFrontThickness=0.01,
                                           CanBackThickness=0.01,
                                           Emode='Indirect',
                                           Efixed=1.845)

    print('Correction workspaces: %s' % (', '.join(corr.getNames())))

Output:

.. testoutput:: exSampleAndCan

    Correction workspaces: corr_ass, corr_assc, corr_acsc, corr_acc

Related Algorithms
------------------

:ref:`ApplyPaalmanPingsCorrection <algm-ApplyPaalmanPingsCorrection>`
will correctly apply the partial absorption factors

.. categories::

.. sourcelink::
