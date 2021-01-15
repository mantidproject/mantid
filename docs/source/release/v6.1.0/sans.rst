============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

      
Algorithms and instruments
--------------------------

Improvements
############

 - In :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>`, there can be multiple inputs for sensitivity calculation.
   A new parameter, SensitivityWithOffsets, has been added to mark that these multiple sensitivities should be processed
   separately and specially merged before calculation of the efficiency to remove gaps caused by beam stop mask.
 - In :ref:`SANSILLReduction <algm-SANSILLReduction>`, a new parameter, StoreOnlyEfficiencyInput, is used to flag that
   efficiency input should be process outside of the :ref:`SANSILLReduction <algm-SANSILLReduction>` algorithm

Bugfixes
########


:ref:`Release 6.1.0 <v6.1.0>`
