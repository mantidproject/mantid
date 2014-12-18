.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Converts a 2D workspace from :ref:`units <Unit Factory>` 
of spectrum number/**energy transfer** 
to the intensity as a function of **momentum transfer** :math:`Q` 
and **energy transfer** :math:`\Delta E`. The rebinning is done as a 
weighted sum of overlapping polygons with
fractional area tracking. The result is stored in a new workspace type:
**RebinnedOutput**. The new workspace presents the data as the
fractional counts divided by the fractional area. The biggest
consequence of this method is that in places where there are no counts
and no acceptance (no fractional areas), **nan**\ -s will result.

The algorithm operates in non-PSD mode by default. This means that all
azimuthal angles and widths are forced to zero. PSD mode will determine
the azimuthal angles and widths from the instrument geometry. This mode
is activated by placing the following named parameter in a Parameter
file: *detector-neighbour-offset*. The integer value of this parameter
should be the number of pixels that separates two pixels at the same
vertical position in adjacent tubes.


See  :ref:`algm-SofQW` for centerpoint binning  or :ref:`algm-SofQW2`  
for simpler and less precise but faster binning strategies.

Usage
-----

**Example - simple transformation of inelastic workspace:**

.. testcode:: SofQW3

   # create sample inelastic workspace for MARI instrument containing 1 at all spectra 
   ws=CreateSimulationWorkspace(Instrument='MAR',BinParams='-10,1,10')
   # convert workspace into Matrix workspace with Q-dE coordinates 
   ws=SofQW3(InputWorkspace=ws,QAxisBinning='-3,0.1,3',Emode='Direct',EFixed=12)
  
   print "The converted X-Y values are:"
   Xrow=ws.readX(59);
   Yrow=ws.readY(59);   
   for i in xrange(0,20):
    print '! {0:>6.2f} {1:>6.2f} '.format(Xrow[i],Yrow[i]),
    if (i+1)%10 == 0:
        print '!\n',
   print '! {0:>6.2f} ------- !'.format(Xrow[20]),



.. testcleanup:: SofQW3

   DeleteWorkspace(ws)
   
**Output:**

.. testoutput:: SofQW3

   The converted X-Y values are:
   ! -10.00   1.00  !  -9.00   1.00  !  -8.00   1.00  !  -7.00   1.00  !  -6.00   1.00  !  -5.00   1.00  !  -4.00   1.00  !  -3.00   1.00  !  -2.00   1.00  !  -1.00   1.00  !
   !   0.00   1.00  !   1.00   1.00  !   2.00   1.00  !   3.00   1.00  !   4.00   1.00  !   5.00   1.00  !   6.00   1.00  !   7.00   1.00  !   8.00   1.00  !   9.00   1.00  !
   !  10.00 ------- !


.. categories::
