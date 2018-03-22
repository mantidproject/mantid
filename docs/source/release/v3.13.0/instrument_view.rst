========================
Instrument Visualization
========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Instrument View
---------------

The Instrument View visualization tool in Mantid has undergone some major changes under the hood which has resulted in a smoother, more responsive interface. 
Instruments generally load faster as well, however, below are a few noteworthy improvements to load times: 

+------------+-----------+
| Instrument | Speedup   |
+============+===========+
| WISH       | 5x        |
+------------+-----------+
| BASIS      | 5x        |
+------------+-----------+
| GEM        | 4x        |
+------------+-----------+
| SANS2D     | 3x        |
+------------+-----------+
| POLARIS    | 3x        |
+------------+-----------+
| CNCS       | 2x        |
+------------+-----------+

:ref:`Release 3.13.0 <v3.13.0>`