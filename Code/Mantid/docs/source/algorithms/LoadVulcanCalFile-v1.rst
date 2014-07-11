.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm loads a set of VULCAN's calibration files, including
detector offset file and bad pixel file, and convert VULCAN's offset
on time-of-flight to Mantid's offset on d-spacing.

By this algorithm, Vulcan's calibration file can be converted to
the standard calibration file for SNSPowderReduction.

Detector offset file
====================

There are :math:`62500` (:math:`50\times 1250`) rows in the offset file.
In each row, the first value is the pixel ID; and the second is inner-module offset.

For each module, it starts from row :math:`1250\times M_{i}`, where :math:`M_{i}` is the module ID starting
from 0.

- Line :math:`1250\times M_i + 0`: pixel ID, offset (first detector in module)
- Line :math:`1250\times M_i + 1`: pixel ID, offset
- ... ...
- Line :math:`1250\times M_i + 1231`: pixle ID, offset (last detector in module)
- Line :math:`1250\times M_i + 1232`: pixel ID, offset (detector is not used)
- ... ...
- Line :math:`1250\times M_i + 1248`: pixel ID, inter module correction
- Line :math:`1250\times M_i + 1249`: pixel ID, inter bank correction

Bad pixel file
==============

In bad pixel file, each line contains one and only one integer
corresponding to the detector ID of a bad pixel.
The bad pixels will be masked in the output MaskWorkspace.

Conversion from offset in TOF to d-spacing
==========================================

With VULCAN's offsets in TOF, the calibration is done as the following.

- Total offset is the product of innter-bank offset, inner-module offset and inter-module offset

.. math:: \log_{10}(\xi) = \log_{10}(\xi_0) + \log_{10}(\xi_1) + \log_{10}(\xi_2)

- Time-of-flight value of each neutron is obtained by division of total offset.

.. math:: \log_{10}(T')  = \log_{10}(T) - \log_{10}(\xi)

.. math:: T' = \frac{T}{\xi}`

where (1) :math:`\xi_0` is the inner-bank correction, (2) :math:`\xi_1` is the inner module (inter-bank) correction,
and (3) :math:`\xi_2` is the inter-module (inner-pack) correction.

Be noticed that the correction factor recorded in VULCAN's offset file is :math:`\log(\xi)`.
Thus if we define :math:`\Xi = \log(\xi)`, then time focussing formula used by VUCLAN's IDL code
is :math:`T^{(f)} = \frac{T}{10^{\Xi}}`

Therefore, by defining :math:`\xi^{(v)}` as the VULCAN's offset, and :math:`\xi^{(m)}` as
the Mantid's offset, then we can convert VULCAN's offset to Mantid's as

.. math:: \xi^{(m)} = \frac{L\cdot\sin\theta}{L'\cdot\sin\theta'}\cdot\frac{1}{\xi^{(m)}} - 1

VULCAN uses effective DIFC and :math:`2\theta` for the effective detector to be focussed on.
It follows the Bragg rule for time-of-flight, i.e.,

.. math:: T = DIFC \cdot d = (252.777\cdot L\cdot 2\cdot\sin\theta)\cdot d


Usage
-----

.. testcode:: LoadVulcanCalFile

  # Load Vulcan calibration files
  LoadVulcanCalFile(OffsetFilename='pid_offset_vulcan_new.dat',
    BadPixelFilename='bad_pids_vulcan_new_6867_7323.dat',
    WorkspaceName='Vulcan_idl', BankIDs='21,22,23,26,27,28',
    EffectiveDIFCs='16372.6,16377,16372.1,16336.6,16340.8,16338.8',
    Effective2Thetas='90.091,90.122,90.089,89.837,89.867,89.852')

  # Print
  offsetws = mtd["Vulcan_idl_offsets"]
  groupws = mtd["Vulcan_idl_group"]
  for iws in [0, 100, 1000, 3500, 7000]:
    print "Spectrum %-5d Offset = %.5f of Group %d" % (iws, offsetws.readY(iws)[0], groupws.readY(iws)[0])
  maskws = mtd["Vulcan_idl_mask"]
  print "Size of mask workspace = %d" % (maskws.getNumberHistograms())

.. testcleanup::

Output:

.. testoutput:: LoadVulcanCalFile

  Spectrum 0     Offset = -0.00047 of Group 1
  Spectrum 100   Offset = -0.00096 of Group 1
  Spectrum 1000  Offset = -0.00060 of Group 1
  Spectrum 3500  Offset = -0.00036 of Group 3
  Spectrum 7000  Offset = 0.00058 of Group 6
  Size of mask workspace = 7392

.. categories::
