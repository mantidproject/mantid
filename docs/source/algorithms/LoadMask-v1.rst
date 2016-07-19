.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to load a masking file, which can be in XML
format (defined later in this page) or old-styled calibration file.

Definition of Mask
------------------

* If a pixel is masked, it means that the data from this pixel won't be used. In the masking workspace (i.e., `SpecialWorkspace2D <http://www.mantidproject.org/SpecialWorkspace2D>`_ ), the corresponding value is 1.
* If a pixel is NOT masked, it means that the data from this pixel will be used. In the masking workspace (i.e., `SpecialWorkspace2D <http://www.mantidproject.org/SpecialWorkspace2D>`_ ), the corresponding value is 0.

File Format
-----------

XML File Format
###############

Example 1::

 <?xml version="1.0" encoding="UTF-8" ?>
 <detector-masking>
  <group>
   <detids>3,34-44,47</detids>
   <component>bank123</component>
   <component>bank124</component>
  </group>
 </detector-masking>

ISIS File Format
################

Example 2::

 1-3 62-64
 65-67 126-128
 129-131 190-192
 193-195 254-256
 257-259 318-320
 321-323 382-384
 385 387 446 448
 ... ...

All the integers in file of this format are spectrum Numbers to mask. Two
spectrum Numbers with "-" in between indicate a continuous range of spectra
to mask. It does not matter if there is any space between integer number
and "-". There is no restriction on how the line is structured. Be
noticed that any line starting with a non-digit character, except space,
will be treated as a comment line.

This algorithm loads masking file to a SpecialWorkspace2D/MaskWorkspace.

Supporting ::

 * Component ID --> Detector IDs --> Workspace Indexes
 * Detector ID --> Workspace Indexes
 * Spectrum Number --> Workspace Indexes
 

When spectra mask is used on multiple workspaces, the same masking is produced only
if all masked workspaces have the same spectra-detector map.
When mask is generated for one workspace and applied to workspace with different 
spectra-detector mapping, the same masking can be produced by using *Workspace*
option, using this workspace as the source of the spectra-detector mapping.


Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: DoIt

    ws = Load('HYS_11092_event.nxs')
    mask = LoadMask('HYS', 'HYS_mask.xml')
    # To check if mask loaded, apply it
    MaskDetectors(ws, MaskedWorkspace=mask)
    # One can alternatively do
    # ws.maskDetectors(MaskedWorkspace=mask)

    # Check some pixels
    print "Is detector 0 masked:", ws.getDetector(0).isMasked()
    print "Is detector 6245 masked:", ws.getDetector(6245).isMasked()
    print "Is detector 11464 masked:", ws.getDetector(11464).isMasked()
    print "Is detector 17578 masked:", ws.getDetector(17578).isMasked()
    print "Is detector 20475 masked:", ws.getDetector(20475).isMasked()

Output:

.. testoutput:: DoIt

    Is detector 0 masked: True
    Is detector 6245 masked: True
    Is detector 11464 masked: False
    Is detector 17578 masked: False
    Is detector 20475 masked: True

**Example: Using reference workspace with spectra mask**
   
.. testcode:: 

    # Load workspace with real spectra-derector mask
    rws = Load(Filename=r'MAR11001.raw', OutputWorkspace='realWs',InlcudeMonitors=True);
    # Create simulation workspace for MARI using 1:1 spectra-detector map
    sws  = CreateSimulationWorkspace(Instrument='MARI', BinParams='-0.5,0.5,1', OutputWorkspace='simWS')
    # Create similar simulation workspace to compare different masking options
    swc  = CloneWorkspace(sws)

    # Create sample mask
    MaskDetectors(Workspace=rws, SpectraList='10,11,12,13,100,110,120,130,140,200,300')
    masked_list = ExportSpectraMask(rws,'SampleMask')
    file2remove = os.path.join(config.getString('defaultsave.directory'),'SampleMask.msk')
    print ("Masked Spectra No:  {0}".format(masked_list))
    #
    
    # Apply spectra mask to 1:1 instrument
    sws= LoadMask('SampleMask.msk','MARI',sws)
    # Apply spectra mask using real workspace spectra-detector map. 
    # rws1 = Load(Filename=r'MAR11001.raw', OutputWorkspace='realWs',InlcudeMonitors=True);
    # Note that rws does not need to be masked, We use it masked here only to avoid loading it again
    # it just needs to contain the same spectra-detector map as the initial workspace
    swc= LoadMask('SampleMask.msk',Workspace=rws)

    # Delete unnecessary test mask file
    os.remove(file2remove)

    # See the difference:
    nhist = sws.getNumberHistograms()
    sp00 = []
    dp0m = []
    sp1m = []
    dp1m = []
    sp2m = []
    dp2m = []
    for ind in xrange(0,nhist):
        try:
            sig0  = sum(rws.readY(ind))
            if sig0 == 0:
                sp00.append(ind)
            det0 = rws.getDetector(ind)
            if det0.isMasked():
                dp0m.append(det0.getID())
            if sws.readY(ind)[0]>0.5:
                det1 = sws.getDetector(ind)
                sp1m.append(ind)
                dp1m.append(det1.getID())
            if swc.readY(ind)[0]>0.5:        
                det1 = swc.getDetector(ind)
                sp2m.append(ind)
                dp2m.append(det1.getID())
        except:
            pass

    print "Zero signal Spect N: ",sp00
    print "Init  masked Det ID: ",dp0m
    print "Masked spectra ws 1: ",sp1m
    print "Masked detID   ws 1: ",dp1m
    print "Masked spectra ws 2: ",sp2m
    print "Masked detID   ws 2: ",dp2m
    print " Note identical detector ID-s for initial and mask 2 Ws:"
    dp0m.sort()
    dp2m.sort()
    print " Initial masked detID: ",dp0m
    print " Final   masked detID: ",dp2m


Output:


.. categories::

.. sourcelink::
