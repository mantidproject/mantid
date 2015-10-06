.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates phonon densities of states, Raman and IR spectrum from the
output of CASTEP code obtained in the form of *.phonon* and *.castep* files.

The PeakWidth property may be passed a function containg the variable "energy"
(e.g. *0.1*energy*) to set the FWHM of the peak as a function of the energy
(centre point of the peak). This can be useful for comparison with experimental
data by allowing the peak width to change according to the resolution of the
instrument.

If the IonTable spectrum type is used then the output workspace will be
a table workspace containing each ion that is present in a *.phonon* file.

If the BondTable spectrum type is used then the output workspace will be
a table workspace containing details of the bonds defined in the *.castep*
file.

.. note::
  The BondAnalysis mode is an experimental feature which currently can not be
  compared to alternative software (e.g. Crystal), this mode should be used for
  quick approximations only.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - loading data from phonon & castep files:**

.. testcode:: ExSimulatedDensityOfStatesSimple

    # Loading the same data from a castep and phonon file
    phonon_ws = SimulatedDensityOfStates(PHONONFile='squaricn.phonon')
    castep_ws = SimulatedDensityOfStates(CASTEPFile='squaricn.castep')

    print CheckWorkspacesMatch(phonon_ws, castep_ws)

Output:

.. testoutput:: ExSimulatedDensityOfStatesSimple

    Success!

**Example - loading partial contributions of ions:**

.. testcode:: ExSimulatedDensityOfStatesPartial

    squaricn = SimulatedDensityOfStates(PHONONFile='squaricn.phonon',
                                        Ions=['H', 'C', 'O'])

    for name in squaricn.getNames():
      print name

Output:

.. testoutput:: ExSimulatedDensityOfStatesPartial

    squaricn_H
    squaricn_C
    squaricn_O

**Example - loading summed partial contributions of ions:**

.. testcode:: ExSimulatedDensityOfStatesPartialSummed

    sum_ws = SimulatedDensityOfStates(PHONONFile='squaricn.phonon',
                                      Ions=['H', 'C', 'O'],
                                      SumContributions=True)
    total_ws = SimulatedDensityOfStates(PHONONFile='squaricn.phonon')

    print CheckWorkspacesMatch(total_ws, sum_ws, Tolerance=1e-12)

Output:

.. testoutput:: ExSimulatedDensityOfStatesPartialSummed

    Success!

**Example - Getting the list of ions in a phonon file:**

.. testcode:: ExSimulatedDensityOfStatesIonTable

    ion_ws = SimulatedDensityOfStates(PHONONFile='squaricn.phonon',
                                      SpectrumType='IonTable')
    print ', '.join(ion_ws.column('Species'))

Output:

.. testoutput:: ExSimulatedDensityOfStatesIonTable

    H, H, H, H, C, C, C, C, C, C, C, C, O, O, O, O, O, O, O, O

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
