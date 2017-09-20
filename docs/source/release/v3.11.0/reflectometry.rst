=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Algorithms
----------

- The following bugs have been fixed in the summation in Q functionality in :ref:`algm-ReflectometryReductionOne`:

  - the incorrect angle was being used in the final conversion to Q in the divergent beam case
  - the input was being cropped, causing loss of counts
  - summation in Q was giving incorrect results for a point detector

- A new property, ``Diagnostics``, has been added to :ref:`algm-ReflectometryReductionOne` to allow the output of additional interim workspaces for debug purposes.
- :ref:`algm-LoadILLReflectometry` has been fixed to correctly load D17 files acquired in the TOF mode.

- A new version of :ref:`SpecularReflectionCalculateTheta <algm-SpecularReflectionCalculateTheta>` (version 2) has been added which works with detectors at :math:`2\theta`. Version 1 works with detectors at :math:`\theta`.

- The following changes have been made to :ref:`CalculateResolution <algm-NRCalculateSlitResolution>`:

  - The algorithm has been renamed to :ref:`algm-NRCalculateSlitResolution` as this algorithm is specific to neutron reflectometry, and the resolution it calculates is the slit resolution.
  - Some errors in the resolution calculation have been fixed. Note that this affects the Q binning in the results of :ref:`ReflectometryReductonOneAuto <algm-ReflectometryReductionOneAuto>` (versions 1 and 2) and :ref:`ReflectometryReductionOne <algm-ReflectometryReductionOne>` (version 1 only).
  - The ``TwoThetaLogName`` property has been replaced by ``ThetaLogName``. This still takes ``Theta`` as the default log name, which was causing confusion before because it was being used as two theta. Is is now being used as theta, as the new property name suggests.
  - The output property ``TwoThetaOut`` has been removed because it is not useful. The algorithm now returns a single value which is the resolution.


Reflectometry Reduction Interface
---------------------------------

ISIS Reflectometry
##################

- The interface can now operate asynchronously in that one can still interact with Mantid while data is processing (instead of freezing Mantid until it finished):

  - Reduction can be paused using the new 'Pause' button added to the interface. It may be resumed again by clicking on the 'Process' button again.
  - Data reduction must be paused first before the interface can be closed.
  - When reduction is paused, the interface will finish reducing the current row before pausing.
  - Changing item selection while paused will cause the newly selected items to be processed instead.
  - Altering data within a row while paused will set that row and its containing group unprocessed. Adding/removing rows from a group will also set the group unprocessed.
  - Deleting or renaming output workspaces of processed rows/groups will set that row/group unprocessed.

- During reduction, rows and groups that have been successfully processed are highlighted green.

- New 'autoreduce' button added for automatically reducing all runs obtained from a given investigation id.

  - With an id supplied, clicking 'autoreduce' searches for runs that are included in the investigation, transfers them to the processing table and processes all runs.
  - Button disabled while reduction in process. Can be re-enabled by pausing autoreduction, where clicking 'autoreduce' again will continue processing rows.
  - Changing the instrument, investigation id or transfer method while paused and clicking 'autoreduce' will start a new autoreduction.


ISIS Reflectometry (Old)
########################

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
