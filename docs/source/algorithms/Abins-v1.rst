.. algorithm::

Credits
-------

Author:
    Krzysztof Dymkowski

Contributors:
   Sanghamitra Mukhopadhyay, Elliot Oram, Leonardo Bernasconi, Leandro Liborio

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Abins is a plugin for Mantid which allows scientists to compare experimental and theoretical inelastic neutron
scattering spectra (INS).

Abins requires a file with the ab-initio phonon data to perform INS analysis. Currently output data from CASTEP
(.phonon) and CRYSTAL (.out) DFT programs can be used to perform analysis. Optionally a user can provide an experimental
file with measured dynamical structure factor S in order to directly compare theoretical and experimental spectra. A
user can produce one dimensional INS spectrum which can be compared against TOSCA and TOSCA-like instruments.

After successfully performed analysis a user obtains a group workspace which stores theoretical and optionally
experimental spectra. Currently a user can produce theoretical spectrum for the given type of atom (for example for
benzene  two types of atoms: C, H) and for each quantum event (up to fourth order). A user can produce a total
theoretical spectrum for the given atom which is a sum over all considered quantum events for that atom. A user can
also produce a total spectrum for the whole considered system. Dynamical structure factor S is calculated for
all atoms in the system. If needed  a user can also include in a simulation elevated temperature.

A description about the implemented working equations can be found :ref:`here <DynamicalStructureFactorFromAbInitio>`.

Abins is in constant development and suggestions
how to make it better are very welcome. For any suggestions of enhancements please contact
Dr. Sanghamitra Mukhopadhyay (sanghamitra.mukhopadhyay@stfc.ac.uk).

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

    wrk=Abins(AbInitioProgram="CRYSTAL",VibrationalOrPhononFile="b3lyp.out", QuantumOrderEventsNumber="1")

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
                      Atoms="H", SumContributions=True, QuantumOrderEventsNumber="1", ScaleByCrossSection="Incoherent")

    for name in wrk_verbose.getNames():
        print(name)

Output:

.. testoutput:: AbinsexplicitParameters

    experimental_wrk
    wrk_verbose_total
    wrk_verbose_H_total
    wrk_verbose_H

.. categories::

.. sourcelink::
  :cpp: None
  :h: None


References
----------

.. [1] K. Dymkowski, S. F. Parker, F. Fernandez-Alonso and S. Mukhopadhyay,  “AbINS: The modern software for INS interpretation” , Physica B, doi:10.1016/j.physb.2018.02.034 (2018).
