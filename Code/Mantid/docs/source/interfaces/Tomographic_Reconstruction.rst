Data Comparison
===============

.. contents:: Table of Contents
  :local:

Overview
--------

This interface aims at integrating and simplifying the following tasks
related to tomographic reconstruction and imaging with neutrons:


while much of its functionality is being developed in a generic way,
it is presently being tested and trialed for the IMAT instrument at
ISIS.

.. interface:: Tomographic Reconstruction
  :align: right

An important feature of this interface is the capability to submit
jobs to a remote compute resource (a compute cluster for
example). Currently remote jobs are run on the `SCARF cluster
<http://www.scarf.rl.ac.uk/>`_, administered by the Scientific
Computing Department of STFC. You can also use this cluster via remote
login and through its `web portal <https://portal.scarf.rl.ac.uk/>`_.


Interface at a glance
---------------------

By default the interface shows the *Run* tab, where you can visualize
images, submit reconstruction jobs, see and control the status of the
jobs submitted recently.

.. interface:: Tomographic Reconstruction
  :widget: runTab
  :align: right

In the setup tab you can set the details of the remote and/or local
compute resources. Importantly, here is where you can set you username
and password to log into the remote compute resource.
          
.. interface:: Tomographic Reconstruction
  :widget: setupTab
  :align: right


Tools
-----

At the moment two reconstruction tools are being set up and trialed on
SCARF:

* `TomoPy <https://www1.aps.anl.gov/Science/Scientific-Software/TomoPy>`_
* `Astra Toolbox <https://www1.aps.anl.gov/Science/Scientific-Software/TomoPy>`_

In the near future it is expected that support will be added for `Savu
<https://github.com/DiamondLightSource/Savu>`_, developed at the
Diamond Light Source.

Data locations
--------------

This is dependent on the facility and instrument.

TODO: this is work in progress. In principle data will be replicated
in the ISIS archive, the SCARF imat disk space, and possibly an
analysis machine located in R3.

Running jobs remotely
---------------------

Running jobs locally
--------------------

Example
-------

TODO: ideally, come up with a good and small example data set.

TomoPy
------

TODO: how to use it. Hints.

Astra Toolbox
-------------

TODO: how to use it. Hints.

Astra Toolbox
-------------

TODO: how to use it. Hints.

Savu
----

TODO: how to use it. Hints.

Uses a specific file format, the DLS NXTomo. A few examples can be
found at
`<https://github.com/DiamondLightSource/Savu/tree/master/test_data>`__.

Pipeline configuration
~~~~~~~~~~~~~~~~~~~~~~
