.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm converts from a :ref:`MatrixWorkspace <MatrixWorkspace>` (in
any input units) into a :ref:`MDWorkspace <MDWorkspace>` containing
3D events in reciprocal space.

The calculations apply only to elastic diffraction experiments. The
conversion can be done either to Q-space in the lab or sample frame, or
to HKL of the crystal.

Version 3 of this algorithm will by default automatically calculate the extents
of the MD workspace using the :ref:`algm-ConvertToMDMinMaxLocal` algorithm.
Old version of this algorithm (version 2) also still exists which uses the fixed
bounds +/- 50. See the :ref:`algm-ConvertToDiffractionMDWorkspace-v2` for
details of the old implementation and :ref:`algm-ConvertToMDMinMaxLocal` for
information on how extents are now calculated.


Types of Conversion
###################

-  **Q (lab frame)**: this calculates the momentum transfer (ki-kf) for
   each event is calculated in the experimental lab frame.
-  **Q (sample frame)**: the goniometer rotation of the sample is taken
   out, to give Q in the frame of the sample. See
   :ref:`algm-SetGoniometer` to specify the goniometer used in
   the experiment.
-  **HKL**: uses the :ref:`UB matrix <Lattice>` (see :ref:`algm-SetUB`,
   :ref:`algm-FindUBUsingFFT` and others) to calculate the HKL
   Miller indices of each event.

Lorentz Correction
##################

If selected, the following Lorentz correction factor is applied on each
event by multiplying its weight by L:

:math:`L = \frac{ sin(\theta)^2 } { \lambda^{4} }`

Where :math:`\theta` is *half* of the neutron scattering angle
(conventionally called :math:`2\theta`). :math:`\lambda` is the neutron
wavelength in *Angstroms*.

This correction is also done by the :ref:`algm-AnvredCorrection` algorithm, and
will be set to false if that algorithm has been run on the input workspace.

Usage

**Example - Convert re-binned MARI 2D workspace to 3D MD workspace for further analysis/merging with data at different temperatures :**

.. testcode:: ExConvertToDiffractionMDWorkspace

   # create or load event workspace
   events = CreateSampleWorkspace(OutputWorkspace='events', WorkspaceType='Event', Function='Multiple Peaks')
   # convert to  MD workspace
   md = ConvertToDiffractionMDWorkspace(InputWorkspace=events, OutputWorkspace='md', OneEventPerBin=False, LorentzCorrection=True, SplitThreshold=150)

   # A way to look at these results as a text:
   print("Resulting MD workspace has {0} events and {1} dimensions".format(md.getNEvents(),md.getNumDims()))
   print("Workspace Type is:  {}".format(md.id()))

**Output:**

.. testoutput:: ExConvertToDiffractionMDWorkspace
   :options: +ELLIPSIS

   Resulting MD workspace has 81... events and 3 dimensions
   Workspace Type is:  MDEventWorkspace<MDEvent,3>


.. categories::

.. sourcelink::
