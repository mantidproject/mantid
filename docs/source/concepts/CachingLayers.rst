.. _CachingLayers:

==============================================
Caching Layers - SpectrumInfo and DetectorInfo
==============================================

.. contents::
  :local:

Introduction
------------

There are two caching layers, ``SpectrumInfo`` and ``DetectorInfo``, that are going to be introduced to Mantid as part of the work towards Instrument 2.0. Eventually these classes will cache commonly accessed information about spectra or detectors, namely masking, monitor flags, L1, L2, 2-theta and position, leading to improved performance and cleaner code.

Current Status
##############

``SpectrumInfo`` and ``DetectorInfo`` are currently implemented as wrapper classes in Mantid. Using the new interfaces everywhere now will help with the eventual rollout of Instrument 2.0. It also provides cleaner code and will help with the rollout of planned indexing changes. It is also necessary to provide the addition of time indexing, required to support scanning instruments.

SpectrumInfo
____________

``SpectrumInfo`` can be obtained from a call to ``MatrixWorkspace::spectrumInfo()``. The wrapper class holds a reference to a ``DetectorInfo`` object and calls through to this for access to information on masking, monitor flags etc.

DetectorInfo
____________

``DetectorInfo`` can be obtained from a call to ``ExperimentInfo::detectorInfo()`` (usually this would be called on a ``MatrixWorkspace``). The wrapper class holds a reference to the parametrised instrument for retrieving the relevant information.

There is also a real ``DetectorInfo`` class that has masking implemented, which the ``DetectorInfo`` holds a reference to. This is not important for the discussion of the rollout, but all masking operations are now going via the real ``DetectorInfo`` layer.

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

  #include "MantidAPI/SpectrumInfo.h"

  ...

  const aut &spectrumInfo = ws->spectrumInfo();

  if (spectrumInfo.hasDetectors(wsIndex)) {
    // do stuff
  } else {
    // do exception
    return false;
  }

In this case, which is generally more common, the check is for at least one detector. It is also possible to check for the existence of a unique detector, see the alternative after example below.

**After - check for a unique detector**

.. code-block:: c++

  #include "MantidAPI/SpectrumInfo.h"

  ...

  const aut &spectrumInfo = ws->spectrumInfo();

  if (!spectrumInfo.hasUniqueDetector(wsIndex)) {
    // do exception
    return false;
  }

  // do stuff


Access to Detectors
___________________

The ``detector(wsIndex)`` method on ``SpectrumInfo`` returns the parameterised detector or detector group for the workspace. This can be used for doing things like moving a component.

``SpectrumInfo`` does not provide access to things like ``Object::solidAngle()``. The ``detector()`` method on ``SpectrumInfo`` can also be used to get access to these methods.

Useful Tips
___________

* Creation of ``SpectrumInfo`` objects is not cheap. Make sure ``ws.spectrumInfo()`` is called outside of loops that go over all spectra.
* If a ``SpectrumInfo`` object is required for more than one workspace then include the workspace name in the name of the ``SpectrumInfo`` object, to avoid confusion.
* Get the ``SpectrumInfo`` object as a const reference and use auto - ``const auto &spectrumInfo = ws->spectrumInfo();``.
* Do not forget to add the import - ``#include "MantidAPI/SpectrumInfo.h"``.

Complete Examples
_________________

* `CreatePSDBleedMask.cpp <https://github.com/mantidproject/mantid/pull/18218/files#diff-f490acf06e76f93898dc7d486c8dfa93>`_

* `HRPDSlabCanAbsorption.cpp <https://github.com/mantidproject/mantid/pull/18464/files#diff-fc151838d9d7cc2e4ea469e98472c791>`_

DetectorInfo
############

Basics
______

The conversion is similar to that for ``SpectrumInfo``. For ``DetectorInfo`` all instances of ``Instrument::getDetector()`` should be replaced using calls to the ``DetectorInfo`` object obtained from ``MatrixWorkspace::detectorInfo()``. The methods below can then be called on ``DetectorInfo`` instead of on the detector.

* ``isMonitor(detIndex)``
* ``isMasked(detIndex)``
* ``l2(detIndex)``
* ``twoTheta(detIndex)``
* ``signedTwoTheta(detIndex)``
* ``position(detIndex)``

**Indexing**

The ``DetectorInfo`` object is accessed by an index going from 0 to the number of detectors. A vector of detector IDs can be obtained from a call to ``detectorInfo.detectorIDs()``. Alternatively the method ``detectorInfo.indexOf(detID)`` can be used to get the index for a particular detector ID. Examples of both of these are given below.

Below are example refactorings with different approaches for the indexing.

**Before refactoring**

.. code-block:: c++

  auto instrument = ws->getInstrument();
  if (!instrument)
    throw runtime_error("There is no instrument in input workspace.")

  size_t numdets = instrument->getNumberDetectors();
  vector<detid_t> detIds = instrument->getDetectorIDs();

  for (size_t i = 0; i < numdets; ++i) {
    IDetector_const_sptr tmpdetector = instrument->getDetector(detIds[i]);
    if (tmpdetector->isMasked()) {
      maskeddetids.push_back(tmpdetid);
    }
  }

**After - looping over index**

.. code-block:: c++

  #include "MantidAPI/Detector.h"

  ...

  const auto &instrument = ws->detectorInfo();
  if (detectorInfo.size() == 0)
    throw runtime_error("There is no instrument in input workspace.")

  vector<detid_t> detIds = detectorInfo.detectorIDs();

  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if (detectorInfo.isMasked(i)) {
      maskedDetIds.push_back(detIds[i]);
    }
  }

**After - looping over detector IDs**

.. code-block:: c++

  #include "MantidAPI/Detector.h"

  ...

  const auto &instrument = ws->detectorInfo();
  if (detectorInfo.size() == 0)
    throw runtime_error("There is no instrument in input workspace.")

  vector<detid_t> detIds = detectorInfo.detectorIDs();

  for (detid_t detId : detIds) {
    if (detectorInfo.isMasked(detectorInfo.indexOf(detId))) {
      maskedDetIds.push_back(detId);
    }
  }

Access to Detectors
___________________

As for the ``SpectrumInfo`` object ``DetectorInfo`` can return a parameterised detector for the workspace.

Useful Tips
___________

See tips for ``SpectrumInfo`` - the same advice applies to using ``DetectorInfo``.

Complete Examples
_________________

* `SmoothNeighbours.cpp <https://github.com/mantidproject/mantid/pull/18295/files#diff-26a49ef923e1bdd77677b962528d1e01>`_

* `ClaerMaskFlag.cpp <https://github.com/mantidproject/mantid/pull/18198/files#diff-7f0f739ba6db714eb6ef64b6125e4620>`_

Rollout status
--------------

See ticket `17743 <https://github.com/mantidproject/mantid/issues/17743>`_ for an overview of the ``SpectrumInfo`` rollout, including completed and algorithms, and remaining algorithms. Please follow the instructions on the ticket for the rollout.

For ``DetectorInfo`` rollout see ticket `????? <https://github.com/mantidproject/mantid/issues/?????>`_. Again, please follow instructions there for the rollout.


Dealing with problems
---------------------

Please get in touch with Ian Bush, Simon Heybrock or Owen Arnold for any questions about the caching layers and rollout.


.. categories:: Concepts
