=======================
Direct Geometry Changes
=======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Direct Geometry
---------------

New Algorithms
##############

:ref:`DirectILLAutoProcess <algm-DirectILLAutoProcess>` performs full data reduction treatment for ILL direct geometry instruments for empty container, vanadium, and sample, both single crystal and powder.

Improvements
############

 :ref:`DirectILLCollectData <algm-DirectILLCollectData>` has two new properties: `GroupDetHorizontallyBy` and `GroupDetVerticallyBy` which allow for averaging pixel counts between the tubes and inside them, respectively, of for flat background calculation

:ref:`Release 6.4.0 <v6.4.0>`
