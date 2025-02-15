.. algorithm::

Credits
-------

Author:
    Adam Jackson, Krzysztof Dymkowski

Contributors:
   Sanghamitra Mukhopadhyay, Elliot Oram, Leonardo Bernasconi, Leandro Liborio

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Abins is a plugin for Mantid which allows scientists to compare experimental and theoretical inelastic neutron
scattering spectra (INS).

Abins requires data from existing lattice dynamics calculations to determine the phonon mode frequencies and displacements. These are used to compute INS intensities; higher-order phonons may be approximated as combinations of fundamental modes.
Several file formats are supported for frequency and displacement data at given :math:`\mathbf{q}`-points. Alternatively, force-constants data can be provided and a fine :math:`\mathbf{q}`-point mesh will be sampled automatically.
Supported codes include CASTEP (.phonon), CRYSTAL (.out), DMOL3 (.outmol), GAUSSIAN (.log), PHONOPY (.yaml), VASP (.xml) as well as some pre-processed JSON (.json) formats. For more information about the files, features and quirks of these codes see :ref:`AbinsDataFormats`.

The user may also provide an experimental file with measured dynamical structure factor S in order to directly
compare theoretical and experimental spectra.
Abins produces one dimensional INS spectrum that can be compared against TOSCA, IN1-LAGRANGE and similar instruments;
a semi-empirical powder averaging model accounts for q- and energy-dependent intensity behaviour in this system.
The user-input temperature value is included in a Debye-Waller term, recreating the intensity fall-off with increasing
wavelength.
More information about the implemented working equations can be found :ref:`here <DynamicalStructureFactorFromAbInitio>`.

After successful analysis a user obtains a Mantid Workspace Group which stores theoretical spectra (and,
optionally, experimental data).
Currently a user can produce theoretical spectra for given atoms (e.g. 'atom_1', the first atom listed in the input
data) or types of atom (for example for benzene two element symbols: C, H) and for each quantum event (up to tenth
order).
Total theoretical spectra can also be generated, summing over all considered quantum events for that atom or element.
The user can also produce a total spectrum for the whole considered system.
The dynamical structure factor S is calculated for all atoms in the system and results are cached, so if no settings
have been changed then subsequent runs of Abins can quickly create more Mantid Workspaces without re-calculating any
spectra.

Abins is in constant development and suggestions for improvements are very welcome. For any such contributions please
contact Dr. Sanghamitra Mukhopadhyay.
If you are developing or modifying Abins, see the :ref:`AbinsImplementation` notes which outline some of the key conventions, practices and pitfalls.

If Abins is used as part of your data analysis routines, please cite the relevant reference [1]_.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - loading CASTEP phonon data:**

.. testsetup:: *

    # On CI defaultsave.directory may be set to an inappropriate
    # value, causing the input validator to raise an error.
    # Set it somewhere that is guaranteed to be suitable.

    from tempfile import TemporaryDirectory
    from mantid.kernel import ConfigService

    test_dir = TemporaryDirectory()

    initial_defaultsave = ConfigService.getString("defaultsave.directory")
    ConfigService.setString("defaultsave.directory", test_dir.name)

.. testcleanup:: *

    # Restore the original defaultsave.directory to avoid surprises
    # when running doctests locally.

    test_dir.cleanup()
    ConfigService.setString("defaultsave.directory", initial_defaultsave)

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

Here the cache file is directed to a temporary directory and will be cleaned up automatically.

.. testcode:: AbinsExplicitParameters

    from tempfile import TemporaryDirectory

    with TemporaryDirectory() as tmp_dir:

        wrk_verbose=Abins(AbInitioProgram="CASTEP", VibrationalOrPhononFile="benzene.phonon",
                          ExperimentalFile="benzene_experimental.dat",
                          TemperatureInKelvin=10, BinWidthInWavenumber=1.0, SampleForm="Powder", Instrument="TOSCA",
                          Atoms="H, atom1, atom2", SumContributions=True, QuantumOrderEventsNumber="1", ScaleByCrossSection="Incoherent",
                          CacheDirectory=tmp_dir)

    for name in wrk_verbose.getNames():
        print(name)

Output:

.. testoutput:: AbinsExplicitParameters

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
