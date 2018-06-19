.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
^^^^^^^^^^^

Creates a ``.maud`` calibration file from a set of focused diffraction
workspaces and a GSAS calibration file.

MAUD Format
^^^^^^^^^^^

``.maud`` is a text-based format used to convert the data in a GDA
file (see :ref:`SaveGDA <algm-SaveGDA>`) from TOF to D-spacing in
`MAUD <http://maud.radiographema.eu/>`_. The algorithm uses a template
to produce its output, which lives in
``scripts/Diffraction/isis_powder/gem_routines/maud_param_template.maud``.

The parameters of interest in the ``.maud`` file are:
- Bank IDs - essentially just a label for each spectrum in MAUD
- Diffractometer constants (also called conversion factors, or
  sometimes DIFC values) **DIFC**, **DIFA** and **TZERO**. These are
  explained in section 1 of `Refinement of time of flight Profile
  Parameters in GSAS
  <https://www.isis.stfc.ac.uk/Pages/refinement-of-profile-parameters-with-polaris-data.pdf>`_
  [RonSmith]_
- Scattering angles for each bank, **theta** and **eta** (more
  normally called **phi**
- Profile coefficients for GEM's chosen peak shape (GSAS TOF function
  type 1) **alpha-0**, **alpha-1**, **beta-0**, **beta-1**,
  **sigma-0**, **sigma-1** and **sigma-2**. These are explained on
  page 143 of the `GSAS Manual
  <http://www.ccp14.ac.uk/ccp/ccp14/ftp-mirror/gsas/public/gsas/manual/GSASManual.pdf>`_
  [GSASManual]_
- There are also parameters for a second function, GSAS TOF function
  type 2, which are zeroed

Bank Grouping
^^^^^^^^^^^^^

It should be noted that calibration parameters are not given for every
bank, as generating such a file would be impractical, given the data
we get from texture experiments on GEM.

Instead we assign each of the 160 banks the parameters from one of the
6 banks used when focusing GEM data normally. This algorithm's 'sister
algorithm', :ref:`SaveGDA <algm-SaveGDA>` applies a D to TOF
conversion using the same parameters per bank (essentially faking the
time-of-flight). See :ref:`save_gda_tof` for more details on this.

*References*:

.. [RonSmith] Smith, R. "Refinement of time of flight Profile
              Parameters in GSAS"

.. [GSASManual] Larson, A. C. & Von Dreele, R. B (2004). "General
		Structure Analysis System (GSAS)", Los Alamos National
		Laboratory Report LAUR 86-748

Usage
-----

.. testcode:: SaveGEMMAUDParamFile

   import os

   def collect_parameter(param_header, file_contents):
       file_index = file_contents.index(param_header) + 1
       param_values = []

       while file_contents[file_index]:
           param_values.append(float(file_contents[file_index]))
           file_index += 1

       return param_values

   # Banks 1 to 4 of a previous texture focus in isis_powder
   # We don't use the full 160 banks as the test becomes too slow
   input_group = Load(Filename="GEM61785_D_texture_banks_1_to_4.nxs",
                      OutputWorkspace="SaveGEMMAUDParamFiletest_GEM61785")

   output_file = os.path.join(config["defaultsave.directory"], "GEM61785.maud")
   SaveGEMMAUDParamFile(InputWorkspace=input_group,
                        OutputFilename=output_file,
			GSASParamFile="GEM_PF1_PROFILE.IPF",
			# Assign spectra 1, 2 and 3 to bank 2 in calib file,
                        # and spectrum 4 to bank 3
			GroupingScheme=[2, 2, 2, 3])

   with open(output_file) as f:
       file_contents = f.read().split("\n")

   difcs = collect_parameter("_instrument_bank_difc", file_contents)
   print("DIFC values: " + " ".join("{:.2f}".format(difc) for difc in difcs))

   thetas = collect_parameter("_instrument_bank_tof_theta", file_contents)
   print("Theta values: " + " ".join("{:.2f}".format(theta) for theta in thetas))

.. testcleanup:: SaveGEMMAUDParamFile

   os.remove(output_file)
   mtd.remove("SaveGEMMAUDParamFiletest_GEM61785")

Output:

.. testoutput:: SaveGEMMAUDParamFile

   DIFC values: 1468.19 1468.19 1468.19 2788.34
   Theta values: 9.12 8.16 8.04 9.06
