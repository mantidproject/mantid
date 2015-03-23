.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates absorption corrections for a flat plate sample giving output in the
Paalman & Pings absorption factors: :math:`A_{s,s}` (correction factor for
scattering and absorption in sample), :math:`A_{s,sc}` (scattering in sample and
absorption in sample and container), :math:`A_{c,sc}` (scattering in container
and absorption in sample and container) and  :math:`A_{c,c}` (scattering and
absorption in container).

Details of the analytical method used to calculate the correction factors is
available in `RAL Technical Report 74-103
<http://purl.org/net/epubs/work/64111>`__.

Restrictions on the input workspace
###################################

The input workspace must have a fully defined instrument that has X axis units
of wavelength.

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

    print 'Correction workspaces: %s' % (', '.join(corr.getNames()))

Output:

.. testoutput:: exSampleAndCan

    Correction workspaces: corr_ass, corr_assc, corr_acsc, corr_acc

.. categories::
