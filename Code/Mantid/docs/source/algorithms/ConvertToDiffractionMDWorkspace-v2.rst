.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm converts from a `MatrixWorkspace <http://mantidproject.org/MatrixWorkspace>`__ (in
any input units) into `MDWorkspace <http://mantidproject.org/MDWorkspace>`__ containing 
3D events in reciprocal space.

The calculations apply only to elastic diffraction experiments. The
conversion can be done either to Q-space in the lab or sample frame, or
to HKL of the crystal.

Version 2 of the algorithm is the wrapper around :ref:`algm-ConvertToMD` algorithm, used for
diffraction workflow and for supporting the interface of the previous specialized version of this 
algorithm.  Old specialized version of this algorithm also exists.

See the :ref:`algm-ConvertToDiffractionMDWorkspace-v1` for details of the old and  :ref:`algm-ConvertToMD` for this algorithms implementations. 
 
The main difference between the results produced by the version one and two of this algorithm 
is the type of the workspace, produced by default. 
Version one is producing `MDLeanEvent<3> <http://www.mantidproject.org/MDWorkspace#Description%20of%20MDWorkspace>`__-s workspace 
and this version generates `MDEvent<3> <http://www.mantidproject.org/MDWorkspace#Description%20of%20MDWorkspace>`__-s workspace.

To obtain a workspace containing `MDLeanEvent<3> <http://www.mantidproject.org/MDWorkspace#Description%20of%20MDWorkspace>`__-s, 
and fine-tune the output workspace properties, 
one has to create OutputWorkspace using :ref:`algm-CreateMDWorkspace` algorithm first.

 

Types of Conversion
###################

-  **Q (lab frame)**: this calculates the momentum transfer (ki-kf) for
   each event is calculated in the experimental lab frame.
-  **Q (sample frame)**: the goniometer rotation of the sample is taken
   out, to give Q in the frame of the sample. See
   :ref:`algm-SetGoniometer` to specify the goniometer used in
   the experiment.
-  **HKL**: uses the UB matrix (see :ref:`algm-SetUB`,
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

This correction is also done by the
:ref:`algm-AnvredCorrection` algorithm, and will be set to
false if that algorithm has been run on the input workspace.

Usage 

**Example - Convert re-binned MARI 2D workspace to 3D MD workspace for further analysis/merging with data at different temperatures :**

.. testcode:: ExConvertToDiffractionMDWorkspace

   # create or load event workspace
   events = CreateSampleWorkspace(OutputWorkspace='events', WorkspaceType='Event', Function='Multiple Peaks')
   # convert to  MD workspace
   md = ConvertToDiffractionMDWorkspace(InputWorkspace=events, OutputWorkspace='md', OneEventPerBin=False, LorentzCorrection=True, SplitThreshold=150)

   # A way to look at these results as a text:
   print "Resulting MD workspace has {0} events and {1} dimensions".format(md.getNEvents(),md.getNumDims())
   print "Workspace Type is: ",md.id()   

**Output:**

.. testoutput:: ExConvertToDiffractionMDWorkspace

   Resulting MD workspace has 520128 events and 3 dimensions
   Workspace Type is:  MDEventWorkspace<MDEvent,3>


.. categories::
