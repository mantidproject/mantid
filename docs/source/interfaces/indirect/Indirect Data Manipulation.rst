.. _interface-indirect-data-manipulation:

Indirect Data Manipulation
==========================

.. contents:: Table of Contents
  :local:

Overview
--------

The Indirect Data Manipulation interface provides Processes for manipulating
reduced data from :ref:`Indirect Data Reduction <interface-indirect-data-reduction>`
and converting reduced instrument data to S(Q, w), or moments for analysis in the
:ref:`Indirect Data Analysis <interface-inelastic-data-analysis>` and
:ref:`Indirect Bayes <interface-indirect-bayes>` interfaces.

.. interface:: Data Manipulation
  :align: right
  :width: 350

Action Buttons
~~~~~~~~~~~~~~

Settings
  Opens the :ref:`Settings <interface-indirect-settings>` GUI which allows you to
  customize the settings for the Indirect interfaces.

?
  Opens this help page.

Py
  Exports a Python script which will replicate the processing done by the current tab.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

Symmetrise
----------

This tab allows you to take an asymmetric reduced file (*_red.nxs*) and symmetrise it about
the Y axis.

The curve is symmetrised such that the range of positive values between :math:`EMin`
and :math:`EMax` are reflected about the Y axis and replaces the negative values
in the range :math:`-EMax` to :math:`-EMin`, the curve between :math:`-EMin` and
:math:`EMin` is not modified.

.. interface:: Data Reduction
  :widget: tabSymmetrise

Symmetrise Options
~~~~~~~~~~~~~~~~~~

Input
  Allows you to select a reduced NeXus file (*_red.nxs*) or workspace (*_red*) as the
  input to the algorithm.

EMin & EMax
  Sets the energy range that is to be reflected about :math:`y=0`.

Spectrum No
  Changes the spectrum shown in the preview plots.

XRange
  Changes the range of the preview plot, this can be useful for inspecting the
  curve before running the algorithm.

Preview
  This button will update the preview plot and parameters under the Preview
  section.

Run
  Runs the processing configured on the current tab.

Plot Spectra
  If enabled, it will plot the selected workspace indices in the selected output workspace.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

.. _preview-properties:

Preview
~~~~~~~

The preview section shows what a given spectra in the input will look like after
it has been symmetrised and gives an idea of how well the value of EMin fits the
curve on both sides of the peak.

Negative Y
  The value of :math:`y` at :math:`x=-EMin`.

Positive Y
  The value of :math:`y` at :math:`x=EMin`.

Delta Y
  The difference between Negative and Positive Y. Typically this should be as
  close to zero as possible.

.. _symmetrise-example-workflow:

Symmetrise Example Workflow
~~~~~~~~~~~~~~~~~~~~~~~~~~~
The Symmetrise tab operates on ``_red`` files. The file used in this workflow can
be produced using the 26176 run number on the ISIS Energy Transfer tab. The instrument used to
produce this file is IRIS, the analyser is graphite and the reflection is 002. See the
:ref:`isis-energy-transfer-example-workflow`.

1. In the **Input** box, load the file named ``iris26176_graphite002_red``. This will
   automatically plot the data on the first mini-plot.

2. Move the green slider located at x = -0.5 to be at x = -0.4.

3. Click **Preview**. This will update the :ref:`Preview properties <preview-properties>` and
   the neighbouring mini-plot.

4. Click **Run** and wait for the interface to finish processing. This will run the
   :ref:`Symmetrise <algm-Symmetrise>` algorithm. The output workspace is called
   ``iris26176_graphite002_sym_red``.

5. Click **Plot Spectra** to produce a spectra plot of the output workspace. Other indices can be
   plotted by entering indices in the box next to the **Plot Spectra** button. For example,
   entering indices 0-2,4,6-7 will plot the spectra with workspace indices 0, 1, 2, 4, 6 and 7.

Go to the :ref:`sqw-example-workflow`.

S(Q, w)
-------

Provides an interface for running the :ref:`SofQW <algm-SofQW>` algorithm.

.. interface:: Data Reduction
  :widget: tabSQw

Instrument Options
~~~~~~~~~~~~~~~~~~

Instrument
  Used to select the instrument on which the data being reduced was created on.

Analyser
  The analyser bank that was active when the experiment was run, or for which
  you are interested in seeing the results of.

Reflection
  The reflection plane of the instrument set up.

.. tip:: If you need clarification as to the instrument setup you should use
  please speak to the instrument scientist who dealt with your experiment.

S(Q, w) Options
~~~~~~~~~~~~~~~

Input
  Allows you to select a reduced NeXus file (*_red.nxs*) or workspace (*_red*) as the
  input to the algorithm. An automatic contour plot of *_rqw* will be plotted in the preview
  plot once a file has finished loading.

Q Low, Q Width & Q High
  Q binning parameters that are passed to the :ref:`SofQW <algm-SofQW>` algorithm. The low and high
  values can be determined using the neighbouring contour plot.

Rebin in Energy
  If enabled the data will first be rebinned in energy before being passed to
  the :ref:`SofQW <algm-SofQW>` algorithm.

E Low, E Width & E High
  The energy rebinning parameters. The low and high values can be determined using the neighbouring contour plot.

Run
  Runs the processing configured on the current tab.

Plot Spectra
  If enabled, it will plot the selected workspace indices in the selected output workspace.

Plot Contour
  If enabled, it will plot the selected output workspace as a contour plot.

Save Result
  If enabled the result will be saved as a NeXus file in the default save directory.

.. _sqw-example-workflow:

S(Q, w) Example Workflow
~~~~~~~~~~~~~~~~~~~~~~~~
The S(Q, w) tab operates on ``_red`` files. The file used in this workflow can be produced
using the 26176 run number on the ISIS Energy Transfer tab. The instrument used to
produce this file is IRIS, the analyser is graphite and the reflection is 002. See the
:ref:`isis-energy-transfer-example-workflow`.

1. In the **Input** box, load the file named ``iris26176_graphite002_red``. This will
   automatically plot the data as a contour plot within the interface.

2. Set the **Q Low**, **Q Width** and **Q High** to be 0.5, 0.05 and 1.8. These values are
   read from the contour plot.

3. Tick **Rebin in Energy**.

4. Set the **E Low**, **E Width** and **E High** to be -0.5, 0.005 and 0.5. Again, these values
   should be read from the contour plot.

5. Click **Run** and wait for the interface to finish processing. This will perform an energy
   rebin before performing the :ref:`SofQW <algm-SofQW>` algorithm. The output workspace ends
   with suffix _sqw and is called ``iris26176_graphite002_sqw``.

6. Enter a list of workspace indices in the output options (e.g. 0-2,4,6-7) and then click
   **Plot Spectra** to plot spectra from the output workspace.

6. Click the down arrow on the **Plot Spectra** button, and select **Plot Contour**. This will
   produce a contour plot of the output workspace.

7. Choose a default save directory and then click **Save Result** to save the output workspace.
   The _sqw file is used in the :ref:`moments-example-workflow`.

Moments
-------

This interface uses the :ref:`SofQWMoments <algm-SofQWMoments>` algorithm to
calculate the :math:`n^{th}` moment of an :math:`S(Q, \omega)` workspace created
by the SofQW tab.

.. interface:: Data Reduction
  :widget: tabMoments

Moments Options
~~~~~~~~~~~~~~~

Input
  Allows you to select an :math:`S(Q, \omega)` file (*_sqw.nxs*) or workspace
  (*_sqw*) as the input to the algorithm.

Scale By
  Used to set an optional scale factor by which to scale the output of the
  algorithm.

EMin & EMax
  Used to set the energy range of the sample that the algorithm will use for
  processing.

Run
  Runs the processing configured on the current tab.

Plot Spectra
  If enabled, it will plot the selected workspace indices in the selected output workspace.

Save Result
  If enabled the result will be saved as a NeXus file in the default save directory.

.. _moments-example-workflow:

Moments Example Workflow
~~~~~~~~~~~~~~~~~~~~~~~~
The Moments tab operates on ``_sqw`` files. The file used in this workflow is produced during
the :ref:`sqw-example-workflow`.

1. In the **Input** box, load the file named ``irs26176_graphite002_sqw``. This will
   automatically plot the data in the first mini-plot.

2. Drag the blue sliders on the mini-plot so they are x=-0.4 and x=0.4.

3. Click **Run** and wait for the interface to finish processing. This will run the
   :ref:`SofQWMoments <algm-SofQWMoments>` algorithm. The output workspace ends
   with suffix _moments and is called ``iris26176_graphite002_moments``.

.. categories:: Interfaces Indirect