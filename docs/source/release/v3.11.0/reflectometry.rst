=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

ConvertToReflectometryQ
-----------------------


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
