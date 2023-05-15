=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Event Filtering
---------------

During the v6.7 development cycle, there were major changes to the code underlying event filtering that was originally designed `here <https://github.com/mantidproject/mantid/issues/34794>`_.
The main changes to mantid are as follows:

* A ``TimeROI`` object that contains a list of use and ignore regions in time. The times are ``[inclusive, exclusive)``.
* When filtering/splitting logs in the ``Run`` object, the logs are copied as is and a new ``TimeROI`` object is set on the ``Run``. Values that are active during a ROI are kept as are one before/after each ROI. The main change is that logs no longer have fake values to mimic the filtering/splitting.
* Code should change from asking a ``TimeSeriesProperty`` for its statistics to asking the ``Run`` object for the statistics of a log because the ``Run`` is the only place that knows the information about the ``TimeROI``.
* When filtering/splitting logs, the filters are now ``[inclusive, exclusive)`` where previous behavior was ``[inclusive, inclusive]``. The previous behavior was inconsistent with how events are filtered. This change, fixes issues observed with the integrated proton charge.
* When workspaces are added, the resulting ``TimeROI`` is the union of the individual ``TimeROI``s. If either workspace had a ``TimeROI`` with ``TimeROI.useAll()==True``, it is assumed to be from that workspace's start-time to end-time.
* Arithmetic statistics (e.g. simple mean rather than time weighted) will be largely unchanged and are not necesarily correct


The algorithms modified as part of this are :ref:`FilterByLogValue <algm-FilterByLogValue>`, :ref:`FilterByTime <algm-FilterByTime>`, and :ref:`FilterEvents <algm-FilterEvents>`.
The "sample log viewer" has been modified to optionally show the ``TimeROI`` as an overlay on the logs.

Algorithms
----------

New features
############
.. amalgamate:: Framework/Algorithms/New_features

Bugfixes
############
.. amalgamate:: Framework/Algorithms/Bugfixes


Beanline
--------

New features
############
.. amalgamate:: Framework/Beamline/New_features

Bugfixes
########
.. amalgamate:: Framework/Beamline/Bugfixes


Fit Functions
-------------

New features
############
.. amalgamate:: Framework/Fit_Functions/New_features

Bugfixes
############
.. amalgamate:: Framework/Fit_Functions/Bugfixes


Data Objects
------------

New features
############
.. amalgamate:: Framework/Data_Objects/New_features

Bugfixes
########
.. amalgamate:: Framework/Data_Objects/Bugfixes


Geometry
--------

New features
############
.. amalgamate:: Framework/Geometry/New_features

Bugfixes
############
.. amalgamate:: Framework/Geometry/Bugfixes


Python
------

New features
############
.. amalgamate:: Framework/Python/New_features

Bugfixes
############
.. amalgamate:: Framework/Python/Bugfixes


MantidWorkbench
---------------

See :doc:`mantidworkbench`.
:ref:`Release 6.7.0 <v6.7.0>`
