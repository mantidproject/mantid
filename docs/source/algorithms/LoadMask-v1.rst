.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to load a masking file, which can be in XML
format (defined later in this page) or old-styled calibration file.
The ``Instrument`` can be a IDF.

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


When a spectra mask (ISIS) is used on multiple workspaces, the same masking is produced only
if all masked workspaces have the same spectra-detector map.
When mask is generated for one workspace and applied to workspace with different
spectra-detector mapping, the same masking can be produced by using *Workspace*
option, using this workspace as the source of the spectra-detector mapping.
See the Spectra mask usage sample below.


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
    print("Is detector 0 masked: {}".format(ws.getDetector(0).isMasked()))
    print("Is detector 6245 masked: {}".format(ws.getDetector(6245).isMasked()))
    print("Is detector 11464 masked: {}".format(ws.getDetector(11464).isMasked()))
    print("Is detector 17578 masked: {}".format(ws.getDetector(17578).isMasked()))
    print("Is detector 20475 masked: {}".format(ws.getDetector(20475).isMasked()))

Output:

.. testoutput:: DoIt

    Is detector 0 masked: True
    Is detector 6245 masked: True
    Is detector 11464 masked: False
    Is detector 17578 masked: False
    Is detector 20475 masked: True

**Example: Using reference workspace with Spectra Mask**

.. testcode:: ExLoadSpectraMask

    # Load workspace with real spectra-derector mask
    rws = Load(Filename=r'MAR11001.raw', OutputWorkspace='realWs',InlcudeMonitors=True);

    # Create sample mask
    MaskDetectors(Workspace=rws, SpectraList='10,11,12,13,100,110,120,130,140,200,300')
    masked_list = ExportSpectraMask(rws,'SampleMask')
    file2remove = os.path.join(config.getString('defaultsave.directory'),'SampleMask.msk')

    #

    # Load sample  spectra mask using 1:1 instrument
    mask1to1ws= LoadMask('MARI','SampleMask.msk')

    # Apply spectra mask using real workspace spectra-detector map.
    # Note that rws does not need to be masked like its here, We use it masked here only to avoid overhead of loading it again
    # it just needs to contain the same spectra-detector map as the initial workspace
    # you may want to try rows below to be sure:
    #rws1 = Load(Filename=r'MAR11001.raw', OutputWorkspace='realWs1',InlcudeMonitors=True);

    # Load Mask using  instrument and spectra-detector map provided with source workspace
    maskRealSDM=LoadMask('MARI','SampleMask.msk',RefWorkspace=rws)

    # Clear up rubbish
    os.remove(file2remove)

    # See the difference:
    nhist = mask1to1ws.getNumberHistograms()
    Sig0Masked = []
    Det0Masked= []
    MaskedSp1to1 = []
    MaskedDet1to1= []
    MaskedSp_R  = []
    MaskedDet_R= []
    for ind in range(0,nhist):
        try:
            det = rws.getDetector(ind)
            if det.isMasked():
                Sig0Masked.append(ind)
                Det0Masked.append(det.getID())
            #  1:1 map generated from instrument definitions
            if mask1to1ws.readY(ind)[0]>0.5:
                det = mask1to1ws.getDetector(ind)
                MaskedSp1to1.append(ind)
                MaskedDet1to1.append(det.getID())
            # Real spectra-detector map:
            if maskRealSDM.readY(ind)[0]>0.5:
                det = maskRealSDM.getDetector(ind)
                MaskedSp_R.append(ind)
                MaskedDet_R.append(det.getID())
        except:
            pass
    print("*** ************************************ **********************************************")
    print("*** Masked Spec. Id(s):  {0}".format(masked_list))
    print( "*** Initial workspace masking parameters **********************************************")
    print("Masked Spectra Numbers:  {}".format(Sig0Masked))
    print("Masked Detector IDs   :  {}".format(Det0Masked))
    print("*** One to one mask workspace has masked the same spectra numbers but different detectors")
    print("ws 1to1 Masked spectra:  {}".format(MaskedSp1to1))
    print("ws 1to1 Masked DedIDs :  {}".format(MaskedDet1to1))
    print("*** Real spectra-det-map workspace has masked different spectra numbers but the same detectors")
    print("ws RSDM Masked spectra:  {}".format(MaskedSp_R))
    print("ws RSDM Masked DedIDs :  {}".format(MaskedDet_R))
    print("*** indeed the same:")
    Det0Masked.sort()
    MaskedDet_R.sort()
    print("sorted initial DetIDs :  {}".format(Det0Masked))
    print("sorted RSDM    DedIDs :  {}".format(MaskedDet_R))
    print("*** ************************************ **********************************************")
    print("*** note spectra with id 4 is a monitor, not present in the masking workspaces")
    print("*** ************************************ **********************************************")

Output:

.. testoutput:: ExLoadSpectraMask

    *** ************************************ **********************************************
    *** Masked Spec. Id(s):  [  4  10  11  12  13 100 110 120 130 140 200 300]
    *** Initial workspace masking parameters **********************************************
    Masked Spectra Numbers:  [9, 10, 11, 12, 99, 109, 119, 129, 139, 199, 299]
    Masked Detector IDs   :  [4106, 4107, 4108, 4109, 4608, 4702, 4712, 4806, 4816, 2220, 2524]
    *** One to one mask workspace has masked the same spectra numbers but different detectors
    ws 1to1 Masked spectra:  [9, 10, 11, 12, 99, 109, 119, 129, 139, 199, 299]
    ws 1to1 Masked DedIDs :  [1110, 1111, 1112, 1113, 1401, 1411, 1421, 1431, 1509, 1705, 2201]
    *** Real spectra-det-map workspace has masked different spectra numbers but the same detectors
    ws RSDM Masked spectra:  [318, 418, 787, 788, 789, 790, 877, 887, 897, 907, 917]
    ws RSDM Masked DedIDs :  [2220, 2524, 4106, 4107, 4108, 4109, 4608, 4702, 4712, 4806, 4816]
    *** indeed the same:
    sorted initial DetIDs :  [2220, 2524, 4106, 4107, 4108, 4109, 4608, 4702, 4712, 4806, 4816]
    sorted RSDM    DedIDs :  [2220, 2524, 4106, 4107, 4108, 4109, 4608, 4702, 4712, 4806, 4816]
    *** ************************************ **********************************************
    *** note spectra with id 4 is a monitor, not present in the masking workspaces
    *** ************************************ **********************************************

.. categories::

.. sourcelink::
