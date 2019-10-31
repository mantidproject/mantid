.. algorithm::

Credits
-------

Author:
    Krzysztof Dymkowski

Contributors:
   Sanghamitra Mukhopadhyay, Elliot Oram, Leonardo Bernasconi, Leandro Liborio, Adam Jackson

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Abins is a plugin for Mantid which allows scientists to compare experimental and theoretical inelastic neutron
scattering spectra (INS).

Abins requires a file with the ab-initio phonon data to perform INS analysis. Currently output data from CASTEP
(.phonon), CRYSTAL (.out), GAUSSIAN (.log) or DMOL3 (.outmol) DFT programs can be used to perform analysis.
Optionally, a user can provide an experimental file with measured dynamical structure factor S in order to directly
compare theoretical and experimental spectra.
A user can produce one dimensional INS spectrum which can be compared against TOSCA and TOSCA-like instruments;
Abins uses a semi-empirical powder averaging model to account for q- and energy-dependent phenomena in this system.
The user-input temperature value is included in a Debye-Waller term, recreating the intensity fall-off with increasing
wavelength.

After successfully-performed analysis a user obtains a Mantid Workspace Group which stores theoretical spectra (and,
optionally, experimental data).
Currently a user can produce theoretical spectra for given atoms (e.g. 'atom_1', the first atom listed in the input
data) or types of atom (for example for benzene two element symbols: C, H) and for each quantum event (up to fourth
order).
Total theoretical spectra can also be generated, summing over all considered quantum events for that atom or element.
The user can also produce a total spectrum for the whole considered system.
The dynamical structure factor S is calculated for all atoms in the system and results are cached, so if no settings
have been changed then subsequent runs of Abins can quickly create more Mantid Workspaces without re-calculating any
spectra.

More information about the implemented working equations can be found :ref:`here <DynamicalStructureFactorFromAbInitio>`.

Abins is in constant development and suggestions for improvements are very welcome. For any such contributions please
contact Dr. Sanghamitra Mukhopadhyay (sanghamitra.mukhopadhyay@stfc.ac.uk).

If Abins is used as part of your data analysis routines, please cite the relevant reference [1]_.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - loading CASTEP phonon data:**

.. testcode:: AbinsCastepSimple

    benzene_wrk = Abins(AbInitioProgram="CASTEP", VibrationalOrPhononFile="benzene.phonon",
                        QuantumOrderEventsNumber="1")


    for name in benzene_wrk.getNames():
        print(name)

Output:

.. testoutput:: AbinsCastepSimple

    benzene_wrk_C_total
    benzene_wrk_C
    benzene_wrk_H_total
    benzene_wrk_H

**Example - loading CRYSTAL phonon data:**

.. testcode:: AbinsCrystalSimple

    wrk=Abins(AbInitioProgram="CRYSTAL", VibrationalOrPhononFile="b3lyp.out", QuantumOrderEventsNumber="1")

    for name in wrk.getNames():
        print(name)

Output:

.. testoutput:: AbinsCrystalSimple

    wrk_C_total
    wrk_C
    wrk_H_total
    wrk_H
    wrk_N_total
    wrk_N
    wrk_Na_total
    wrk_Na
    wrk_O_total
    wrk_O

**Example - calling AbINS with more arguments:**

.. testcode:: AbinsexplicitParameters

    wrk_verbose=Abins(AbInitioProgram="CASTEP", VibrationalOrPhononFile="benzene.phonon",
                      ExperimentalFile="benzene_experimental.dat",
                      TemperatureInKelvin=10, BinWidthInWavenumber=1.0, SampleForm="Powder", Instrument="TOSCA",
                      Atoms="H, atom1, atom2", SumContributions=True, QuantumOrderEventsNumber="1", ScaleByCrossSection="Incoherent")

    for name in wrk_verbose.getNames():
        print(name)

Output:

.. testoutput:: AbinsexplicitParameters

    experimental_wrk
    wrk_verbose_total
    wrk_verbose_H_total
    wrk_verbose_H
    wrk_verbose_atom_1_total
    wrk_verbose_atom_1
    wrk_verbose_atom_2_total
    wrk_verbose_atom_2

.. categories::

.. sourcelink::
  :cpp: None
  :h: None


References
----------

.. [1] K. Dymkowski, S. F. Parker, F. Fernandez-Alonso and S. Mukhopadhyay,  “AbINS: The modern software for INS interpretation” , Physica B, doi:10.1016/j.physb.2018.02.034 (2018).
