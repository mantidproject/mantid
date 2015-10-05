.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates absorption corrections for a cylindrical or annular sample giving
output in the Paalman & Pings absorption factors: :math:`A_{s,s}` (correction
factor for scattering and absorption in sample), :math:`A_{s,sc}` (scattering in
sample and absorption in sample and container), :math:`A_{c,sc}` (scattering in
container and absorption in sample and container) and  :math:`A_{c,c}`
(scattering and absorption in container).

Restrictions on the input workspace
###################################

The input workspace must have a fully defined instrument that has X axis units
of wavelength.

Usage
-----

**Example:**

.. testcode:: ExCylinderPaalmanPingsCorrection

    # Create a sample workspace
    sample = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1,
                                   XUnit='Wavelength',
                                   XMin=6.8, XMax=7.9,
                                   BinWidth=0.1)

    # Copy and scale it to make a can workspace
    can = CloneWorkspace(InputWorkspace=sample)
    can = Scale(InputWorkspace=can, Factor=1.2)

    # Calculate absorption corrections
    corr = CylinderPaalmanPingsCorrection(SampleWorkspace=sample,
                                          SampleChemicalFormula='H2-O',
                                          SampleInnerRadius=0.05,
                                          SampleOuterRadius=0.1,
                                          CanWorkspace=can,
                                          CanChemicalFormula='V',
                                          CanOuterRadius=0.15,
                                          BeamHeight=0.1,
                                          BeamWidth=0.1,
                                          StepSize=0.002,
                                          Emode='Indirect',
                                          Efixed=1.845)

    print 'Correction workspaces: %s' % (', '.join(corr.getNames()))

Output:

.. testoutput:: ExCylinderPaalmanPingsCorrection

    Correction workspaces: corr_ass, corr_assc, corr_acsc, corr_acc

.. categories::

.. sourcelink::
