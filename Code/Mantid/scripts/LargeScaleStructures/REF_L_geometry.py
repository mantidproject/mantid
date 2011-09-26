from geometry_writer import MantidGeom
import math

NUM_PIXELS_PER_TUBE = 256
NUM_TUBES = 304
PIXEL_WIDTH = 0.0007
PIXEL_HEIGHT = 0.0007


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
    det.addModerator(-13.601)
    det.addSamplePosition()
    det.addComment("MONITORS")
    det.addMonitors(names=["monitor1"], distance=["-0.23368"])

    id_str = "detector1"
    det.addComponent("detector1", id_str)
    doc_handle = det.makeTypeElement(id_str)

    det.addPixelatedTube("tube", NUM_PIXELS_PER_TUBE, pixel_height*NUM_PIXELS_PER_TUBE, type_name="pixel")
    
    det.addCylinderPixel("pixel", (0.0, 0.0, 0.0), (0.0, 1.0, 0.0),
                         pixel_width/2.0, pixel_height)
    
    for i in range(0, NUM_TUBES):
        det.addComponent("tube%d" % i, root=doc_handle)
        det.addDetector(str((i-NUM_TUBES/2+0.5)*pixel_width), "0", "0", "0", "0", "0", "tube%d"%i, "tube")

    det.addComment("FIXME: Do something real here.")
    det.addDummyMonitor(0.01, 0.03)

    id_list = [0, NUM_TUBES*NUM_PIXELS_PER_TUBE-1, None]
    det.addDetectorIds(id_str, id_list)
    det.addMonitorIds(["-1"])


    det.writeGeom(xml_outfile)

if __name__ == "__main__":
    create_geometry()