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

- During reduction, rows and groups that have been successfully processed are highlighted green.


ISIS Reflectometry (Old)
########################

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.11%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
