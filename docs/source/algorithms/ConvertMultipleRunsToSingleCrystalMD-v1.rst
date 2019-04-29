.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This workflow algorithm loads, converts to MDWorkspace and combines a
series of runs. The resulting workspace is a :ref:`MDWorkspace
<MDWorkspace>` containing a volume of scattering events. If a :ref:`UB
matrix <Lattice>` is provided and `QFrame='HKL'`, then the MD workspace
will be in HKL otherwise it will be in Q_sample.

This is **not** a correctly normalized workspace, for that look at
:ref:`SingleCrystalDiffuseReduction
<algm-SingleCrystalDiffuseReduction>`. The output workspace can be
used with :ref:`FindPeaksMD <algm-FindPeaksMD>` and from that
determine the :ref:`UB matrix <Lattice>`. The output workspace (if in
Q sample) can then be used with :ref:`MDNorm <algm-MDNorm>` to
correctly bin and normalise the data.

The input filename follows the syntax from
:py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>`

You should refer to :ref:`ConvertToMD <algm-ConvertToMD>` for all its
options. If MinValues and MaxValues are not provided then limits will
be calculated with :ref:`ConvertToMDMinMaxGlobal
<algm-ConvertToMDMinMaxGlobal>`

The resulting workspaces can be saved and loaded with :ref:`SaveMD
<algm-SaveMD>` and :ref:`LoadMD <algm-LoadMD>` respectively.

Masking
#######

The workspace will be masked by the provided masking file. A masking
file can be created my masking a data file then saving it using
:ref:`SaveMask <algm-SaveMask>`.

Usage
-----

**Single file Q-sample**

.. testcode::

   ConvertMultipleRunsToSingleCrystalMD(Filename='CNCS_7860',
                                        MinValues=[-2,-2,-2],
                                        MaxValues=[2,2,2],
                                        OutputWorkspace='output',
                                        SetGoniometer=True,
                                        Axis0="huber,0,1,0,1")
   ws=mtd['output']
   print("The workspace is in {}".format(ws.getSpecialCoordinateSystem()))
   print("There are {} experiment runs in the workspace".format(ws.getNumExperimentInfo()))
   print("Number of Events = {}".format(ws.getNEvents()))
   print("There are {} dimensions with names: {} {} {}".format(ws.getNumDims(), ws.getDimension(0).name, ws.getDimension(1).name, ws.getDimension(2).name))

Output:

.. testoutput::

   The workspace is in QSample
   There are 1 experiment runs in the workspace
   Number of Events = 100210
   There are 3 dimensions with names: Q_sample_x Q_sample_y Q_sample_z

**Multiple files Q-sample**

.. code-block:: python

   ConvertMultipleRunsToSingleCrystalMD(Filename='CORELLI_29782:29817:10',
                                        FilterByTofMin=1000,
                                        FilterByTofMax=16666,
                                        OutputWorkspace='output',
                                        SetGoniometer=True,
                                        Axis0="BL9:Mot:Sample:Axis1,0,1,0,1")
   ws=mtd['output']
   print("The workspace is in {}".format(ws.getSpecialCoordinateSystem()))
   print("There are {} experiment runs in the workspace".format(ws.getNumExperimentInfo()))
   print("Number of Events = {}".format(ws.getNEvents()))
   print("There are {} dimensions with names: {} {} {}".format(ws.getNumDims(), ws.getDimension(0).name, ws.getDimension(1).name, ws.getDimension(2).name))

Output:

.. code-block:: none

   The workspace is in QSample
   There are 4 experiment runs in the workspace
   Number of Events = 47223197
   There are 3 dimensions with names: Q_sample_x Q_sample_y Q_sample_z

**Single file HKL**

.. testcode::

   # Create a ISAW UB file for the test
   import mantid
   UBfilename=mantid.config.getString("defaultsave.directory")+"ConvertMultipleRunsToSingleCrystalMDTest.mat"
   with open(UBfilename,'w') as f:
       f.write("0.0  0.5  0.0  \n")
       f.write("0.0  0.0  0.25  \n")
       f.write("0.2  0.0  0.0  \n")
       f.write("2.0  4.0  5.0  90  90  90  40  \n")
       f.write("0.0  0.0  0.0   0   0   0   0  \n")
       f.write("\n\nsome text about IPNS convention")

   ConvertMultipleRunsToSingleCrystalMD(Filename='CNCS_7860',
                                        MinValues=[-2,-2,-2],
                                        MaxValues=[2,2,2],
                                        OutputWorkspace='output',
                                        SetGoniometer=True,
                                        Axis0="huber,0,1,0,1",
                                        UBMatrix=UBfilename,
                                        QFrame='HKL')
   ws=mtd['output']
   print("The workspace is in {}".format(ws.getSpecialCoordinateSystem()))
   print("There are {} experiment runs in the workspace".format(ws.getNumExperimentInfo()))
   print("Number of Events = {}".format(ws.getNEvents()))
   print("There are {} dimensions with names: {} {} {}".format(ws.getNumDims(), ws.getDimension(0).name, ws.getDimension(1).name, ws.getDimension(2).name))

Output:

.. testoutput::

   The workspace is in HKL
   There are 1 experiment runs in the workspace
   Number of Events = 112266
   There are 3 dimensions with names: [H,0,0] [0,K,0] [0,0,L]

**Multiple files HKL**

.. code-block:: python

   ConvertMultipleRunsToSingleCrystalMD(Filename='CORELLI_29782:29817:10',
                                        FilterByTofMin=1000,
                                        FilterByTofMax=16666,
                                        OutputWorkspace='output',
                                        SetGoniometer=True,
                                        Axis0="BL9:Mot:Sample:Axis1,0,1,0,1",
                                        UBMatrix="/SNS/CORELLI/IPTS-15526/shared/benzil_Hexagonal.mat",
                                        QFrame='HKL')
   ws=mtd['output']
   print("The workspace is in {}".format(ws.getSpecialCoordinateSystem()))
   print("There are {} experiment runs in the workspace".format(ws.getNumExperimentInfo()))
   print("Number of Events = {}".format(ws.getNEvents()))
   print("There are {} dimensions with names: {} {} {}".format(ws.getNumDims(), ws.getDimension(0).name, ws.getDimension(1).name, ws.getDimension(2).name))

Output:

.. code-block:: none

   The workspace is in HKL
   There are 4 experiment runs in the workspace
   Number of Events = 47223200
   There are 3 dimensions with names: [H,0,0] [0,K,0] [0,0,L]


Related Algorithms
------------------

:ref:`ConvertToMD <algm-ConvertToMD>` is used to Convert to MD

:ref:`SingleCrystalDiffuseReduction <algm-SingleCrystalDiffuseReduction>` does the correct normalisation for multiple runs

.. categories::

.. sourcelink::
