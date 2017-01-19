.. _CachingLayers:

==============================================
Caching Layers - SpectrumInfo and DetectorInfo
==============================================

.. contents::
  :local:

Introduction
------------

There are two caching layers that are going to be introduced to Mantid as part of the work towards Instrument 2.0, `SpectrumInfo` and `DetectorInfo`. Eventually these classes will cache commonly accessed information about spectra or detectors, namely masking, monitor flags, L1, L2, 2-theta and position. Caching the values in these layers will give performance increases, as geometry calculations do not need to be performed every time.

Motivation
##########

``SpectrumInfo`` and ``DetectorInfo`` are currently implemented as wrapper classes in Mantid. Rolling out the new interfaces now will help with the eventual rollout of Instrument 2.0. It provides cleaner code and will also help with the rollout of indexing changes. It is also necessary to provide the indexing changes required to support scanning instruments.

SpectrumInfo
############

``SpectrumInfo`` can be obtained from a call to ``MatrixWorkspace::spectrumInfo()``. The wrapper class holds a reference to a ``DetectorInfo`` object and calls through to this for access to information on masking, monitor flags etc.

DetectorInfo
############

``DetectorInfo`` can be obtained from a call to ``ExperimentInfo::detectorInfo()``. The wrapper class holds a reference to the parametrised instrument for retrieving the relevant information.

There is also a real ``DetectorInfo`` class that has masking implemented. This is not important for the discussion the rollout, and all masking operations are already going via the new caching layers.

Changes for Rollout
-------------------

SpectrumInfo
############

Basics
______

All instances of ``MatrixWorkspace::getDetector()`` should be replaced using calls to the ``SpectrumInfo`` object obtained from ``MatrixWorkspace::spectrumInfo()``. The methods below can then be called on `SpectrumInfo` instead of on the detector.

* ``isMonitor(wsIndex)``
* ``isMasked(wsIndex)``
* ``l2(wsIndex)``
* ``twoTheta(wsIndex)``
* ``signedTwoTheta(wsIndex)``
* ``position(wsIndex)``

The try/catch pattern for checking if a spectrum has a detector might look something like the following before/after example.

**Before refactoring**

.. code-block:: c++

  try {
    Geometry::IDetector_const_sptr det = ws.getDetector(wsIndex);
    // do stuff
  catch (Exception::NotFoundError &) {
    // do exception
    return false;
  }

**After - check it has some detectors**

.. code-block:: c++

  if (spectrumInfo.hasDetectors(wsIndex)) {
    // do stuff
  } else {
    // do exception
    return false;
  }

In this case, which is generally more common, the check is for some detectors. It is also possible to check for the existence of a unique detector, see the alternative after example below.

**After - check for a unique detector**

.. code-block:: c++

  if (!spectrumInfo.hasUniqueDetector(wsIndex)) {
    // do exception
    return false;
  }

  // do stuff


Access to Detectors
___________________

The ``detector()`` method on ``SpectrumInfo`` returns the parameterised detector for the workspace. This can be used for doing thigs like moving a component.

``SpectrumInfo`` does not provide access to things like ``Object::solidAngle()``. The ``detector()`` method on ``SpectrumInfo`` can also be used to get access to these methods.

Useful Tips
___________

* Creation of ``SpectrumInfo`` objects is not cheap. Make sure ``workspace.spectrumInfo()`` is called outside of loops that go over all spectra.
* If a ``SpectrumInfo`` object is required for different workspaces then include the workspace name in the name of the ``SpectrumInfo`` object to avoid confusion.
* Get the ``SpectrumInfo`` object as a const reference and use auto - ``const auto &spectrumInfo = workspace->spectrumInfo();``.
* Do not forget to add the import - ``#include "MantidAPI/SpectrumInfo.h"``.

Complete Examples
_________________

* `CreatePSDBleedMask.cpp <https://github.com/mantidproject/mantid/pull/18218/files#diff-f490acf06e76f93898dc7d486c8dfa93>`_

* `HRPDSlabCanAbsorption.cpp <https://github.com/mantidproject/mantid/pull/18464/files#diff-fc151838d9d7cc2e4ea469e98472c791>`_

DetectorInfo
############

Basics
______

Access to Detectors
___________________

Useful Tips
___________

Complete Examples
_________________

* `???.cpp <https://github.com/mantidproject/mantid/pull/...>`_

* `???.cpp <https://github.com/mantidproject/mantid/pull/...>`_

Rollout status
--------------

See ticket `17743 <https://github.com/mantidproject/mantid/issues/17743>`_ for an overview of the ``SpectrumInfo`` rollout, including completed and algorithms, and remaining algorithms. Please follow the instructions on the ticket for the rollout.

For ``DetectorInfo`` rollout see ticket `????? <https://github.com/mantidproject/mantid/issues/?????>`_. Again, please follow instructions there for the rollout.


Dealing with problems
---------------------

Please get in touch with Ian Bush, Simon Heybrock or Owen Arnold for any questions about the rollout.



.. categories:: Concepts
