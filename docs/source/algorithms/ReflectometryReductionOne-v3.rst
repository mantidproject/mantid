.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is not meant to be used directly by users. Please see
:ref:`algm-ReflectometryReductionOneAuto`, which is a facade over this
algorithm.

Version 3 is a refactor of :ref:`ReflectometryReductionOne version 2
<algm-ReflectometryReductionOne-v2>` into a task-based algorithm. The reduction
methodology and output workspaces are intended to be the same as version 2. See
:ref:`ReflectometryReductionOne version 2 <algm-ReflectometryReductionOne-v2>`
for the detailed description of the reduction workflow, including conversion to
wavelength, monitor normalisation, transmission correction, summation in Q, and
conversion to momentum transfer.

Task Execution Order
####################

Version 3 exposes the ``TaskExecutionOrder`` property. This property is
intended for advanced use. If it is left blank, the algorithm dynamically builds
the task execution order from the input workspace and reduction properties. If
it is provided, the custom task execution order is used instead.

When ``TaskExecutionOrder`` is blank, the generated order depends on:

* the input workspace X unit. A workspace already in wavelength is treated as
  partially reduced, so the initial extraction, summation, wavelength conversion,
  monitor normalisation, and transmission normalisation tasks are skipped;
* ``SummationType``. ``SumInQ`` uses the Q-summation task after the
  normalisation steps, while the default wavelength summation uses the detector
  summation task before wavelength conversion;
* monitor normalisation properties. ``TaskNormalizeByMonitor`` is included only
  when ``I0MonitorIndex``, ``MonitorBackgroundWavelengthMin``, and
  ``MonitorBackgroundWavelengthMax`` are all set;
* transmission and algorithmic correction properties. ``TaskNormalizeByTransmission``
  is included when ``FirstTransmissionRun`` is set. If ``CorrectionAlgorithm``
  is set, ``TaskNormalizeByAlgorithm`` is used instead;
* ``SubtractBackground``. ``TaskBackgroundSubtraction`` is included only when
  this property is set.

The available task names are:

* ``TaskExtractROI``
* ``TaskBackgroundSubtraction``
* ``TaskSumDetectors``
* ``TaskConvertToWavelength``
* ``TaskNormalizeByMonitor``
* ``TaskNormalizeByTransmission``
* ``TaskNormalizeByAlgorithm``
* ``TaskSumDetectorsInQ``
* ``TaskCropWavelength``
* ``TaskConvertToQ``

Custom task orders must use valid task names and must include the prerequisite
tasks needed to satisfy each task's input dependencies. Knowledge of the implementation of the task-based structure
is therefore required.

For routine reductions, leave ``TaskExecutionOrder`` blank so that the algorithm selects the same
workflow as version 2 from the supplied properties.

Previous Versions
-----------------

This is version 3 of the algorithm. For version 2, please see `here
<ReflectometryReductionOne-v2.html>`_.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Reduce a run using the dynamically generated task order**

.. testcode:: ExReflRedOneDynamicTEO

   run = Load(Filename='INTER00013460.nxs')
   IvsQ, IvsLam = ReflectometryReductionOne(InputWorkspace=run,
                                            WavelengthMin=1.0,
                                            WavelengthMax=17.0,
                                            ProcessingInstructions='4',
                                            I0MonitorIndex=2,
                                            MonitorBackgroundWavelengthMin=15.0,
                                            MonitorBackgroundWavelengthMax=17.0,
                                            MonitorIntegrationWavelengthMin=4.0,
                                            MonitorIntegrationWavelengthMax=10.0)

   print("{:.4f}".format(IvsLam.readY(0)[533]))
   print("{:.4f}".format(IvsLam.readY(0)[534]))
   print("{:.4f}".format(IvsQ.readY(0)[327]))
   print("{:.4f}".format(IvsQ.readY(0)[328]))


Output:

.. testoutput:: ExReflRedOneDynamicTEO

   0.0003
   0.0003
   0.0003
   0.0003


**Example - Reduce a run using a custom task execution order**

.. testcode:: ExReflRedOneCustomTEO

   run = Load(Filename='INTER00013460.nxs')
   task_order = [
       "TaskExtractROI",
       "TaskSumDetectors",
       "TaskConvertToWavelength",
       "TaskNormalizeByMonitor",
       "TaskCropWavelength",
       "TaskConvertToQ",
   ]
   IvsQ, IvsLam = ReflectometryReductionOne(TaskExecutionOrder=task_order,
                                            InputWorkspace=run,
                                            WavelengthMin=1.0,
                                            WavelengthMax=17.0,
                                            ProcessingInstructions='4',
                                            I0MonitorIndex=2,
                                            MonitorBackgroundWavelengthMin=15.0,
                                            MonitorBackgroundWavelengthMax=17.0,
                                            MonitorIntegrationWavelengthMin=4.0,
                                            MonitorIntegrationWavelengthMax=10.0)

   print("{:.4f}".format(IvsLam.readY(0)[533]))
   print("{:.4f}".format(IvsLam.readY(0)[534]))
   print("{:.4f}".format(IvsQ.readY(0)[327]))
   print("{:.4f}".format(IvsQ.readY(0)[328]))


Output:

.. testoutput:: ExReflRedOneCustomTEO

   0.0003
   0.0003
   0.0003
   0.0003

.. categories::

.. sourcelink::
