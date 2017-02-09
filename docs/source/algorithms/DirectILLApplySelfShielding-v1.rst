.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm subtracts empty container data and applies self-shielding corrections to *InputWorkspace*. It can work in three different modes depending on the input workspaces supplied:

* **Only *EmptyContainerWorkspace* is given**: no self-shielding corrections, simply subtract empty container.
* **No *EmptyContainerWorkspace*, but *SelfShieldingCorrectionWorkspace* is given**: only apply self-shielding corrections to *InputWorkspace*.
* **Both *EmptyContainerWorkspace* and *SelfShieldingCorrectionWorkspace* are given**: apply self-shielding corrections and subtract empty container.

In all modes where *EmptyContainerWorkspace* is supplied, the container data is multiplied by *EmptyContainerScaling* before subtraction.

*SelfShieldingCorrectionWorkspace* can be obtained from the :ref:`DirectILLSelfShielding <algm-DirectILLSelfShielding>` algorithm.

Usage
-----

**Example - Not implemented**

.. categories::

.. sourcelink::
