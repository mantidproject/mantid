.. _05_using_mantid_algorithms:

=======================
Using Mantid Algorithms
=======================

Any of the defined algorithms in Mantid, this includes other extensions,
can be used as part of your new algorithm. To call an algorithm simply use
the standard syntax for calling algorithms in Mantid, i.e. the algorithm
name followed by the arguments.

A good example might be wanting to load a file, as
:ref:`Load <algm-Load-v1>` is a very flexible algorithm:

.. code-block:: python

    from mantid.kernel import *
    from mantid.api import *

    class LoadAndDoSomething(PythonAlgorithm):

        def PyInit(self):
            self.declareProperty(FileProperty("Filename", "",
                                              action=FileAction.Load))
            self.declareProperty(MatrixWorkspaceProperty(
                                        "OutputWorkspace",
                                        "",
                                        direction=Direction.Output))
            # ... other stuff

        def PyExec(self):
            from mantid.simpleapi import Load, Scale, DeleteWorkspace

            _tmpws = Load(Filename=self.getPropertyValue("Filename"))
            _tmpws = Scale(InputWorkspace=_tmpws, Factor=100)

            # Sets reference externally and sets the name to that
            # given by the OutputWorkspace property
            self.setProperty("OutputWorkspace", _tmpws)

            # Removes temporary reference created here
            # (doesn't delete workspace)
            DeleteWorkspace(_tmpws)

The algorithm defines a single output workspace property called
``OutputWorkspace``. This property needs to be linked to the produced output
of the algorithm, which is done using ``self.setProperty``.

You've probably noticed we import the algorithms you will be using in
``PyExec`` instead of at the top of this file. Whilst this is a contradiction
of PEP8 it allows us to import the various algorithms we need when the
algorithm is run instead of when Mantid is loaded.
