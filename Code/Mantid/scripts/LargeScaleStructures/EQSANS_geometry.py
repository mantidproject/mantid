from geometry_writer import MantidGeom
import math

RADIUS = 5.0
NUM_PIXELS_PER_TUBE = 256
TUBE_SIZE = 1.1000
NUM_TUBES_PER_BANK = 4
TUBE_WIDTH = 0.011
AIR_GAP_WIDTH = 0.0
NUM_BANKS = 48
NUM_BANKS_PER_LAYER = 24
GAP_BTW_PLANES = 0.0082

def get_position(bank, tube, tube_width=TUBE_WIDTH):
    """
        Get the location of a tube in real-space
        @param bank: bank number [0:47]
        @param tube: tube number within the bank [0:3]
        @param tube_width: width of the tube (pixel width)
    """
    # Determine which plane we are on
    i_plane = math.floor(bank/NUM_BANKS_PER_LAYER)
    # Get arc length
    # Minus 0.5 pixel because we are looking for the center of the tube
    arc = ( NUM_TUBES_PER_BANK*(NUM_BANKS/4.0 - (bank - i_plane*NUM_BANKS_PER_LAYER)) - tube -0.5 )*tube_width
    theta = arc/RADIUS
    z = RADIUS*math.cos(theta)-RADIUS+i_plane*GAP_BTW_PLANES
    x = RADIUS*math.sin(theta)
    return x, 0, z

def create_geometry(file_name=None, tube_width=TUBE_WIDTH, tube_length=TUBE_SIZE):
    """
        Create a geometry file
        @param file_name: name of the output file [optional]
        @param tube_width: width of the tubes (pixel width)
        @param tube_length: length of the tubes (number of pixels times pixel height)
    """
    # Get geometry information file
    inst_name = "EQ-SANS"
    if file_name is None:
        xml_outfile = inst_name+"_Definition.xml"
    else:
        xml_outfile = file_name

    det = MantidGeom(inst_name)
    det.addSnsDefaults()
    det.addComment("SOURCE AND SAMPLE POSITION")
    det.addModerator(-14.122)
    det.addSamplePosition()
    det.addComment("MONITORS")
    det.addMonitors(names=["monitor1"], distance=["-0.23368"])

    id_str = "detector1"
    det.addComponent(id_str, id_str)
    doc_handle = det.makeTypeElement(id_str)

    det.addCylinderPixel("pixel", (0.0, 0.0, 0.0), (0.0, 1.0, 0.0),
                        (tube_width/2.0),
                        (tube_length/NUM_PIXELS_PER_TUBE))

    for i in range(0, NUM_BANKS/2):
        i_low_bank = i
        i_high_bank = i+NUM_BANKS/2
        low_bank = "bank"+str(i_low_bank+1)
        high_bank = "bank"+str(i_high_bank+1)

        # FRONT plane
        for j in range(NUM_TUBES_PER_BANK):
            tube_id = "bank"+str(i_low_bank+1)+"_tube"+str(j+1)
            x, y, z = get_position(i_low_bank, j, tube_width)
            det.addComponent(tube_id, root=doc_handle)
            det.addPixelatedTube('_'+tube_id, NUM_PIXELS_PER_TUBE, tube_length, type_name="pixel")
            det.addDetector(x, y, z, "0", "0", "0", tube_id, '_'+tube_id)

        # BACK plane
        for j in range(NUM_TUBES_PER_BANK):
            tube_id = "bank"+str(i_high_bank+1)+"_tube"+str(j+1)
            x, y, z = get_position(i_high_bank, j, tube_width)
            det.addComponent(tube_id, root=doc_handle)
            det.addPixelatedTube('_'+tube_id, NUM_PIXELS_PER_TUBE, tube_length, type_name="pixel")
            det.addDetector(x, y, z, "0", "0", "0", tube_id, '_'+tube_id)

    det.addComment("MONITOR SHAPE")
    det.addComment("FIXME: Do something real here.")
    det.addDummyMonitor(0.01, 0.03)

    det.addComment("DETECTOR IDs")
    id_list = [0, NUM_BANKS*NUM_TUBES_PER_BANK*NUM_PIXELS_PER_TUBE-1, None]
    det.addDetectorIds(id_str, id_list)

    det.addComment("MONITOR IDs")
    det.addMonitorIds(["-1"])

    det.writeGeom(xml_outfile)

if __name__ == "__main__":
    create_geometry()