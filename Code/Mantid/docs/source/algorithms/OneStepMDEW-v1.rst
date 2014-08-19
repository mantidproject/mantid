.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used in the Paraview event nexus loader to both load
an event nexus file and convert it into a `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_ for use in visualization.

The :ref:`algm-LoadEventNexus` algorithm is called with default
parameters to load into an :ref:`EventWorkspace <EventWorkspace>`.

After, that,  :ref:`algm-ConvertToDiffractionMDWorkspace` algorithm is called with the new
EventWorkspace as input. The parameters are set to convert to **Q** in the lab frame, 
with Lorentz correction, and default size/splitting behavior parameters.

Usage
-----

Used internaly. See examples of :ref:`algm-LoadEventNexus` and  :ref:`algm-ConvertToDiffractionMDWorkspace` algorithms
for the details of the usage of these algorithms


.. categories::
