#pylint: disable=invalid-name
from geometry_writer import MantidGeom

NUM_PIXELS_PER_TUBE = 304
NUM_TUBES = 256
PIXEL_WIDTH = 0.0007
PIXEL_HEIGHT = 0.0007

def create_grouping(workspace=None):
    # This should be read from the
    npix_x = 256
    npix_y = 304

    ## Integrated over X
    if workspace is not None:
        if mtd[workspace].getInstrument().hasParameter("number-of-x-pixels"):
            npix_x = int(mtd[workspace].getInstrument().getNumberParameter("number-of-x-pixels")[0])
        if mtd[workspace].getInstrument().hasParameter("number-of-y-pixels"):
            npix_y = int(mtd[workspace].getInstrument().getNumberParameter("number-of-y-pixels")[0])

    f = open("REFL_Detector_Grouping_Sum_X_rot.xml",'w')
    f.write("<detector-grouping description=\"Integrated over X\">\n")

    for y in range(npix_y):
        # index = max_y * x + y
        indices = range(y, npix_x*(npix_y), npix_y)

        # Detector IDs start at zero, but spectrum numbers start at 1
        # Grouping works on spectrum numbers
        indices_lst = [str(i+1) for i in indices]
        indices_str = ','.join(indices_lst)
        f.write("  <group name='%d'>\n" % y)
        f.write("    <ids val='%s'/>\n" % indices_str)
        f.write("  </group>\n")

    f.write("</detector-grouping>\n")
    f.close()

    ## Integrated over Y
    f = open("REFL_Detector_Grouping_Sum_Y_rot.xml",'w')
    f.write("<detector-grouping description=\"Integrated over Y\">\n")

    for x in range(npix_x):
        # index = max_y * x + y
        indices = range(x*npix_y,(x+1)*npix_y)

        # Detector IDs start at zero, but spectrum numbers start at 1
        # Grouping works on spectrum numbers
        indices_lst = [str(i+1) for i in indices]
        indices_str = ','.join(indices_lst)
        f.write("  <group name='%d'>\n" % 303)
        f.write("    <ids val='%s'/>\n" % indices_str)
        f.write("  </group>\n")

    f.write("</detector-grouping>\n")
    f.close()


def create_geometry(file_name=None, pixel_width=None, pixel_height=None):
    inst_name = "REF_L"
    short_name = "REF_L"

    if pixel_width is None: pixel_width = PIXEL_WIDTH
    if pixel_height is None: pixel_height = PIXEL_HEIGHT

    if file_name is None:
        xml_outfile = inst_name+"_Definition.xml"
    else:
        xml_outfile = file_name

    det = MantidGeom(inst_name)
    det.addSnsDefaults()
    det.addComment("SOURCE AND SAMPLE POSITION")
    det.addModerator(-13.63)  #was 13.601
    det.addSamplePosition()
    det.addComment("MONITORS")
    det.addMonitors(names=["monitor1"], distance=["-0.23368"])
    det.addComment("DETECTOR")
    id_str = "detector1"
    det.addComponent("detector1", id_str)
    doc_handle = det.makeTypeElement(id_str)

    det.addPixelatedTube("tube", NUM_PIXELS_PER_TUBE, PIXEL_HEIGHT*NUM_PIXELS_PER_TUBE, type_name="pixel")

    det.addCylinderPixel("pixel", (0.0, 0.0, 0.0), (0.0, 1.0, 0.0),
                         pixel_width/2.0, pixel_height)

    for i in range(0, NUM_TUBES):
        det.addComponent("tube%d" % i, root=doc_handle)
        det.addDetector(str((i-NUM_TUBES/2+0.5)*pixel_width), "0", "+1.28", "0", "0", "0", "tube%d"%i, "tube")

    det.addComment("FIXME: Do something real here.")
    det.addDummyMonitor(0.01, 0.03)

    id_list = [0, NUM_TUBES*NUM_PIXELS_PER_TUBE-1, None]
    det.addDetectorIds(id_str, id_list)
    det.addMonitorIds(["-1"])

    det.writeGeom(xml_outfile)

if __name__ == "__main__":
    create_geometry()
    #create_grouping()
