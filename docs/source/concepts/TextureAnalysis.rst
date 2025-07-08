.. _TextureAnalysis:

Texture Analysis Theory
=======================

.. contents::

Introduction to Texture Analysis
################################

For argument sake, lets say you have some sample and you are interested to know what the crystallographic texture is -- that is to say, you want to know what
the relationship is between the macroscopic dimensions of your sample and some given crystallographic plane e.g. :math:`(100)`?

.. figure:: /images/texture-example-sample.png
   :alt: An example cuboid sample and a corresponding mantid representation

Taking this sample as an example, you can see that, by merit of being a cuboid, the sample has a unique height, width and length.
These directions may or may not also be correlated with some processing procedure (e.g. in this case: the length is the Rolling Direction;
The width is the Traverse Direction and the height is the Normal Direction).

Some questions you might have about your sample are:

- How is the underlying crystal structure orientated in relation to these macroscopic directions?
- Does this relationship change when looking at different points within the sample?
- Is this relationship a product of the processing?


These are the questions which the texture analysis pipeline in Mantid seeks to help you answer.

Within the software are able to produce Pole Figures for different Bragg reflections.
The pole figure plot will typically show the intensity of the peak associated with that reflection along different macroscopic sample directions.

Additionally, you are able to produce similar plots but looking at other features of the bragg peak, such as peak position for strain mapping.


Interpreting Pole Figures
#########################

The way to interpret the pole figure is to imagine that your sample is within a sphere.
Each unique direction relative to your sample will intercept the sphere at a unique point -- these points are the poles (like the North and South Poles of the Earth).

If we imagine being able to sample the intensity of your given reflection peak in every possible direction, this would correspond to sampling the surface of the sphere.
Plotting the intensities on this sphere, we would get a complete 3D representation of how the intensity changes along all macroscopic sample directions.
The second graphic shows the plot with the intensity values convolved into the radial coordinate, which gives another spatial representation of the intensity of the bragg reflection
as a function of direction around the sample.

.. figure:: /images/texture-direction-sphere.gif
   :alt: GIF showing how the set of direction vectors trace out the surface of a sphere

.. figure:: /images/texture-direction-peaks.gif
   :alt: GIF showing how the set of direction vectors trace out the surface of a sphere, with intensity convolved into position

Much like how maps of the world provide 2D representations of the 3D globe, we can do the same thing by projecting the 3D pole figures down into a 2D pole figure.
The below graphic shows the relationship between the 2D pole figure and the 3D sphere which defines all unique directions around a sample.

.. figure:: /images/texture-pole-figure-interpretation.gif
   :alt: GIF showing the relationship between the 3D and 2D representations of the pole figure

The surface of this sphere is coloured by the intensity of a selected bragg peak, giving a 3D pole figure.
Additionally, the graphic is shows the distortion between this spherical representation and the intensity convolved representation.

Depending upon the exact transformation, the 2D pole figure can be chosen to maintain/highlight a desired geometric relation in the 3D surface
(e.g. the azimuthal and stereographic projections provided maintain the angular relationship, which can be useful for viewing the symmetry of poles).
In reality, we cannot sample every possible point on this sphere, we are experimentally confined by our detector geometries and finite time, to only sample a subset of these points.
These are the points displayed in the experimental pole figure scatter plot.
(It is possible to interpolate between these points to get a more continuous representation -- which is given as an option to display the contour plot instead.)

.. figure:: /images/texture-pole-figure-displays.png
   :alt: Image comparing the scatter plot pole figure and the contour interpolation


Generating Pole Figures
#######################

The below figure shows how the orientation of the detectors, relative to the sample, relates to the 3D and 2D pole figures.
The top two graphics show the individual scattering vectors for two of the detectors, depicted as gold and pink arrows,
and how the intrinsic directions of the sample move relative to these scattering vectors as the orientation of that sample changes during the experiment.
The bottom left graphic then shows, in the fixed, intrinsic sample frame of the pole figures, the corresponding relative movement of these scattering vectors.
Here the sphere is coloured with the intensity of the complete pole figure.
The bottom right graphic shows how the scattering vectors (corresponding to all the 30 detectors) are then projected into the 2D pole figure, again, the pink and gold detectors are highlighted here.

.. figure:: /images/texture-pole-figure-lookup.gif
   :alt: GIF showing the relationship between the experimental geometry and the pole figure


This final graphic, below, shows how the intensities are determined for the points in the experimental pole figure.
Here the two detector banks have been split up into 3x5 grids. The summed spectra for each block in the grid is collected over the course of the experiment and these are shown on the left and right plots.
The pole figure for a given reflection is then generated by fitting a peak to the desired reflection and reading out the peak parameter of interest which, in the case shown, is the integrated intensity.
The bottom images show these integrated intensity values on the actual detector banks and how these are projected into the 2D pole figure.

.. figure:: /images/texture-pole-figure-detectors.gif
   :alt: GIF showing how intensities are calculated for each detector in the pole figure

Pole Figure Resolution and Coverage
###################################

A few factors will affect the final quality of the pole figure data, with the two main considerations being how the detector banks are grouped and
for what sample orientations data is collected.

In mantid, the first of these -- the detector groupings, can be decided in post, after the experiment has been run.
The reality here (at least for ENGIN-X), is that despite it being possible to generate an experimental pole figure using each individual detector pixel as a unique point,
the confidence in the metric which has been extracted from peak fit will be a product of the signal-to-noise-ratio of those individual signals. This can be improved by
grouping neighbouring pixels together, thus obtaining cleaner spectra to fit, at the trade off of spatial resolution. Alternatively, beam access permitting, longer collection times
can be used, allowing finer pixel groupings (or none at all) to be achievable and improve the spatial resolution.

The second factor -- sample orientations, is something which perhaps requires more consideration before hitting go on data collection. The factors to weigh up here are
optimising your balance of time vs uncertainty. If you are quite confident in some aspect of your texture (such as a known symmetry), you may be able to target data
collection to obtain datasets with the detectors covering only a few key sectors in the pole figure, saving time on fewer experimental runs. In contrast, if the texture
is unknown, the optimal strategy is likely to be obtaining even coverage across the entire figure, and aiming to do this in a time efficient manner. The other trade off
of this exploratory coverage, compared to a more targeted approach is that one will likely end up with fewer data points around the actual regions of interest. A discussion
of possible exploratory coverage schemes is given by Malamud [#detBanks]_.

As such, again time permitting, a dual approach may prove advantageous for unknown textures, where a preliminary full coverage dataset is collect and, upon subsequent
inspection, addition runs are collected targeting the identified regions of interest.

Texture Analysis within Mantid
==============================

The creation of pole figures within mantid can be achieved in two distinct workflows: either using scripts within the python interface or
through the Engineering Diffraction user interface. The application of the latter will be discussed separately in :ref:`_Engineering_Diffraction-ref`,
here we will focus on the scripting approach. It is worth noting that for practical application, the scripts offer the most time efficient workflow and, as such,
are probably the preferable approach for creating pole figures post-experiment, with the user interface offering a more interactive approach which lends itself to
processing and guiding the evolution of the experiment, as it happens.

Introduction to Sample Shape, Material, and Orientation
#######################################################

A critical aspect to performing texture analysis is having the correct representation of the sample, its shape, and its intrinsic directions for each dataset you process.
This is crucial because these are the factors which will determine where points end up within the pole figure. Getting these things right within mantid, should hopefully, not be
too onerous, but care should be taken to make good records of the physical layouts during the experiment to check your recreation in mantid.

The way the texture analysis has been designed in Mantid, is that each run's workspace should contain the information about the sample shape and its orientation relative
to an initial reference position. It is then required, at the point of pole figure creation, to provide the intrinsic sample directions, in lab coordinates, for this
initial reference position. Typically this is achieved by having the initial reference position as the sample mounted upon the goniometer of choice in its default "home" position.
The sample would ideally be aligned on the homed goniometer to have intrinsic directions aligned with identifiable directions in the lab coordinates, which is often
intuatively done in practice (intrinsic directions are typically aligned with some topological features and these are oft aligned to be parallel or perpendicular to the beam).
If the sample is not so straightforwardly positioned in the reference state, some more care should be taken to get the definition of these initial directions correct.

From here, the transformation to each run's sample orientation is exactly the same as the transformation defined by the goniometer state for that run. On ENGINX, there are
two main goniometers used - the Eulerian Cradle and the Cybaman. Extracting the state transformations for these two goniometers setups require different approaches, but
should provide coverage for a broad range of additional setups.

The general procedure for transfering these pieces of information onto the relevant workspaces is as follows. First define a "Reference Workspace" upon which the initial
sample shape and orientation can be saved (along with any information on material which might be used for absorption correction). Next, load in all the run workspaces
corresponding to this experiment. Load an orientation file to set the goniometer transformation on the individual workspaces. Copy the sample definition across from the
reference workspace to each of the run workspaces. This is applied as part of the absorption script provided below. We also provide some additional notes and scripts
to aid in the setup of reference workspaces and orientation files

Reference Workspace
###################

The following script will allow the setup of the reference workspace, alternatively this functionality is available interactively within the Absorption Correction Tab
of the user interface.
.. code::python

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np
   from Engineering.texture.correction.correction_model import TextureCorrectionModel

   # Create an example Reference Workspace

   exp_name = "Example"
   root_dir = fr"C:\Users\Name\Engineering_Mantid\User\{exp_name}"
   instr = "ENGINX"


   model = TextureCorrectionModel()
   LoadEmptyInstrument(InstrumentName=instr, OutputWorkspace="")

   model.create_reference_ws(exp_name)

   # either set or load sample shape
   #set:
   shape_xml = ""
   SetSampleShape(model.reference_ws, shape_xml)

   #load:
   shape_file = ""
   LoadSampleShape(model.reference_ws, shape_file)

   # Now set the sample material
   # set material
   SetSampleMaterial(model.reference_ws, "Fe")

   # save reference file
   model.save_reference_file(exp_name, None, root_dir) # just set group as None here

Orientation Files
#################

As discussed previously, the orientation information is expected to come from either the Eulerian Cradle or the Cybaman, but these two goniometers are handled broadly
by providing either a series of fixed rotations around known axes (cradle) or by providing a flattened transformation matrix corresponding to a more complicated
transformation (cybaman). The flag which controls this behaviour is `orient_file_is_euler`.

If this is `True`, the orientation file is expect to be a text file with a row for each run and, within each row, a rotation angle for each axis.
These axes are then defined by `euler_scheme`, taking a string of lab directions for the initial
axes of each goniometer axis. The sense of the rotation around these axes are then defined by `euler_axes_sense`, where the string given should be comma separated +/-1,
one for each axis, where rotations are counter-clockwise (1) or clockwise (-1).

If `orient_file_is_euler` is `False`, the orientation file is expected to be a text file with a row for each run and, within each row the first 9 values are expected to
be a flattened 3x3 transformation matrix. It is anticipated that this matrix would be extracted from the SscansS2 software, and a script is provided below for converted
the transformation matrices from SscansS2 reference frame into mantid. In principle, a flattened matrix from any sample positioner could be given here instead.

.. code:: python

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np

   txt_file = r"path\to\sscanss_output_matrices.txt"
   NUM_POINTS = 3 # sscanss allows matrices to be calculated at multiple points for the same desired orientation
   # for mantid, we want these as separate experiments so we separate them out into different orientation files

   with open(txt_file, "r") as f:
      goniometer_strings = [line.replace("\t", ",") for line in f.readlines()]

   transformed_strings = []


   for gs in goniometer_strings:
      or_vals = gs.split(",")
      trans_vals = or_vals[9:]
      run_mat = np.asarray(or_vals[:9], dtype=float).reshape((3, 3)).T
      mantid_mat = run_mat[[1, 2, 0], :][:, [1, 2, 0]]
      new_string = ",".join([str(x) for x in mantid_mat.reshape(-1)]+trans_vals)
      transformed_strings.append(new_string)

   num_scans = len(goniometer_strings)//NUM_POINTS

   # saves the output in the same location as the initial file, just with _mantid_point_{point index} on the end of each file name

   for scan_ind in range(NUM_POINTS):
      save_file = txt_file.replace(".txt", f"_mantid_point_{scan_ind}.txt")

      with open(save_file, "w") as f:
         f.writelines(transformed_strings[scan_ind*num_scans:(scan_ind+1)*num_scans])


Absorption Correction
#####################

A consideration when performing texture analysis is to decide how to deal with attenuation and absorption. Depending upon the material being used,
the accuracy required, and the amount of time available, you may or may not want to apply a correction to the raw data to correct for neutron attenuation.
Mantid offers a suite of approaches to tackle this (:ref:`_Sample Corrections`), so to a certain extent this can be tailored to the use case, but here we
will discuss the methodology designed to replicate the functionality available within the user interface, making use of :ref:`algm-MonteCarloAbsorption`.

Below is a script that can be used to this end. The script is split into three sections - imports, experiment information, and execution. For most use cases
the only section needing attention is the experimental information. This section should be sufficiently annotated to explain how to use it, but should mirror
the user interface while providing more repeatable processing.

.. code:: python

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np
   from mantid.api import AnalysisDataService as ADS
   from os import path, makedirs, scandir
   from Engineering.texture.TextureUtils import find_all_files, run_abs_corr

   ############### ENGINEERING DIFFRACTION INTERFACE ABSORPTION CORRECTION ANALOGUE #######################

   ######################### EXPERIMENTAL INFORMATION ########################################

   # First, you need to specify your file directories, If you are happy to use the same root, from experiment
   # to experiment, you can just change this experiment name.
   exp_name = "ExampleExperiment"

   # otherwise set root directory here:
   root_dir = fr"C:\Users\Name\Engineering_Mantid\User\{exp_name}"

   # next, specify the folder with the files you would like to apply the absorption correction to
   corr_dir = fr"C:\Users\Name\Documents\Example\DataFiles"

   # For texture, it is expected that you have a single sample shape, that is reorientated between runs.
   # this is handled by having a reference workspace with the shape in its neutral position
   # (position in the beamline when the goniometer is home)
   # This reference workspace probably requires you to do some interacting and validating, so should be setup in the UI
   # (Interfaces/Diffraction/Engineering Diffraction/Absorption Correction)

   # if this is the case copy ref should be True and the ref_ws_path should be given
   # otherwise, if set ref is true, it is assumed that the sample shapes are already present on the workspaces
   copy_ref = True
   ref_ws_path = path.join(root_dir, "ReferenceWorkspaces", f"{exp_name}_reference_workspace.nxs")

   # if using the reference you now need to reorientate the sample, this can be done using orientation files
   # two standard types

   # Euler Orientation (orient_file_is_euler = True)
   # for this, euler_scheme and euler_axes_sense must be given to say which lab frame directions the goniometer axes are pointing along
   # and where the rotations are counter-clockwise (1) or clockwise (-1)

   # Matrix Orientation (orient_file_is_euler = False)
   # for this the first 9 values in each row of the files are assumed to be flattened rotation matrix.
   # These are used to directly reorientate the samples
   orientation_file = r"C:\Users\Name\Documents\Example\DataFiles\pose_matrices_mantid.txt"
   orient_file_is_euler = False
   euler_scheme = "YXY"
   euler_axes_sense = "1,-1,1"

   # Now you can specify information about the correction
   include_abs_corr = True # whether to perform the correction based on absorption
   monte_carlo_args = "SparseInstrument:True" # what arguments to pass to MonteCarloAbsorption alg
   clear_ads_after = True # whether to remove the produced files from the ADS to free up RAM
   gauge_vol_preset = "4mmCube" # or "Custom" # the gauge volume being used
   gauge_vol_shape_file = None # or "path/to/xml" # a custom gauge volume shape file

   # There is also the option to output an attenuation table alongside correcting the data
   # This will return a table of the attenuation coefficient at the point specified
   include_atten_table = False
   eval_point = "2.00"
   eval_units = "dSpacing" #must be a valid argument for ConvertUnits

   # Finally, you can add a divergence correction to the data, this is still a work in progress, so keep False for now
   include_div_corr = False
   div_hoz = 0.02
   div_vert = 0.02
   det_hoz = 0.02

   ######################### RUN SCRIPT ########################################

   # load the ref workspace
   ref_ws_str = path.splitext(path.basename(ref_ws_path))[0]
   Load(Filename = ref_ws_path, OutputWorkspace = ref_ws_str)

   # load data workspaces
   corr_wss = find_all_files(corr_dir)
   wss = [path.splitext(path.basename(fp))[0] for fp in corr_wss]
   for iws, ws in enumerate(wss):
      if not ADS.doesExist(ws):
         Load(Filename = corr_wss[iws], OutputWorkspace= ws)

   # run script
   run_abs_corr(wss = wss,
               ref_ws = ref_ws_str,
               orientation_file = orientation_file,
               orient_file_is_euler = orient_file_is_euler,
               euler_scheme = euler_scheme,
               euler_axes_sense = euler_axes_sense,
               copy_ref = copy_ref,
               include_abs_corr = include_abs_corr,
               monte_carlo_args = monte_carlo_args,
               gauge_vol_preset = gauge_vol_preset,
               gauge_vol_shape_file = gauge_vol_shape_file,
               include_atten_table = include_atten_table,
               eval_point = eval_point,
               eval_units = eval_units,
               exp_name = exp_name,
               root_dir = root_dir,
               include_div_corr = include_div_corr,
               div_hoz = div_hoz,
               div_vert = div_vert,
               det_hoz = det_hoz,
               clear_ads_after = clear_ads_after)


Focusing
########

Regardless of whether absorption correction has been applied (at the very least the absorption correction script should probably be run with `include_abs_corr = False`,
in order to apply the sample shape and orientations), some focusing of data is likely required for creating pole figures. In principle, unfocussed data could be used,
but this would be rather slow due to the fitting of peaks on each spectra, and this would not necessarily provide meaningful improvement in spatial resolution. As far as
ENGIX is concerned, grouping any more finely than the block level is mostly diminishing returns. The below script can be used to generate some custom groupings at
the module or block level, and could be modified for more exotic groupings beyond this, but there are standard groupings available as well.

.. code::python

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np

   def get_detector_grouping_string(ws, group_by):
      info = ws.componentInfo()
      detinfo = ws.detectorInfo()
      dets = detinfo.detectorIDs()
      instr_dets = info.detectorsInSubtree(info.root())

      def get_det_id(comp_ind, dets, instr_dets):
         return dets[np.where(instr_dets == comp_ind)][0]

      nbi = info.indexOfAny("NorthBank")
      sbi = info.indexOfAny("SouthBank")


      nbmi = info.children(nbi)
      sbmi = info.children(sbi)

      nbmbi = [xx for x in [info.children(int(nbm)) for nbm in nbmi] for xx in x]
      sbmbi = [xx for x in [info.children(int(sbm)) for sbm in sbmi] for xx in x]
      if group_by == "module":
         n_mods = ",".join(
               ["+".join([str(get_det_id(x, dets, instr_dets)) for x in info.detectorsInSubtree(int(nbm))]) for nbm in
               nbmi])
         s_mods = ",".join(
               ["+".join([str(get_det_id(x, dets, instr_dets)) for x in info.detectorsInSubtree(int(sbm))]) for sbm in
               sbmi])
         return ",".join([n_mods, s_mods])
      if group_by == "block":
         n_blocks = ",".join(
               ["+".join([str(get_det_id(x, dets, instr_dets)) for x in info.detectorsInSubtree(int(nbm))]) for nbm in
               nbmbi])
         s_blocks = ",".join(
               ["+".join([str(get_det_id(x, dets, instr_dets)) for x in info.detectorsInSubtree(int(sbm))]) for sbm in
               sbmbi])
         return ",".join([n_blocks, s_blocks])

   ws = LoadEmptyInstrument(InstrumentName = "ENGINX")

   block_string = get_detector_grouping_string(ws, "block")

   det_group = CreateGroupingWorkspace(InputWorkspace = ws, CustomGroupingString = block_string, OutputWorkspace = "det_group")

   CreateGroupingWorkspace(InstrumentName='ENGINX',
                           ComponentName='ENGIN-X',
                           CustomGroupingString=block_string,
                           OutputWorkspace = "det_group")

   SaveCalFile(r"path\to\cal\block.cal", GroupingWorkspace = "det_group")

These cal files can be provided as a `grouping_filepath` if desired, or used to calibrate in the user interface and the resultant `prm` file can be used for focusing.

If using a standard grouping, no grouping_filepath or prm_filepath is required, and simply the string (e.g. `"Texture30"`) is needed.


References
----------

.. [#detBanks] J. Appl. Cryst. (2014). 47, 1337–1354 doi:10.1107/S1600576714012710

.. categories:: Concepts
