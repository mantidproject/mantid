.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Function to save a .ins file - one of the input files required for the SHELX crystallographic code.
The file contains information about the sample (spacegroup, lattice parameters and atoms present) and the format
of the reflections file.

The ``InputWorkspace`` must have a UB set (from which the lattice parameters are retrieved) and a sample material
(from which the atoms and number of formula units are taken). The spacegroup can be taken given as an input argument to
the function, or from the crystal structure if one has been set on the workspace.

By default it is assumed that atoms have a ratio of isotopes seen in nature, however if
``UseNaturalIsotopicAbundances=False`` the scattering cross-sections defined for the isotopes specified in
the sample material will be used. In this case there are other atomic properties that are output, one of these is the
covalent radius (which SHELX uses for the purposes of outputting bonds/geometry information) which is set to be
1 Angstrom (if ``UseNaturalIsotopicAbundances=True`` SHELX will look up the covalent radii of atoms itself).


Usage
-----

**Example:**

.. code-block:: python

    from mantid.simpleapi import *

    ws = CreateSampleWorkspace(OutputWorkspace='ws', NumBanks=1, BankPixelWidth=1, BinWidth=20000)  # 1 bin
    SetUB(Workspace=ws, a=7.6508, b=13.2431, c=11.6243, alpha=90, beta=104.1183, gamma=90)
    ndensity = 4 / ws.sample().getOrientedLattice().volume()  # number density with 4 formula units per u.c
    SetSample(InputWorkspace=self.ws, Material={'ChemicalFormula': 'C12 H9 N3 O2 S1',
              'SampleNumberDensity': ndensity})
    SaveINS(InputWorkspace=self.ws, Filename='SHELX.ins', Spacegroup='P 1 21/n 1', UseNaturalIsotopicAbundances=True)


.. categories::

.. sourcelink::
