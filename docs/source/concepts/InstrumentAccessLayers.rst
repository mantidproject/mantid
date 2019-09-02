.. _InstrumentAccessLayers:

================================================================
Instrument Access via SpectrumInfo, DetectorInfo, ComponentInfo
================================================================

.. contents::
  :local:

Introduction
------------

There are three layers to access instrument information, :py:obj:`~mantid.api.SpectrumInfo`, :py:obj:`~mantid.geometry.DetectorInfo`, and :py:obj:`~mantid.geometry.DetectorInfo`, which are introduced to Mantid as part of Instrument 2.0. These classes  store all commonly accessed information about spectra and detectors, components, and the relationships between them. Masking, monitor flags, L1, L2, 2-theta and position are stored as part of :py:obj:`~mantid.geometry.DetectorInfo`. In addition, :py:obj:`~mantid.geometry.ComponentInfo` provides the API to tree and shape related operations historically performed by :ref:`Instrument` type.

A spectrum corresponds to (a group of) one or more detectors. Most algorithms work with spectra and thus :py:obj:`~mantid.api.SpectrumInfo` would be used. Some algorithms work on a lower level (with individual detectors) and thus :py:obj:`~mantid.geometry.DetectorInfo` would be used.

The legacy :ref:`Instrument` largely consists of ``Detectors`` and ``Components`` - all detectors are also components. :py:obj:`~mantid.geometry.DetectorInfo` and :py:obj:`~mantid.geometry.ComponentInfo` are the respective replacements for these. :py:obj:`~mantid.geometry.ComponentInfo` introduces a **component index** for access, and :py:obj:`~mantid.geometry:DetectorInfo` introduces a **detector index**, these will be discussed further below. :py:obj:`~mantid.geometry.DetectorInfo` and :py:obj:`~mantid.geometry.ComponentInfo` share in-memory data. The difference between the two is best thought about in terms of their interfaces. The interface for :py:obj:`~mantid.geometry.DetectorInfo` is designed for working with detectors, and the interface for :py:obj:`~mantid.geometry.ComponentInfo` is designed for working with generic components.

In many cases direct access to legacy :ref:`Instrument` can be removed by using these layers. This will also help in moving to using indexes for enumeration, and only working with IDs for user-facing input.

Current Status
##############

``SpectrumInfo``, ``DetectorInfo`` and ``ComponentInfo``  are largely complete, with a diminishing number of cases where any legacy direct ``Instrument`` access is still necessary. However, using the new interfaces everywhere now will help with the eventual complete rollout of Instrument 2.0.

SpectrumInfo
____________

``SpectrumInfo`` can be obtained from a call to :py:obj:`mantid.api.MatrixWorkspace.spectrumInfo()`. The wrapper class holds a reference to a ``DetectorInfo`` object and calls through to this for access to information on masking, monitor flags etc.

DetectorInfo
____________

``DetectorInfo`` can be obtained from a call to :py:obj:`mantid.api.ExperimentInfo.detectorInfo()` (usually this method would be called on ``MatrixWorkspace``). The wrapper class holds a reference to the parametrised instrument for retrieving the relevant information.

There is also a near-complete implementation of the "real" ``DetectorInfo`` class, in the ``Beamline`` namespace. The wrapper ``DetectorInfo`` class (which you get from :py:obj:`~mantid.api.ExperimentInfo.detectorInfo()`) holds a reference to the real class. This does not affect the rollout, where the wrapper class should still be used in all cases.

``ExperimentInfo`` now also provides a method ``mutableDetectorInfo()`` so that non-const access to the DetectorInfo is possible for purposes of writing detector related information such as positions or rotations.

The python interface to ``DetectorInfo`` has matured, and includes widespread immutable access via iterators. The iterators can also be used to set masking flags.

See :ref:`DetectorInfo <DetectorInfo>` for more information.

ComponentInfo
______________
``ComponentInfo`` can be obtatined from a call to ``ExperimentInfo::componentInfo()`` (usually this method would be called on ``MatrixWorkspace``). Much like ``DetectorInfo``, the ``ComponentInfo`` yielded from this method call is a wrapper, which contains shape and index information, that cannot yet be moved in to the real ``Beamline::ComponentInfo``. However, replacing existing usage of ``IComponent`` and ``IObjComponent`` wherever possible with ``ComponentInfo`` across the framework will represent a major step forwards.

For writing to the component tree. You can extract a non-const ``ComponentInfo`` via ``ExperimentInfo::mutableComponentInfo``.

The python interface to ``ComponentInfo`` has matured, and now provides equal, if not better support than the ``Instrument`` API for navigating the high-level instrument. Iterator support has also been provided for ``ComponentInfo``.

See :ref:`ComponentInfo <ComponentInfo>` for more information.

Changes for Rollout
-------------------

Performance Tests
#################

Before starting the refactoring work please take a look at the state of any performance tests that exist for the algorithms. If they exist they should be run to get the "before" timings. If they do not exist please add performance test for any algorithms that are widely used, or might be expected to have a performance increase. See `this performance test <https://github.com/mantidproject/mantid/pull/18189/files#diff-5695221d30495359738f90b83ceb0ba3>`_ added for the previous ``SpectrumInfo`` rollout phase for an example of adding such a test.

Each PR should include the runtime metrics for the algorithms changed, so that improvements can be captured for the release notes.

ComponentInfo
#############

Basics
______

The conversion is similar to that for ``DetectorInfo``, which is already largely complete in the framework. For ``ComponentInfo`` all instances of ``Instrument::getComponentByID(const ComponentID id)`` should be replaced using calls to the ``ComponentInfo`` object obtained from ``MatrixWorkspace::componentInfo()``. The methods below can then be called on ``ComponentInfo`` instead of on the component.

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

The ``ComponentInfo`` object is accessed by an index going from 0 to the number of components (including the instrument iteself). **The component index for a detector is EQUAL to the detector index**, this is an important point to understand. In other words, a detector with a Detector Index of 5, for the purposes of working with a ``DetectorInfo`` and  will have a Component Index of 5, when working with a ``ComponentInfo``. Explained in yet another way: The first 0 - n components referenced in the ``ComponentInfo`` are detectors, where n is the total number of detectors. This guarantee can be leveraged to provide speedups, as some of the examples will show.

A ``ComponentID`` for compatibility with older code, and be extracted from ``ComponentInfo::componentID(componentIndex)``, but such calls should be avoided where possible.

It is also possible to use the method ``componentInfo.indexOf(componentID)`` to get the index for a particular component ID. However, this is a call to a lookup in an unordered map, so is an expensive calculation which should be avoided where possible.

**One should NEVER expose a Component Index or Detector Index through a user facing interface, such an algorithm or fit function.**. Detector Index and Component Indexes are internal concepts for fast enumeration. It is however desirable to translate from a ``ComponentIndex`` via ``ComponentInfo::indexOf`` to as early as possible and in such a way to avoid repeated calls to this method, as stated above. Likewise, conversion back to a ``ComponentIndex``, if so required, should be done as infrequently and, as late as possible.

Below is an example refactoring.

**Before refactoring**

.. code-block:: c++

  auto instrument = ws->getInstrument();
  std::vector<IComponent_const_sptr> children;
  instrument->getChildren(children, true /*Get all sub-children too*/);
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
  for (size_t i = 0; i < componentInfo.size(); ++i) {
    componentInfo.solidAngle(i, observer);
  }

Access to Components and working with Detectors
_______________________________________________

Detector Indices are the same as the corresponding Component Indices. Note that there are no dynamic casts. The following examples are for illustration purposes only.

**Combining DetectorInfo and ComponentInfo**

.. code-block:: c++

  #include "MantidGeometry/Instrument/ComponentInfo.h"
  #include "MantidGeometry/Instrument/DetectorInfo.h"

  ...

  const auto &componentInfo = ws->componentInfo();
  const auto &detectorInfo = ws->componentInfo();

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

  size_t bank0Index; // Component index for bank 0
  ...

  const auto &componentInfo = ws->componentInfo();
  auto bankComponents = componentInfo.componentsInSubtree(bank0Index);
  auto bankDetectors = componentInfo.detectorsInSubtree(bank0Index);

Mutable ComponentInfo
_____________________

The method ``ExperimentInfo::mutableComponentInfo()`` returns a non-const ``ComponentInfo`` object. This allows the methods below to be used.

* ``setPosition(const size_t index, const Kernel::V3D &position);``
* ``setRotation(const size_t index, const Kernel::Quat &rotation);``
* ``setScaleFactor(const size_t index, const Kernel::V3D &scaleFactor);``

Useful Tips
___________

* Creation of ``ComponentInfo`` is not cheap enough to perform uncessarily inside loops. For const access, ``ws.componentInfo()`` should be called outside of loops that enumerate over all components.
* If a ``ComponentInfo`` object is required for more than one workspace, include the workspace name in the variable name to avoid confusion.
* Get the ``ComponentInfo`` object as a const-ref and use ``const auto &componentInfo = ws->componentInfo();``, do not get a non-const reference unless you really do need to modify the object, and ensure that the ``&`` is always included to prevent accidental copies.
* ``ComponentInfo`` is widely forward declared. Ensure that you import - ``#include "MantidGeometry/Instrument/ComponentInfo.h"``
* As explained above, a detector index is the same thing as a component index. No translation necessary. The fact that the first 0-n component indexes are for detectors is a feature that can be leveraged.
* A bank always has a higher component index than any of its nested components. The root is the highest component index of all. This feature can be leveraged. Consider reverse iterating through component indexes when performing operations that involve higher-level components.

Dealing with problems
---------------------

Join #instrument-2_0 on Slack if you need help or have questions. Please also feel free to get in touch with Owen Arnold, Simon Heybrock or Lamar Moore directly for any questions about the ``ComponentInfo`` rollout.


.. categories:: Concepts
