.. _InstrumentAccessLayers:

===================================================
Instrument Access via SpectrumInfo and DetectorInfo
===================================================

.. contents::
  :local:

Introduction
------------

There are two new layers to access instrument information, ``SpectrumInfo`` and ``DetectorInfo``, that are going to be introduced to Mantid as part of the work towards Instrument 2.0. Eventually these classes will store commonly accessed information about spectra or detectors, namely masking, monitor flags, L1, L2, 2-theta and position, leading to improved performance and cleaner code.

A spectrum corresponds to (a group of) one or more detectors. Most algorithms work with spectra and thus ``SpectrumInfo`` would be used. Some algorithms work on a lower level (with individual detectors) and thus ``DetectorInfo`` would be used.

In many cases direct access to ``Instrument`` can be removed by using these layers. This will also help in moving to using indexes for enumeration, and only working with IDs for user-facing input.

Current Status
##############

``SpectrumInfo`` and ``DetectorInfo`` are currently implemented as wrapper classes in Mantid. Using the new interfaces everywhere now will help with the eventual rollout of Instrument 2.0. It also provides cleaner code and will help with the rollout of planned indexing changes. It is also necessary to provide the addition of time indexing, required to support scanning instruments.

SpectrumInfo
____________

``SpectrumInfo`` can be obtained from a call to ``MatrixWorkspace::spectrumInfo()``. The wrapper class holds a reference to a ``DetectorInfo`` object and calls through to this for access to information on masking, monitor flags etc.

DetectorInfo
____________

``DetectorInfo`` can be obtained from a call to ``ExperimentInfo::detectorInfo()`` (usually this method would be called on ``MatrixWorkspace``). The wrapper class holds a reference to the parametrised instrument for retrieving the relevant information.

There is also a partial implementation of the "real" ``DetectorInfo`` class, in the ``Beamline`` namespace. The real class currently stores the masking information for a detector. The wrapper ``DetectorInfo`` class holds a reference to the real class. This does not affect the rollout, where the wrapper class should still be used in all cases.

Changes for Rollout
-------------------

Performance Tests
#################

Before starting the refactoring work please take a look at the state of any performance tests that exist for the algorithms. If they exist they should be run to get the "before" timings. If they do not exist please add performance test for any algorithms that are widely used, or might be expected to have a performance increase. See `this performance test <https://github.com/mantidproject/mantid/pull/18189/files#diff-5695221d30495359738f90b83ceb0ba3>`_ added for the ``SpectrumInfo`` rollout for an example of adding such a test.

Each PR should include the runtime metrics for the algorithms changed, so that improvements can be captured for the release notes.

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

  const auto &spectrumInfo = ws->spectrumInfo();

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

  const auto &spectrumInfo = ws->spectrumInfo();

  if (!spectrumInfo.hasUniqueDetector(wsIndex)) {
    // do exception
    return false;
  }

  // do stuff


Access to Detectors
___________________

The ``detector(wsIndex)`` method on ``SpectrumInfo`` returns the parameterised detector or detector group for the workspace. As ``SpectrumInfo`` does not provide access to things like ``Object::solidAngle()``, the ``detector()`` method on ``SpectrumInfo`` can be used to get access to these methods.

Mutable SpectrumInfo
____________________

The method ``MatrixWorkspace::mutableSpectrumInfo()`` returns a non-const ``SpectrumInfo`` object. Currently the only method that this access is required for is ``SpectrumInfo::setMasked(const size_t index, bool masked)``.

Useful Tips
___________

* Creation of ``SpectrumInfo`` objects is not cheap. Make sure ``ws.spectrumInfo()`` is called outside of loops that go over all spectra.
* If a ``SpectrumInfo`` object is required for more than one workspace then include the workspace name in the name of the ``SpectrumInfo`` object, to avoid confusion.
* Get the ``SpectrumInfo`` object as a const reference and use auto - ``const auto &spectrumInfo = ws->spectrumInfo();``.
* Do not forget to add the import - ``#include "MantidAPI/SpectrumInfo.h"``.

Complete Examples
_________________

* `FindCenterOfMassPosition2.cpp <https://github.com/mantidproject/mantid/pull/17394/files#diff-905c244467474fc320eaf3b8c7a9f0dd>`_

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

The ``DetectorInfo`` object is accessed by an index going from 0 to the number of detectors. A vector of detector IDs can be obtained from a call to ``detectorInfo.detectorIDs()``.

It is also possible to use the method ``detectorInfo.indexOf(detID)`` to get the index for a particular detector ID. However, this is a call to a lookup in an unordered map, so is an expensive calculation which should be avoided where possible.

Below is an example refactoring.

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

  const auto &detectorInfo = ws->detectorInfo();
  if (detectorInfo.size() == 0)
    throw runtime_error("There is no instrument in input workspace.")

  vector<detid_t> detIds = detectorInfo.detectorIDs();

  for (size_t i = 0; i < detectorInfo.size(); ++i) {
    if (detectorInfo.isMasked(i)) {
      maskedDetIds.push_back(detIds[i]);
    }
  }

Access to Detectors
___________________

As for the ``SpectrumInfo`` object ``DetectorInfo`` can return a parameterised detector for the workspace.

Mutable DetectorInfo
____________________

The method ``ExperimentInfo::mutableDetectorInfo()`` returns a non-const ``DetectorInfo`` object. This allows the methods below to be used.

* ``setMasked(const size_t index, bool masked);``
* ``clearMaskFlags();``
* ``setPosition(const size_t index, const Kernel::V3D &position);``
* ``setRotation(const size_t index, const Kernel::Quat &rotation);``
* ``setPosition(const Geometry::IComponent &comp, const Kernel::V3D &pos);``
* ``setRotation(const Geometry::IComponent &comp, const Kernel::Quat &rot);``

For a complete example of setting the position of a detector see the changes to the algorithm `ApplyCalibration.cpp <https://github.com/mantidproject/mantid/commit/0c75f46e85c2dc39c2b76ea811f161fdfdef938e#diff-a4ccabae0099bfc22b3b87154cd6ee9e>`_.

Useful Tips
___________

See tips for ``SpectrumInfo`` - the same advice applies to using ``DetectorInfo``.

Complete Examples
_________________

* `FindDetectorsInShape.cpp <https://github.com/mantidproject/mantid/commit/177ad14b25db7c40ee10be513512be9ae7dd0da1#diff-7f367da22c1a837b315b4bca5b2b3e24>`_

* `SmoothNeighbours.cpp <https://github.com/mantidproject/mantid/pull/18295/files#diff-26a49ef923e1bdd77677b962528d1e01>`_

* `ClaerMaskFlag.cpp <https://github.com/mantidproject/mantid/pull/18198/files#diff-7f0f739ba6db714eb6ef64b6125e4620>`_

* `ApplyCalibration.cpp <https://github.com/mantidproject/mantid/commit/0c75f46e85c2dc39c2b76ea811f161fdfdef938e#diff-a4ccabae0099bfc22b3b87154cd6ee9e>`_ - for mutable ``DetectorInfo``

Rollout status
--------------

See ticket `17743 <https://github.com/mantidproject/mantid/issues/17743>`_ for an overview of the ``SpectrumInfo`` and ``DetectorInfo`` rollout, including completed and algorithms, and remaining algorithms. Please follow the instructions on the ticket for the rollout.

Dealing with problems
---------------------

Join #instrument-2_0 on Slack if you need help or have questions. Please also feel free to get in touch with Ian Bush, Simon Heybrock or Owen Arnold directly for any questions about the ``SpectrumInfo``, ``DetectorInfo`` and the rollout.


.. categories:: Concepts
