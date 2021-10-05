.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that reads information from the sample logs before calling :ref:`SetSample <algm-SetSample>`.

Usage
-----

**Example - get all information from logs**

This assumes that all of the logs are present and contain valid information.
The environment is partially specified as ``InAir`` so the framework knows to look into the correct `environement file <https://github.com/mantidproject/mantid/blob/master/instrument/sampleenvironments/SNS/InAir.xml>`_.

.. code-block:: python

   material = {}
   environment = {"Name": "InAir"}
   geometry = {}
   SetSampleFromLogs(InputWorkspace=wksp, Environment=environment,
                     Material=material, Geometry=geometry)


**Example - specify all information**

This could done using :ref:`SetSample <algm-SetSample>` directly.

.. code-block:: python

   material = {"ChemicalFormula": "Si", "SampleMassDensity": 1.165}
   environment = {"Name": "InAir", "Container": "PAC06"}
   geometry = {"Height": 4.}  # cm

   SetSampleFromLogs(InputWorkspace=wksp, Environment=environment,
                     Material=material, Geometry=geometry)

.. categories::

.. sourcelink::
