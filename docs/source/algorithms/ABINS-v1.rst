.. algorithm::

Credits
-------

Author:
    Krzysztof Dymkowski

Contributors:
    Leonardo Bernasconi (main author of module LoadCRYSTAL), Leandro Liborio (provider of ab-initio data for benchmarks)

.. summary::

.. alias::

.. properties::

Description
-----------

ABINS is a plugin for Mantid which allows scientists to compare experimental and theoretical inelastic neutron
scattering spectra (INS).

ABINS requires a file with the ab-initio phonon data to perform INS analysis. Currently output data from CASTEP
(.phonon) and CRYSTAL (.out) DFT programs can be used to perform analysis. Optionally a user can provide an experimental
file with measured dynamical structure factor S in order to directly compare theoretical and experimental spectra. A
user can produce one dimension  INS spectrum which can be compared against TOSCA and TOSCA-like instruments. If
instrument TwoDMap is chosen one can produce 2D INS spectrum.

After successfully performed analysis a user obtains a group workspace which stores theoretical and optionally
experimental spectra. Currently a user can produce theoretical spectrum for the given type of atom (for example for
benzene  two types of atoms: C, H) and for each quantum event (up to fourth order). A user can produce a total
theoretical spectrum for the given atom which is a sum over all considered quantum events for that atom. A user can
also produce a total spectrum for the whole considered system. Dynamical structure factor S is calculated for
all atoms in the system. If needed  a user can also include in a simulation elevated temperature.

ABINS is in constant development and suggestions
how to make it better are very welcome. For any suggestions of enhancements please contact
Krzysztof Dymkowski (krzysztof.dymkowski@stfc.ac.uk).


Usage
-----

.. include:: ../usagedata-note.txt

**Example - loading CASTEP phonon data :**

.. testcode:: ABINSCastepSimple

    benzene_wrk = ABINS(DFTProgram="CASTEP", PhononFile="benzene.phonon", QuantumOrderEventsNumber="1")


    for name in benzene_wrk.getNames():
        print name

Output:

.. testoutput:: ABINSCastepSimple

    benzene_wrk_H_total
    benzene_wrk_H
    benzene_wrk_C_total
    benzene_wrk_C

**Example - loading CRYSTAL phonon data :**

.. testcode:: ABINSCrystalSimple

    wrk=ABINS(DFTProgram="CRYSTAL", PhononFile="b3lyp.out", QuantumOrderEventsNumber="1")

    for name in wrk.getNames():
        print name

Output:

.. testoutput:: ABINSCrystalSimple

    wrk_Na_total
    wrk_Na
    wrk_C_total
    wrk_C
    wrk_H_total
    wrk_H
    wrk_O_total
    wrk_O
    wrk_N_total
    wrk_N

**Example - calling ABINS with more arguments:**

.. testcode:: ABINSexplicitParameters

    wrk_verbose=ABINS(DFTprogram="CASTEP", PhononFile="benzene.phonon", ExperimentalFile="benzene_experimental.dat",
                    Temperature=10, SampleForm="Powder", Instrument="TOSCA", Atoms="H", SumContributions=True,
                    QuantumOrderEventsNumber="1", ScaleByCrossSection="Incoherent")

    for name in wrk_verbose.getNames():
        print name

Output:

.. testoutput:: ABINSexplicitParameters

    experimental_wrk
    wrk_verbose_total
    wrk_verbose_H_total
    wrk_verbose_H

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
