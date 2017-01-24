.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

ABINS is a plugin for Mantid which allows scientists to compare
experimental and theoretical inelastic neutron scattering spectra (INS).

ABINS requires file with ab-initio phonon data to perform INS analysis. Currently output data from CASTEP (.phonon)
and CRYSTAL (.out) DFT programs can be used to perform analysis. Optionally a user can provide an experimental file
with measured dynamical structure factor S in order to directly compare theoretical and experimental spectra.

After successfully performed analysis a user obtains a group workspace which stores theoretical and optionally
experimental spectra. Currently a user can produce theoretical spectrum for the given type of atom (for example for
benzene  two types of atoms: C, H) and for each quantum event (up to fourth order). A user can produce a total
theoretical spectrum for the given atom which is a sum over all considered quantum events. A user can also produce a
total spectrum for the whole considered system. Dynamical structure factor S is calculated for
all atoms in the system. If needed  a user can also include in a simulation elevated temperature .

Usage
-----

.. include:: ../usagedata-note.txt

**Example - loading CASTEP phonon data :**

.. testcode:: ABINSCastepSimple

    ABINS(DFTProgram="CASTEP", PhononFile="benzene.phonon", QuantumOrderEventsNumber=1,
    OutputWorkspace="benzene_wrk")

**Example - loading CRYSTAL phonon data :**

.. testcode:: ABINSCrystalSimple

    wrk=ABINS(DFTProgram="CRYSTAL", PhononFile="b3lyp.out", QuantumOrderEventsNumber=1)

**Example - calling ABINS with more arguments:**

.. testcode:: ABINSexplicitParameters

  ABINS(DFTprogram="CASTEP", PhononFile="benzene.phonon", ExperimentalFile="benzene_experimental.dat",
        Temperature=10, SampleForm="Powder", Instrument="TOSCA", Atoms="H", SumContributions=True,
        QuantumOrderEventsNumber="1", ScaleByCrossSection="Incoherent", OutputWorkspace= + "wrk_init")

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
