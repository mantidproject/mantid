.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm creates a sliced workspaces from an event-based SANS input workspace according to the settings in the state object.
The algorithm will extract a slice based on a start and end time which are set in the state object. In addtion the data type, ie
if the slice is to be taken from a sample or a can workspace can be specified. Note that the monitor workspace is not being sliced but scaled by the ratio of the proton charge of the sliced worspace to the full workspace. Currently the mask mechanism is implemented for **SANS2D**, **LOQ** and **LARMOR**.


Relevant SANSState entries for SANSSlice
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The required information for the slice operation is retrieved from a state object.


The elements of the slice state are:

+-------------+---------------+-------------------------------------------------------+------------+---------------+
| Entry       | Type          | Description                                           | Mandatory  | Default value |
+=============+===============+=======================================================+============+===============+
| start_time  | List of Float | The first entry is used as a start time for the slice | No         | None          |
+-------------+---------------+-------------------------------------------------------+------------+---------------+
| end_time    | List of Float | The first entry is used as a stop time for the slice  | No         | None          |
+-------------+---------------+-------------------------------------------------------+------------+---------------+


**Note that these settings should be only populated via the GUI or the Python Interface of ISIS SANS.**

Slice options for the data type: *Sample*, *Can*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The *Sample* setting performs regular slicing

The *Can* setting does not apply slicing to the specified workspaces.


.. categories::

.. sourcelink::
