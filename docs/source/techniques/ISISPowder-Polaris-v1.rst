.. _isis-powder-diffraction-polaris-ref:

=====================================================
ISIS Powder Diffraction Scripts - POLARIS Reference
=====================================================

.. contents:: Table of Contents
    :local:

.. _creating_polaris_object_isis-powder-diffraction-ref:

Creating POLARIS Object
------------------------
This method assumes you are familiar with the concept of objects in Python.
If not more details can be read here: :ref:`intro_to_objects-isis-powder-diffraction-ref`

To create a POLARIS object the following parameters are required:

- :ref:`calibration_directory_polaris_isis-powder-diffraction-ref`
- :ref:`output_directory_polaris_isis-powder-diffraction-ref`
- :ref:`user_name_polaris_isis-powder-diffraction-ref`

Optionally a configuration file may be specified if one exists
using the following parameter:

- :ref:`config_file_polaris_isis-powder-diffraction-ref`

See :ref:`configuration_files_isis-powder-diffraction-ref`
on YAML configuration files for more details

Example
^^^^^^^

..  code-block:: python

  from isis_powder import Polaris

  calibration_dir = r"C:\path\to\calibration_dir"
  output_dir = r"C:\path\to\output_dir"

  polaris_example = Polaris(calibration_directory=calibration_dir,
                            output_directory=output_dir,
                            user_name="Mantid")

  # Optionally we could provide a configuration file like so
  # Notice how the file name ends with .yaml
  config_file_path = r"C:\path\to\config_file.yaml
  polaris_example = Polaris(config_file=config_file_path,
                            user_name="Mantid", ...)

Methods
--------
The following methods can be executed on a POLARIS object:

- :ref:`create_vanadium_polaris_isis-powder-diffraction-ref`
- :ref:`focus_polaris_isis-powder-diffraction-ref`
- :ref:`set_sample_polaris_isis-powder-diffraction-ref`

For information on creating a POLARIS object see:
:ref:`creating_polaris_object_isis-powder-diffraction-ref`

.. _create_vanadium_polaris_isis-powder-diffraction-ref:

create_vanadium
^^^^^^^^^^^^^^^^
The *create_vanadium* method allows a user to process a vanadium run.
Whilst processing the vanadium run the scripts can apply any corrections
the user enables and will spline the resulting workspace(s) for later focusing.

On POLARIS the following parameters are required when executing *create_vanadium*:

- :ref:`calibration_mapping_file_polaris_isis-powder-diffraction-ref`
- :ref:`do_absorb_corrections_polaris_isis-powder-diffraction-ref`
- :ref:`first_cycle_run_no_polaris_isis-powder-diffraction-ref`

The following may optionally be set.

- :ref:`mode_polaris_isis-powder-diffraction-ref`
- :ref:`multiple_scattering_polaris_isis-powder-diffraction-ref`
- :ref:`per_detector_vanadium_polaris_isis_powder-diffraction-ref`

Example
=======
..  code-block:: python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  polaris_example.create_vanadium(calibration_mapping_file=cal_mapping_file,
                                  mode="PDF", do_absorb_corrections=True,
                                  first_cycle_run_no=100, multiple_scattering=False)

.. _focus_polaris_isis-powder-diffraction-ref:

focus
^^^^^
The *focus* method processes the user specified run(s). It aligns,
focuses and optionally applies corrections if the user has requested them.

On POLARIS the following parameters are required when executing *focus*:

- :ref:`calibration_mapping_file_polaris_isis-powder-diffraction-ref`
- :ref:`do_absorb_corrections_polaris_isis-powder-diffraction-ref`
- :ref:`do_van_normalisation_polaris_isis-powder-diffraction-ref`
- :ref:`input_mode_polaris_isis-powder-diffraction-ref`
- :ref:`run_number_polaris_isis_powder-diffraction-ref`



The following parameters may also be optionally set:

- :ref:`mode_polaris_isis-powder-diffraction-ref`
- :ref:`multiple_scattering_polaris_isis-powder-diffraction-ref`
- :ref:`file_ext_polaris_isis-powder-diffraction-ref`
- :ref:`sample_empty_polaris_isis_powder-diffraction-ref`
- :ref:`suffix_polaris_isis-powder-diffraction-ref`
- :ref:`empty_can_subtraction_method_isis-powder-diffraction-ref`
- :ref:`paalman_pings_events_per_point_isis-powder-diffraction-ref`
- :ref:`per_detector_vanadium_polaris_isis_powder-diffraction-ref`
- :ref:`van_normalisation_method_isis-powder-diffraction-ref`


Example
=======

..  code-block:: python

  # Notice how the filename ends with .yaml
  cal_mapping_file = r"C:\path\to\cal_mapping.yaml"

  polaris_example.focus(calibration_mapping_file=cal_mapping_file,
                        mode="Rietveld", do_absorb_corrections=False,
                        file_ext=".s01", input_mode="Individual",
                        run_number="100-110")

.. _set_sample_polaris_isis-powder-diffraction-ref:

set_sample
^^^^^^^^^^^
The *set_sample* method allows a user to specify a SampleDetails
object which contains the sample properties used when
:ref:`do_absorb_corrections_polaris_isis-powder-diffraction-ref` is **True**
whilst focusing.

For more details on the SampleDetails object and how to set
it see: :ref:`isis-powder-diffraction-sampleDetails-ref`

The following parameter is required when calling *set_sample*

- *sample* - This must be a SampleDetails object with the
  material set already.

Example
=======

..  code-block:: python

  sample_obj = SampleDetails(...)
  sample_obj.set_material(...)

  polaris_example.set_sample(sample=sample_obj)

.. _create_total_scattering_pdf_polaris-isis-powder-ref:


create_total_scattering_pdf
^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. warning:: Total scattering support is not yet fully implemented.
             Any results obtaining from using the below routine in its current
             state should not be considered accurate or complete.

The *create_total_scattering_pdf* method allows a user to create a Pair Distribution Function (PDF)
from focused POLARIS data, with a view to performing further total scattering analysis.

This function takes as input a workspace group containing focussed data that has been normalized as a differential cross section. The input can be generated by running
:ref:`focus_polaris_isis-powder-diffraction-ref` with van_normalisation_method=Absolute.

The function performs the following processing:

- converts the x units of the workspace group containing the differential cross section to momentum transfer
- calculates the Placzek self scattering algorithm using the workflow algorithm :ref:`algm-TotScatCalculateSelfScattering`.
  This internally calculates a per detector Placzek correction using :ref:`algm-CalculatePlaczek` and then focusses the correction using :ref:`algm-GroupDetectors`
- subtracts the Placzek self scattering correction from the differential cross section
- converts the differential cross-section into an :math:`S(Q) - 1` distribution using the following equation. This is based on equations 5, 9 and 19 in [#Keen]_:

  .. math::

   S(Q) - 1 = (\frac{d\sigma_s}{d\Omega}(Q) - \langle\overline{b^2}\rangle) / \langle\overline{b}\rangle^2 = (\frac{d\sigma_s}{d\Omega}(Q) - \frac{\sigma_s}{4 \pi}) / \frac{\sigma_{coh}}{4 \pi}

  | where :math:`\sigma_s` is the total scattering cross section, :math:`\sigma_{coh}` is the coherent scattering cross section, :math:`b` is the scattering length.
  | The angled brackets indicate averaging across chemical formula units while the overline indicates averaging across isotopes and spin states for an individual element

- converts the :math:`S(Q) - 1` distribution into a PDF using :ref:`algm-PDFFourierTransform`

The input is specified using the run_number parameter. The input workspace group for this run number must
either be loaded in Mantid with the naming format given by the *focus* method:

*<run number>-Results-<TOF/D>*

for example:

12345-Results-TOF

Or a focused file with the same name format must be in the :ref:`output_directory_polaris_isis-powder-diffraction-ref` specified when creating the POLARIS object.

The output PDF can be customized with the following parameters:

- By calling with `pdf_type` the type of PDF output can be specified, with the option of `G(r)`,
  `g(r)`, `RDF(r)` (defaults to `G(r)`).
- By calling with `merge_banks=True` a PDF will be generated based on the weighted sum of the detector
  banks performed using supplied Q limits `q_lims=q_limits`, q_limits can be in the form of an array or
  with shape (2, x) where x is the number of banks, or a string containing the directory
  of an appropriately formatted `.lim` file. To exclude any of the banks use -1 as the value for that bank in each list.
  By default or specifically called with `merge_banks=False` a PDF will be generated for each bank within the focused_workspace.
- By calling with `delta_q` which will calculate the PDF after rebinning the Q workspace to have bin width `delta_r`.
- By calling with `delta_r` which will calculate the PDF with bin width of `delta_q`.
- By calling with `lorch_filter` will calculate the PDF with a Lorth Filter if set to `True`
- By calling with `freq_params` a fourier filter will be performed on the focused signal removing any
  components from atomic distances outside of the parameters. The parameters must be given as list:
  [lower], or [lower, upper]. The upper bound serves to remove noise from the spectrum density, by default
  when a fourier filter is performed this is set to 1000 to minimise loss of detail while still being computationally
  efficient.
- By calling with `debug=True` which will retain the intermediate self scattering correction workspace.
- By calling with `placzek_order` the Placzek correction order can be specified, with the option of 1 or 2 (defaults to 1).
- By calling with `sample_temp` the user can override the sample temperature provided in the logs. It defaults to using values from the logs if available.
- By calling with `pdf_output_name`, the name of the output PDF will be set to the user-provided name. If not specified, the output will be the run number suffixed with the PDF type.

Example
=======

..  code-block:: python

  # Include all the banks
  polaris_example.create_total_scattering_pdf(run_number='12345',
                                              merge_banks=True,
                                              q_lims=[[2.5, 3, 4, 6, 7], [3.5, 5, 7, 11, 40]],
                                              output_binning=[0,0.1,20],
                                              pdf_type='G(r)',
                                              freq_params=[1])

  # Exclude the 2nd and 4th banks
  polaris_example.create_total_scattering_pdf(run_number='12345',
                                              merge_banks=True,
                                              q_lims=[[2.5, -1, 4, -1, 7], [3.5, -1, 7, -1, 40]],
                                              output_binning=[0,0.1,20],
                                              pdf_type='G(r)',
                                              freq_params=[1])

.. _calibration_mapping_polaris-isis-powder-ref:


Calibration Mapping File
-------------------------
The calibration mapping file holds the mapping between
run numbers, current label, offset filename and the empty
and vanadium numbers.

For more details on the calibration mapping file see:
:ref:`cycle_mapping_files_isis-powder-diffraction-ref`

The layout on POLARIS should look as follows for each block
substituting the below values for appropriate values:

.. code-block:: yaml
  :linenos:

  1-100:
    label: "1_1"
    offset_file_name: "offset_file.cal"
    PDF:
      vanadium_run_numbers: "10"
      empty_run_numbers: "20"
    Rietveld:
      vanadium_run_numbers: "30"
      empty_run_numbers: "40"

Lines 5 and 6 in this example set the vanadium and empty run numbers for
chopper off mode. Lines 8 and 9 set the vanadium and empty for chopper
on mode.

Example
^^^^^^^^
.. code-block:: yaml

  1-100:
    label: "1_1"
    offset_file_name: "offset_file.cal"
    PDF:
      vanadium_run_numbers: "10"
      empty_run_numbers: "20"
    Rietveld:
      vanadium_run_numbers: "30"
      empty_run_numbers: "40"

  101-:
    label: "1_2"
    offset_file_name: "offset_file.cal"
    PDF:
      vanadium_run_numbers: "110"
      empty_run_numbers: "120"
    Rietveld:
      vanadium_run_numbers: "130"
      empty_run_numbers: "140"

Parameters
-----------
The following parameters for POLARIS are intended for regular use
when using the ISIS Powder scripts.

.. _calibration_directory_polaris_isis-powder-diffraction-ref:

calibration_directory
^^^^^^^^^^^^^^^^^^^^^
This parameter should be the full path to the calibration folder.
Within the folder the following should be present:

- Grouping .cal file (see: :ref:`grouping_file_name_polaris_isis-powder-diffraction-ref`)
- Masking file (see: :ref:`masking_file_name_polaris_isis-powder-diffraction-ref`)
- Folder(s) with the label name specified in mapping file (e.g. "1_1")
  - Inside each folder should be the offset file with name specified in mapping file

The script will also save out vanadium splines into the relevant
label folder which are subsequently loaded and used within the
:ref:`focus_polaris_isis-powder-diffraction-ref` method.

Example Input:

..  code-block:: python

  polaris_example = Polaris(calibration_directory=r"C:\path\to\calibration_dir", ...)

.. _calibration_mapping_file_polaris_isis-powder-diffraction-ref:

calibration_mapping_file
^^^^^^^^^^^^^^^^^^^^^^^^^
This parameter gives the full path to the YAML file containing the
calibration mapping. For more details on this file see:
:ref:`calibration_mapping_polaris-isis-powder-ref`

*Note: This should be the full path to the file including extension*

Example Input:

..  code-block:: python

  # Notice the filename always ends in .yaml
  polaris_example = Polaris(calibration_mapping_file=r"C:\path\to\file\calibration_mapping.yaml", ...)

.. _mode_polaris_isis-powder-diffraction-ref:

mode
^^^^^^^^^^
*optional*

The current chopper mode to use in the
:ref:`create_vanadium_polaris_isis-powder-diffraction-ref`
and :ref:`focus_polaris_isis-powder-diffraction-ref` method.
This determines which vanadium and empty run numbers
to use whilst processing.

Accepted values are: **PDF**, **PDF_NORM** or **Rietveld**

*Note: This parameter is not case sensitive*

If this value is not set, mantid will attempt to deduce it from
the frequency logs.

Example Input:

..  code-block:: python

  polaris_example.create_vanadium(mode="PDF", ...)
  # Or
  polaris_example.create_vanadium(mode="PDF_NORM", ...)
  # Or alternatively
  polaris_example.focus(mode="Rietveld", ...)

.. _config_file_polaris_isis-powder-diffraction-ref:

config_file
^^^^^^^^^^^
The full path to the YAML configuration file. This file is
described in detail here: :ref:`configuration_files_isis-powder-diffraction-ref`
It is recommended to set this parameter at object creation instead
of on a method as it will warn if any parameters are overridden
in the scripting window.

*Note: This should be the full path to the file including extension*

Example Input:

..  code-block:: python

  # Notice the filename always ends in .yaml
  polaris_example = Polaris(config_file=r"C:\path\to\file\configuration.yaml", ...)

.. _do_absorb_corrections_polaris_isis-powder-diffraction-ref:

do_absorb_corrections
^^^^^^^^^^^^^^^^^^^^^
Indicates whether to perform vanadium absorption corrections
in :ref:`create_vanadium_polaris_isis-powder-diffraction-ref` mode.
In :ref:`focus_polaris_isis-powder-diffraction-ref` mode
sample absorption corrections require the sample be
set first with the :ref:`set_sample_polaris_isis-powder-diffraction-ref`
method.

Accepted values are: **True** or **False**

Example Input:

..  code-block:: python

  polaris_example.create_vanadium(do_absorb_corrections=True, ...)

  # Or (this assumes sample details have already been set)
  polaris_example.focus(do_absorb_corrections=True, ...)

.. _do_van_normalisation_polaris_isis-powder-diffraction-ref:

do_van_normalisation
^^^^^^^^^^^^^^^^^^^^
Indicates whether to divide the focused workspace within
:ref:`focus_polaris_isis-powder-diffraction-ref` mode with a
previously generated vanadium spline.

This requires a vanadium to have been previously created
with the :ref:`create_vanadium_polaris_isis-powder-diffraction-ref`
method

Accepted values are: **True** or **False**

Example Input:

..  code-block:: python

  polaris_example.focus(do_van_normalisation=True, ...)

.. _van_normalisation_method_isis-powder-diffraction-ref:

van_normalisation_method
^^^^^^^^^^^^^^^^^^^^^^^^

Indicates whether a relative or absolute normalisation should
be performed. The possible values are "Relative" and "Absolute".

This parameter is optional. The default value when :ref:`mode_polaris_isis-powder-diffraction-ref` ="Rietveld" is "Relative". The default value when mode="PDF" is "Absolute".

If "Absolute" is selected then the measured intensity is multipled by the following additional factor to give a differential cross section (which is based on equation 8 in [#Howe]_):

.. math::

    \frac{\rho_v V_v \sigma_v}{4 \pi \rho_s V_s}

where :math:`\rho=` number density, :math:`V=` volume of material in the beam and the subscripts :math:`v` and :math:`s` indicate Vanadium and sample respectively.

Example Input:

..  code-block:: python

  polaris_example.focus(van_normalisation_method="Absolute", ...)

.. _empty_can_subtraction_method_isis-powder-diffraction-ref:

empty_can_subtraction_method
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Sets the empty can subtraction type used in the :ref:`focus_polaris_isis-powder-diffraction-ref` method. This parameter
is optional and can be set to either ``"Simple"`` (default) or ``"PaalmanPings"``. In ``"PaalmanPings"`` mode,
the absorption correction is applied by :ref:`PaalmanPingsMonteCarloAbsorption <algm-PaalmanPingsMonteCarloAbsorption>`,
followed by :ref:`ApplyPaalmanPingsCorrection <algm-ApplyPaalmanPingsCorrection>`. Additionally, in this mode
the :ref:`paalman_pings_events_per_point_isis-powder-diffraction-ref` parameter can be utilised.

Example Input:

..  code-block:: python

  polaris_example.focus(empty_can_subtraction_method="PaalmanPings", ...)

.. _file_ext_polaris_isis-powder-diffraction-ref:

file_ext
^^^^^^^^
*Optional*

Specifies a file extension to use when using the
:ref:`focus_polaris_isis-powder-diffraction-ref` method.

This should be used to process partial runs. When
processing full runs (i.e. completed runs) it should not
be specified as Mantid will automatically determine the
best extension to use.

*Note: A leading dot (.) is not required but
is preferred for readability*

Example Input:

..  code-block:: python

  polaris_example.focus(file_ext=".s01", ...)


.. _first_cycle_run_no_polaris_isis-powder-diffraction-ref:

first_cycle_run_no
^^^^^^^^^^^^^^^^^^^
Indicates a run from the current cycle to use when calling
:ref:`create_vanadium_polaris_isis-powder-diffraction-ref`.
This does not have the be the first run of the cycle or
the run number corresponding to the vanadium. However it
must be in the correct cycle according to the
:ref:`calibration_mapping_polaris-isis-powder-ref`.

Example Input:

..  code-block:: python

  # In this example assume we mean a cycle with run numbers 100-200
  polaris_example.create_vanadium(first_cycle_run_no=100, ...)


.. _input_mode_polaris_isis-powder-diffraction-ref:

input_mode
^^^^^^^^^^
Indicates how to interpret the parameter
:ref:`run_number_polaris_isis_powder-diffraction-ref` whilst
calling the :ref:`focus_polaris_isis-powder-diffraction-ref`
method.
If the input_mode is set to *Summed* it will process
to sum all runs specified. If set to *Individual* it
will process all runs individually (i.e. One at a time)

Accepted values are: **Summed** and **Individual**

*Note: This parameter is not case sensitive*

Example Input:

..  code-block:: python

  polaris_example.focus(input_mode="Summed", ...)


.. _multiple_scattering_polaris_isis-powder-diffraction-ref:

multiple_scattering
^^^^^^^^^^^^^^^^^^^
*optional*

Indicates whether to account for the effects of multiple scattering
when calculating absorption corrections.

Accepted values are: **True** or **False**

*Note: Calculating multiple scattering effects will add around
10-30 minutes to the script runtime depending on the speed of
the computer you are using*

Example Input:

..  code-block:: python

  polaris_example.create_vanadium(multiple_scattering=True, ...)
  # Or
  polaris_example.focus(multiple_scattering=False, ...)

.. _output_directory_polaris_isis-powder-diffraction-ref:

output_directory
^^^^^^^^^^^^^^^^
Specifies the path to the output directory to save resulting files
into. The script will automatically create a folder
with the label determined from the
:ref:`calibration_mapping_file_polaris_isis-powder-diffraction-ref`
and within that create another folder for the current
:ref:`user_name_polaris_isis-powder-diffraction-ref`.

Within this folder processed data will be saved out in
several formats.

Example Input:

..  code-block:: python

  polaris_example = Polaris(output_directory=r"C:\path\to\output_dir", ...)

.. _paalman_pings_events_per_point_isis-powder-diffraction-ref:

paalman_pings_events_per_point
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Sets the number of EventsPerPoint to use in the
:ref:`PaalmanPingsMonteCarloAbsorption <algm-PaalmanPingsMonteCarloAbsorption>`. The default value is 1000.
This parameter is only used when :ref:`empty_can_subtraction_method_isis-powder-diffraction-ref` is set to ``"PaalmanPings"``.

Example Input:

..  code-block:: python

  polaris_example.focus(paalman_pings_events_per_point=10, ...)

.. _per_detector_vanadium_polaris_isis_powder-diffraction-ref:

per_detector_vanadium
^^^^^^^^^^^^^^^^^^^^^

Determines whether the Vanadium normalisation is performed at the detector level or the bank level. The default value is False (bank level).
The parameter should be supplied to both the :ref:`create_vanadium_polaris_isis-powder-diffraction-ref` and :ref:`focus_polaris_isis-powder-diffraction-ref` methods (or supplied in the parameter list when creating the POLARIS object) if a reduction based on a per detector Vanadium normalisation is required.

..  code-block:: python

  from isis_powder import Polaris

  calibration_dir = r"C:\path\to\calibration_dir"
  output_dir = r"C:\path\to\output_dir"

  polaris_example = Polaris(calibration_directory=calibration_dir,
                            output_directory=output_dir,
                            user_name="Mantid",
                            per_detector_vanadium=True)

.. _run_number_polaris_isis_powder-diffraction-ref:

run_number
^^^^^^^^^^
Specifies the run number(s) to process when calling the
:ref:`focus_polaris_isis-powder-diffraction-ref` method.

This parameter accepts a single value or a range
of values with the following syntax:

**-** : Indicates a range of runs inclusive
(e.g. *1-10* would process 1, 2, 3....8, 9, 10)

**,** : Indicates a gap between runs
(e.g. *1, 3, 5, 7* would process run numbers 1, 3, 5, 7)

These can be combined like so:
*1-3, 5, 8-10* would process run numbers 1, 2, 3, 5, 8, 9, 10.

In addition the :ref:`input_mode_polaris_isis-powder-diffraction-ref`
parameter determines what effect a range of inputs has
on the data to be processed

Example Input:

..  code-block:: python

  # Process run number 1, 3, 5, 6, 7
  polaris_example.focus(run_number="1, 3, 5-7", ...)
  # Or just a single run
  polaris_example.focus(run_number=100, ...)

.. _sample_empty_polaris_isis_powder-diffraction-ref:

sample_empty
^^^^^^^^^^^^
*Optional*

This parameter specifies a/several sample empty run(s)
to subtract from the run in the
:ref:`focus_polaris_isis-powder-diffraction-ref` method.
If multiple runs are specified it will sum these runs
before subtracting the result.

This input uses the same syntax as
:ref:`run_number_polaris_isis_powder-diffraction-ref`.
Please visit the above page for more details.

*Note: If this parameter is set to* **True**
:ref:`sample_empty_scale_polaris_isis-powder-diffraction-ref`
*must also be set.* This is set to 1.0 by default.

Example Input:

..  code-block:: python

  # Our sample empty is a single number
  polaris_example.focus(sample_empty=100, ...)
  # Or a range of numbers
  polaris_example.focus(sample_empty="100-110", ...)

.. _suffix_polaris_isis-powder-diffraction-ref:

suffix
^^^^^^
*Optional*

This parameter specifies a suffix to append the names of output files
during a focus.

Example Input:

.. code-block:: python

  polaris_example.focus(suffix="-corr", ...)

.. _user_name_polaris_isis-powder-diffraction-ref:

user_name
^^^^^^^^^
Specifies the name of the current user when creating a
new POLARIS object. This is only used when saving data to
sort data into respective user folders.
See :ref:`output_directory_polaris_isis-powder-diffraction-ref`
for more details.

Example Input:

..  code-block:: python

  polaris_example = Polaris(user_name="Mantid", ...)


Advanced Parameters
--------------------
.. warning:: These values are not intended to be changed and should
             reflect optimal defaults for the instrument. For more
             details please read:
             :ref:`instrument_advanced_properties_isis-powder-diffraction-ref`

             This section is mainly intended to act as reference of the
             current settings distributed with Mantid

All values changed in the advanced configuration file
requires the user to restart Mantid for the new values to take effect.
Please read :ref:`instrument_advanced_properties_isis-powder-diffraction-ref`
before proceeding to change values within the advanced configuration file.

.. _focused_cropping_values_polaris_isis-powder-diffraction-ref:

focused_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^^
Indicates a list of TOF values to crop the focused workspace
which was created by :ref:`focus_polaris_isis-powder-diffraction-ref`
on a bank by bank basis.

This parameter is a list of bank cropping values with
one list entry per bank. The values **must** have a smaller
TOF window than the :ref:`vanadium_cropping_values_polaris_isis-powder-diffraction-ref`

On POLARIS this is set to the following TOF windows:

..  code-block:: python

  focused_cropping_values = [
      (700,  30000),  # Bank 1
      (1200, 24900),  # Bank 2
      (1100, 19950),  # Bank 3
      (1100, 19950),  # Bank 4
      (1100, 19950),  # Bank 5
      ]

.. _grouping_file_name_polaris_isis-powder-diffraction-ref:

grouping_file_name
^^^^^^^^^^^^^^^^^^
Determines the name of the grouping cal file which is located
within top level of the :ref:`calibration_directory_polaris_isis-powder-diffraction-ref`.

The grouping file determines the detector ID to bank mapping to use
whilst focusing the spectra into banks.

On POLARIS this is set to the following:

..  code-block:: python

  grouping_file_name: "Master_copy_of_grouping_file_with_essential_masks.cal"

.. _masking_file_name_polaris_isis-powder-diffraction-ref:

vanadium_peaks_masking_file
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Determines the name of the masking file containing the
masks to remove Bragg peaks on Polaris. This file must
be located within the top level of the
:ref:`calibration_directory_polaris_isis-powder-diffraction-ref`.

On POLARIS this is set to the following:

..  code-block:: python

  vanadium_peaks_masking_file: "VanaPeaks.dat"

.. _nxs_filename_polaris_isis-powder-diffraction-ref:

nxs_filename
^^^^^^^^^^^^
A template for the filename of the generated NeXus file.

.. _gss_filename_polaris_isis-powder-diffraction-ref:

gss_filename
^^^^^^^^^^^^
A template for the filename of the generated GSAS file.

.. _dat_files_directory_polaris_isis-powder-diffraction-ref:

dat_files_directory
^^^^^^^^^^^^^^^^^^^
The subdirectory of the output directory where the .dat files are saved

.. _tof_xye_filename_polaris_isis-powder-diffraction-ref:

tof_xye_filename
^^^^^^^^^^^^^^^^
A template for the filename of the generated TOF XYE file.

.. _dspacing_xye_filename_polaris_isis-powder-diffraction-ref:

dspacing_xye_filename
^^^^^^^^^^^^^^^^^^^^^
A template for the filename of the generated dSpacing XYE file.

.. _sample_empty_scale_polaris_isis-powder-diffraction-ref:

sample_empty_scale
^^^^^^^^^^^^^^^^^^
Required if :ref:`sample_empty_polaris_isis_powder-diffraction-ref`
is set to **True**

Sets a factor to scale the sample empty run(s) to before
subtracting. This value is multiplied after summing the
sample empty runs and before subtracting the empty from
the data set. For more details see: :ref:`Scale <algm-Scale-v1>`.

Example Input:

..  code-block:: python

  # Scale sample empty to 90% of original
  polaris_example.focus(sample_empty_scale=0.9, ...)

.. _raw_data_cropping_values_polaris_isis-powder-diffraction-ref:

raw_data_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^^^
Determines the TOF window to crop all spectra down to before any
processing in the :ref:`create_vanadium_polaris_isis-powder-diffraction-ref`
and :ref:`focus_polaris_isis-powder-diffraction-ref` methods.

This helps remove negative counts where at very low TOF
the empty counts can exceed the captured neutron counts
of the run to process.

On POLARIS this is set to the following:

..  code-block:: python

  raw_data_cropping_values: (750, 20000)

.. _spline_coefficient_polaris_isis_powder-diffraction-ref:

spline_coefficient
^^^^^^^^^^^^^^^^^^
Determines the spline coefficient to use when processing
the vanadium in :ref:`create_vanadium_polaris_isis-powder-diffraction-ref`
method. For more details see :ref:`SplineBackground <algm-SplineBackground>`

*Note that if this value is changed 'create_vanadium'
will need to be called again.*

On POLARIS this is set to the following:

..  code-block:: python

  spline_coefficient: 100

.. _spline_coeff_per_detector_polaris_isis_powder-diffraction-ref:

spline_coeff_per_detector
^^^^^^^^^^^^^^^^^^^^^^^^^
Determines the spline coefficient to use when processing
the vanadium in :ref:`create_vanadium_polaris_isis-powder-diffraction-ref`
method. For more details see :ref:`SplineBackground <algm-SplineBackground>`.
This parameter is used if a detector level Vanadium normalisation has been requested
and is typically lower than :ref:`spline_coefficient_polaris_isis_powder-diffraction-ref`

*Note that if this value is changed 'create_vanadium'
will need to be called again.*

On POLARIS this is set to the following:

..  code-block:: python

  spline_coeff_per_detector: 10


.. _vanadium_cropping_values_polaris_isis-powder-diffraction-ref:

vanadium_cropping_values
^^^^^^^^^^^^^^^^^^^^^^^^
Determines the TOF windows to crop to on a bank by bank basis
within the :ref:`create_vanadium_polaris_isis-powder-diffraction-ref`
method. This is applied after focusing and before a spline is taken.

It is used to remove low counts at the start and end of the vanadium run
to produce a spline which better matches the data.

This parameter is a list of bank cropping values with
one list entry per bank. The values **must** have a larger
TOF window than the :ref:`focused_cropping_values_polaris_isis-powder-diffraction-ref`
and a smaller window than :ref:`raw_data_cropping_values_polaris_isis-powder-diffraction-ref`.

On POLARIS this is set to the following:

..  code-block:: python

  vanadium_cropping_values = [(800, 19995),  # Bank 1
                              (800, 19995),  # Bank 2
                              (800, 19995),  # Bank 3
                              (800, 19995),  # Bank 4
                              (800, 19995),  # Bank 5
                             ]

.. _vanadium_sample_details_polaris_isis-powder-diffraction-ref:

Vanadium sample details
^^^^^^^^^^^^^^^^^^^^^^^

chemical_formula
================

The chemical formula for the Vanadium rod.


On POLARIS this is set to the following:

.. code-block:: python

  chemical_formula = "V"

cylinder_sample_height
======================

The height of the Vanadium rod.

On POLARIS this is set to the following:

.. code-block:: python

  cylinder_sample_height = 4.0

cylinder_sample_radius
======================

The radius of the Vanadium rod.

On POLARIS this is set to the following:

.. code-block:: python

  cylinder_sample_radius = 0.25

cylinder_position
=================

The position of the Vanadium rod in [x, y, z]

On POLARIS this is set to the following:

.. code-block:: python

  cylinder_position = [0.0, 0.0, 0.0]

References
----------

.. [#Howe] M A Howe, R L McGreevy and W S Howells
   *The analysis of liquid structure data from time-of-flight neutron diffractometry*
   J. Phys.: Condens. Matter 1 (1989)

.. [#Keen] D A Keen
   *A comparison of various commonly used correlation functions for describing total scattering*
   Journal Applied Crystallography (2000)


.. categories:: Techniques
