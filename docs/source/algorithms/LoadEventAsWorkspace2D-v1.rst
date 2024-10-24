
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------


This algorithm loads event data, integrating the events during loading. It will also set the X-axis based on log data. This will be mostly used by contant wavelength instruments that collect event data but are not performing event filtering. It will be faster than running :ref:`LoadEventNexus <algm-LoadEventNexus>` then doing :ref:`Integration <algm-Integration>`.

An example usage is instead of running :ref:`LoadEventNexus <algm-LoadEventNexus>` then :ref:`HFIRSANS2Wavelength <algm-HFIRSANS2Wavelength>`, you can just use ``LoadEventAsWorkspace2D``.

The X-axis is set based on the either the provided parameters ``XCenter`` and ``XWidth`` of the mean of the log values defined by ``XCenterLog`` and ``XWidthLog``. The X-axis bin will be centered on ``XCenter`` with a width of ``XCenter*XWidth``.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - LoadEventAsWorkspace2D compared to LoadEventNexus+HFIRSANS2Wavelength**

.. code-block:: python

   ws = LoadEventAsWorkspace2D("CG3_16931")

   ws2 = LoadEventNexus("CG3_16931")
   ws2 = HFIRSANS2Wavelength(ws2)

   CompareWorkspaces(Workspace1=ws, Workspace2=ws2)

.. categories::

.. sourcelink::
