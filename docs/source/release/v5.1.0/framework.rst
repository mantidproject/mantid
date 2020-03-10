=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------

- Add specialization to :ref:`SetUncertainties <algm-SetUncertainties>` for the case where InputWorkspace == OutputWorkspace.

Data Objects
------------

Python
------
- A list of spectrum numbers can be got by calling getSpectrumNumbers on a 
  workspace. For example: spec_nums = ws.getSpectrumNumbers()

:ref:`Release 5.1.0 <v5.1.0>`
