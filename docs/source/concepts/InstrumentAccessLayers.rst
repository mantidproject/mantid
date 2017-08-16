.. _InstrumentAccessLayers:

===================================================
Instrument Access via SpectrumInfo and DetectorInfo
===================================================

.. contents::
  :local:

Introduction
------------

There are three new layers to access instrument information, ``SpectrumInfo``, ``DetectorInfo``, and ``ComponentInfo`` that are introduced to Mantid as part of the work towards Instrument 2.0. Eventually these classes will store all commonly accessed information about spectra and detectors, namely masking, monitor flags, L1, L2, 2-theta and position. In addition, ``ComponentInfo`` will provide the updated API to tree and shape related operations currently peformed by the instrument. These changes are leading to improved performance and cleaner code.

A spectrum corresponds to (a group of) one or more detectors. Most algorithms work with spectra and thus ``SpectrumInfo`` would be used. Some algorithms work on a lower level (with individual detectors) and thus ``DetectorInfo`` would be used.

The current instrument largely consists of ``Detectors`` and ``Components`` - all detectors are also components. ``DetectorInfo`` and ``ComponentInfo`` are the respective replacements for these. ``ComponentInfo`` introduces a new **component index** for access, and ``DetectorInfo`` introduces a new **detector index**, these will be discussed further below. 

In many cases direct access to ``Instrument`` can be removed by using these layers. This will also help in moving to using indexes for enumeration, and only working with IDs for user-facing input.

Current Status
##############

``SpectrumInfo``, ``DetectorInfo`` and ``ComponentInfo``  are currently implemented in a compatibility mode, and wrap some of the exiting instrument functionality in Mantid. However, using the new interfaces everywhere now will help with the eventual rollout of Instrument 2.0. It also provides cleaner code and will help with the rollout of planned indexing changes. It is also necessary to provide the addition of time indexing, required to support scanning instruments.

SpectrumInfo
____________

``SpectrumInfo`` can be obtained from a call to ``MatrixWorkspace::spectrumInfo()``. The wrapper class holds a reference to a ``DetectorInfo`` object and calls through to this for access to information on masking, monitor flags etc.

DetectorInfo
____________

``DetectorInfo`` can be obtained from a call to ``ExperimentInfo::detectorInfo()`` (usually this method would be called on ``MatrixWorkspace``). The wrapper class holds a reference to the parametrised instrument for retrieving the relevant information.

There is also a near-complete implementation of the "real" ``DetectorInfo`` class, in the ``Beamline`` namespace. The wrapper ``DetectorInfo`` class (which you get from ``ExperimentInfo::detectorInfo()`` holds a reference to the real class. This does not affect the rollout, where the wrapper class should still be used in all cases.

``ExperimentInfo`` now also provides a method ``mutableDetectorInfo()`` so that non-const access to the DetectorInfo is possible for purposes of writing detector related information such as positions or rotations. These should be used wherever possible, but note, much like the existing instrument, writes are not guranteed to be thread safe.

ComponentInfo
____________
``ComponentInfo`` can be obtatined from a call to ``ExperimentInfo::componentInfo()`` (usually this method would be called on ``MatrixWorkspace``). Much like ``DetectorInfo``, the ``ComponentInfo`` yielded from this method call is a wrapper, which contains shape and index information, that cannot yet be moved in to the real ``Beamline::ComponentInfo``. However, replacing existing usage of ``IComponent`` and ``IObjComponent`` wherever possible with ``ComponentInfo`` across the framework will represent a major step forwards.

For writing to the component tree. You can extract a non-const ``ComponentInfo`` via ``ExperimentInfo::mutableComponentInfo``.

Changes for Rollout
-------------------

Performance Tests
#################

Before starting the refactoring work please take a look at the state of any performance tests that exist for the algorithms. If they exist they should be run to get the "before" timings. If they do not exist please add performance test for any algorithms that are widely used, or might be expected to have a performance increase. See `this performance test <https://github.com/mantidproject/mantid/pull/18189/files#diff-5695221d30495359738f90b83ceb0ba3>`_ added for the previous ``SpectrumInfo`` rollout phase for an example of adding such a test.

Each PR should include the runtime metrics for the algorithms changed, so that improvements can be captured for the release notes.

ComponentInfo
############

Basics
______

The conversion is similar to that for ``DetectorInfo``, which is already largeely complete in the framework. For ``ComponentInfo`` all instances of ``Instrument::getComponentByID(const ComponentID id)`` should be replaced using calls to the ``ComponentInfo`` object obtained from ``MatrixWorkspace::componentInfo()``. The methods below can then be called on ``ComponentInfo`` instead of on the component.

* ``isDetector(componentIndex)``
* ``detectorsInSubtree(componentIndex)``
* ``componentsInSubtree(componentIndex)``
* ``position(componentIndex)``
* ``rotation(componentIndex)``
* ``hasParent(componentIndex)``
* ``parent(componentIndex)``
* ``position(componentIndex)``
* ``solidAngle(componentIndex)``
* ``scaleFactor(componentIndex)``
* ``sourcePosition()``
* ``samplePosition()``
* ``l1()``

The following methods are useful helpers on ``ComponentInfo`` that allow the extraction of the **component index** for key components

* ``root()``
* ``source()``
* ``sample()``

**Indexing**

The ``ComponentInfo`` object is accessed by an index going from 0 to the number of components (including the instrument iteself). **ALL Detector Indexes have a Component Index which is the EXACT same representation**, this is an important point to understand. In other words, a detector with a Detector Index of 5, for the purposes of working with a ``DetectorInfo`` and  will have a Component Index of 5, when working with a ``ComponentInfo``. Explained in yet another way: The first 0 - n components referenced in the ``ComponentInfo`` are detectors, where n is the total number of detectors. This gurantee can be leveraged to provide speedups, as some of the examples will show.  

 A ``ComponentID`` for compatiblity with older code, and be extracted from ``ComponentInfo::componentID(componentIndex)``, but such calls should be avoided where possible.

It is also possible to use the method ``componentInfo.indexOf(componentID)`` to get the index for a particular component ID. However, this is a call to a lookup in an unordered map, so is an expensive calculation which should be avoided where possible.

**One should NEVER expose a Component Index or Detector Index through a public facing (python, gui, ...) to an end user**. Detector Index and Component Indexes are internal concepts for fast enumeration. It is however desirable to translate from a ``ComponentIndex`` via ``ComponentInfo::indexOf`` to as early as possible and in such a way to avoid repeated calls to this method, as stated above. Likewise, conversion back to a ``ComponentIndex``, if so required, should be done as infrequently and, as late as possible.

Below is an example refactoring.

**Before refactoring**

.. code-block:: c++

  auto instrument = ws->getInstrument();
  if (!instrument)
    throw runtime_error("There is no instrument in input workspace.")

  std::vector<IComponent_const_sptr> children;
  instrument->getChildren(children, true);
  std::vector<IComponent_const_sptr>::const_iterator it;
  for (it = children.begin(); it != children.end(); ++it) {
    if (const ObjComponent* obj = dynamic_cast<const ObjComponent*>(it->get())) {
      // Do something with the obj component
      obj.solidAngle(observer);
    }
  }

**After - looping over index**

.. code-block:: c++

  #include "MantidGeometry/Instrument/ComponentInfo.h"

  ...

  const auto &componentInfo = ws->componentInfo();
  if (componentInfo.size() == 0)
    throw runtime_error("There is no instrument in input workspace.")

  for (size_t i = 0; i < componentInfo.size(); ++i) {
    componentInfo.solidAngle(i, observer);
  }

Access to Components and working with Detectors
___________________

Detector Indices are the same as the corresponding Component Indices. Note that there are no dynamic casts.

**Combining DetectorInfo and ComponentInfo**

.. code-block:: c++

  #include "MantidGeometry/Instrument/ComponentInfo.h"
  #include "MantidGeometry/Instrument/DetectorInfo.h"

  ...

  const auto &componentInfo = ws->componentInfo();
  const auto &detectorInfo = ws->componentInfo();
  if (componentInfo.size() == 0)
    throw runtime_error("There is no instrument in input workspace.")
  
  std::vector<double> solidAnglesForDetectors(detectorInfo.size(), -1.0);
  for (size_t i = 0; i < componentInfo.size(); ++i) {
    if(componentInfo.isDetector(i) && !detectorInfo.isMasked(i)) 
     solidAnglesForDetectors[i] = componentInfo.solidAngle(i, observer);
    }
  }

``ComponentInfo`` can give quick access to parent and sub-tree component and detector indices.

.. code-block:: c++

  #include "MantidGeometry/Instrument/ComponentInfo.h"
  #include "MantidGeometry/Instrument/DetectorInfo.h"

  ...

  const auto &componentInfo = ws->componentInfo();
  const auto &detectorInfo = ws->componentInfo();
  if (componentInfo.size() == 0)
    throw runtime_error("There is no instrument in input workspace.")
  
  std::set<size_t> componentsHoldingOnlyDetectors;
  for (size_t i = 0; i < componentInfo.size(); ++i) {
    if(componentInfo.componentsInSubtree(i) == componentInfo.detectorsInSubtree(i)) {
     componentsHoldingOnlyDetectors.insert(i); 
    }
  }

Mutable ComponentInfo
____________________

The method ``ExperimentInfo::mutableComponentInfo()`` returns a non-const ``ComponentInfo`` object. This allows the methods below to be used.

* ``setPosition(const size_t index, const Kernel::V3D &position);``
* ``setRotation(const size_t index, const Kernel::Quat &rotation);``
* ``setScaleFactor(const size_t index, const Kernel::V3D &scaleFactor);``

Useful Tips
___________

* Creation of ``ComponentInfo`` is not cheap enough to perform uncessarily inside loops. For non-const access, ``ws.componentInfo()`` should be called outside of loops that enumerate over all components.
* If a ``ComponentInfo`` object is required for more than one workspace, include the workspace name in the variable name to avoid confusion.
* Get the ``ComponentInfo`` object as a const-ref and use ``const auto &componentInfo = ws->componentInfo();``, do not get a non-const reference unless you really do need to modify the object, and ensure that the ``&`` is always included to prevent accidental copies.
* ``ComponentInfo`` is widely forward declared. Ensure that you import - ``#include "MantidGeometry/Instrument/ComponentInfo.h"``

Complete Examples
_________________

* `FindDetectorsInShape.cpp <https://github.com/mantidproject/mantid/commit/177ad14b25db7c40ee10be513512be9ae7dd0da1#diff-7f367da22c1a837b315b4bca5b2b3e24>`_

* `SmoothNeighbours.cpp <https://github.com/mantidproject/mantid/pull/18295/files#diff-26a49ef923e1bdd77677b962528d1e01>`_

* `ClaerMaskFlag.cpp <https://github.com/mantidproject/mantid/pull/18198/files#diff-7f0f739ba6db714eb6ef64b6125e4620>`_

* `ApplyCalibration.cpp <https://github.com/mantidproject/mantid/commit/0c75f46e85c2dc39c2b76ea811f161fdfdef938e#diff-a4ccabae0099bfc22b3b87154cd6ee9e>`_ - for mutable ``DetectorInfo``

Rollout status
--------------

TODO

Dealing with problems
---------------------

Join #instrument-2_0 on Slack if you need help or have questions. Please also feel free to get in touch with Owen Arnold, Simon Heybrock or Lamar Moore directly for any questions about the ``ComponentInfo`` rollout.


.. categories:: Concepts
