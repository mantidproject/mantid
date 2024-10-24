
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Measurements using polarized neutrons can be used to identify magnetic and nuclear contribution.
In the case of coherent scattering, nuclear spins generally do not change the spin polarization
of the neutrons (non spin flip scattering).
For magnetic scattering, the magnetization parallel to the neutron spin direction produces
non spin flip scattering, while magnetization perpendicular to the spin direction will flip
the neutron spin.

When measuring spin flip and non spin flip cross sections, the finite capabilities of the instrument
will allow a certain percentage of the other component to leak through. This is denoted by the
flipping ratio :math:`F`. Using subscript "m" for measured and "c" for corrected, the corrected
intensities for the spin flip (SF) and non spin flip (NSF) scattering are given by [1]_:

.. math::

    I_{NSFc}=\frac{F}{F-1}I_{NSFm}-\frac{1}{F-1}I_{SFm}\\
    I_{SFc}=\frac{F}{F-1}I_{SFm}-\frac{1}{F-1}I_{NSFm}


Given a multidimensional event workspace, this algorithm will output the workspace where events
are multiplied by  :math:`F/(F-1)` (in ``OutputWorkspace1``) and by :math:`1/(F-1)` (in ``OutputWorkspace2``).
Note however that the flipping ratio might be angle dependent. For example, in the case of
type II superconductors, the flux lattice will depolarize the neutrons differently, depending
on the orientation of the neutron beam with respect to the superconducting planes. In this case
one must use a formula that would use a different flipping ratio for each neutron, that
depends on a goniometer angle. Assuming that we have a log value ``omega`` for the sample rotation,
the flipping ratio might be described by something like [2]_:

.. math::

    F=6.5+2.8\cos(\pi(omega+3.7)/180)

The MD event workspaces label each MDEvent with the experiment info number. The time averaged log
value for ``omega`` is read from each experiment info, the flipping ratio is calculated from the
user provided formula, and the corrected workspaces are computed.

For the parsing of the formula, the algorithm uses `muparser <http://beltoforion.de/article.php?a=muparser>`_.
Once can use any of the implemented functions. The constant "pi" can be used in the formula. Use "_e" for the
base of natural logarithms.


References
----------

.. [1] O Schärpf, *Experience with spin analysis on a time-of-flight multidetectorscatteringinstrument*, In Neutron scattering in the nineties, Proceedings of the IAEA Conference, Jülich 85-97 (1985) `https://inis.iaea.org/collection/NCLCollectionStore/_Public/16/076/16076003.pdf <https://inis.iaea.org/collection/NCLCollectionStore/_Public/16/076/16076003.pdf>`__

.. [2] Igor A Zaliznyak et al, *Polarized neutron scattering on HYSPEC: the HYbrid SPECtrometer at SNS*, J. Phys.: Conf. Ser. 862 012030 (2017) `doi:10.1088/1742-6596/862/1/012030 <https://doi.org/10.1088/1742-6596/862/1/012030>`__


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - FlippingRatioCorrectionMD**

.. testcode:: FlippingRatioCorrectionMDExample

  # Fake an md workspace
  CreateMDWorkspace(Dimensions=2, EventType='MDEvent', Extents='-5,5,-5,5', Names='A,B', Units='a,a' ,OutputWorkspace='mde1')
  AddSampleLog(Workspace='mde1', LogName='param', LogText='0.', LogType='Number', NumberType='Double')
  FakeMDEventData(InputWorkspace='mde1', PeakParams='100,-2,-2,0.1')
  CreateMDWorkspace(Dimensions=2, EventType='MDEvent', Extents='-5,5,-5,5', Names='A,B', Units='a,a',OutputWorkspace='mde2')
  AddSampleLog(Workspace='mde2', LogName='param', LogText='90.', LogType='Number', NumberType='Double')
  FakeMDEventData(InputWorkspace='mde2', PeakParams='100,2,2,0.1')
  MergeMD(InputWorkspaces='mde1,mde2', OutputWorkspace='merged')

  # Calculate flipping ratio corrections
  FlippingRatioCorrectionMD(InputWorkspace='merged',
                            FlippingRatio='10+5*sin(param*pi/180)',
                            SampleLogs='param',
                            OutputWorkspace1='correctionFoFm1',
                            OutputWorkspace2='correction1oFm1')

  # Bin data and extract intensities
  BinMD(InputWorkspace='correctionFoFm1', AlignedDim0='A,-5,5,2', AlignedDim1='B,-5,5,2', OutputWorkspace='correctionFoFm1_histo')
  peak1_FoFm1=mtd['correctionFoFm1_histo'].getSignalArray()[0,0]
  peak2_FoFm1=mtd['correctionFoFm1_histo'].getSignalArray()[1,1]
  BinMD(InputWorkspace='correction1oFm1', AlignedDim0='A,-5,5,2', AlignedDim1='B,-5,5,2', OutputWorkspace='correction1oFm1_histo')
  peak1_1oFm1=mtd['correction1oFm1_histo'].getSignalArray()[0,0]
  peak2_1oFm1=mtd['correction1oFm1_histo'].getSignalArray()[1,1]
  print("{:>17}   Peak1      Peak2".format(' '))
  print("{:>17} {:8.3f}   {:8.3f}".format('Original',100,100))
  print("{:>17} {:8.3f}   {:8.3f}".format('F',10,15))
  print("{:>17} {:8.3f}   {:8.3f}".format('F/(F-1)',10./9.,15/14.))
  print("{:>17} {:8.3f}   {:8.3f}".format('Corrected F/(F-1)',peak1_FoFm1,peak2_FoFm1))
  print("{:>17} {:8.3f}   {:8.3f}".format('1/(F-1)',1./9.,1/14.))
  print("{:>17} {:8.3f}   {:8.3f}".format('Corrected 1/(F-1)',peak1_1oFm1,peak2_1oFm1))

Output:

.. testoutput:: FlippingRatioCorrectionMDExample

                          Peak1      Peak2
               Original  100.000    100.000
                      F   10.000     15.000
                F/(F-1)    1.111      1.071
      Corrected F/(F-1)  111.111    107.143
                1/(F-1)    0.111      0.071
      Corrected 1/(F-1)   11.111      7.143

.. categories::

.. sourcelink::
