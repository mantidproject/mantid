.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used in the Paraview event nexus loader to both load
an event nexus file and convert it into a
`MDEventWorkspace <MDEventWorkspace>`__ for use in visualization.

The :ref:`algm-LoadEventNexus` algorithm is called with default
parameters to load into an `EventWorkspace <EventWorkspace>`__.

After, the
`MakeDiffractionMDEventWorkspace <MakeDiffractionMDEventWorkspace>`__
algorithm is called with the new EventWorkspace as input. The parameters
are set to convert to Q in the lab frame, with Lorentz correction, and
default size/splitting behavior parameters.

.. categories::
