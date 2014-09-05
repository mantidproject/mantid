from xml.dom.minidom import getDOMImplementation
from datetime import datetime
from string import split,join
import re
class MantidGeom:

    def __init__(self, instname, valid_from=None):
        if valid_from is None:
            valid_from = datetime.now()
        the_future = datetime(2100, 1, 31, 23, 59, 59)
        self._document = getDOMImplementation().createDocument(None, "instrument", None)
        self._root = self._document.documentElement
        self._root.setAttribute("name", instname)
        self._root.setAttribute("valid-from", str(valid_from))
        self._root.setAttribute("valid-to", str(the_future))

    def __str__(self):
        return self._root.toprettyxml(indent='  ', newl='\n')

    def writeGeom(self, filename):
        """
            Write the XML geometry to the given filename
            @param filename: Path of the file to write to
        """
        uglyxml = self._root.toprettyxml(indent='  ', newl='\n')
        text_re = re.compile('>\n\s+([^<>\s].*?)\n\s+</', re.DOTALL)
        prettierxml = text_re.sub('>\g<1></', uglyxml)

        text_re = re.compile('((?:^)|(?:</[^<>\s].*?>)|(?:<[^<>\s].*?>)|(?:<[^<>\s].*?/>))\s*\n+', re.DOTALL)
        output_str = text_re.sub('\g<1>\n', prettierxml)

        fh = open(filename, "w")
        fh.write(output_str)
        fh.close()

    def showGeom(self):
        """
            Print the XML geometry to the screen
        """
        print self

    def addSnsDefaults(self):
        """
            Set the default properties for SNS geometries
        """
        defaults_element = self._append_child("defaults", self._root)
        self._append_child("length", defaults_element, unit="metre")
        self._append_child("angle", defaults_element, unit="degree")

        reference_element = self._append_child("reference-frame", defaults_element)
        self._append_child("along-beam", reference_element, axis="z")
        self._append_child("pointing-up", reference_element, axis="y")
        self._append_child("handedness", reference_element, axis="right")

    def addComment(self, comment):
        """
            Add a global comment to the XML file
            @param comment: comment to be added to the XML file
        """
        if comment is None: return

        child = self._document.createComment(str(comment))
        self._root.appendChild(child)

    def _append_child(self, element_name, element_parent, **kwargs):

        element = self._document.createElement(element_name)
        for item in kwargs:
            element.setAttribute(item, str(kwargs[item]))
        element_parent.appendChild(element)
        return element

    def addModerator(self, distance):
        """
            This adds the moderator position for the instrument
        """
        source = self._append_child("component", self._root, type="moderator")
        try:
          distance = float(distance)
          if distance > 0:
            distance *= -1.0
          self._append_child("location", source, z=distance)
        except:
            print "PROBLEM with addModerator"

        child = self._append_child("type", self._root, name="moderator")
        child.setAttribute("is", "Source")

    def addSamplePosition(self, location=None, coord_type="cartesian"):
        """
        Adds the sample position to the file. The coordinates should be passed
        as a tuple of (x, y, z) or (r, t, p). Default location is (0, 0, 0) in
        cartesian coordinates.
        """
        sample = self._append_child("component", self._root, type="sample-position")
        if location is None:
            self._append_child("location", sample, x="0.0", y="0.0", z="0.0")
        else:
            if coord_type is "cartesian":
                self._append_child("location", sample,
                                   x=location[0],
                                   y=location[1],
                                   z=location[2])
            if coord_type is "spherical":
                self._append_child("location", sample,
                                   r=location[0],
                                   t=location[1],
                                   p=location[2])

        child = self._append_child("type", self._root, name="sample-position")
        child.setAttribute("is", "SamplePos")

    def addDetectorPixels(self, name, r=[], theta=[], phi=[], names=[], energy=[]):
        type_element = self._append_child("type", self._root, name=name)

        for i in range(len(r)):
            for j in range(len(r[i])):
                if (str(r[i][j]) != "nan"):
                    basecomponent = self._append_child("component", type_element, type="pixel")
                    location_element = self._append_child("location", basecomponent,r=str(r[i][j]),
                          t=str(theta[i][j]), p=str(phi[i][j]), name=str(names[i][j]))
                    self._append_child("facing", location_element, x="0.0", y="0.0", z="0.0")

                    efixed_comp = self._append_child("parameter", basecomponent, name="Efixed")
                    self._append_child("value", efixed_comp, val=str(energy[i][j]))

    def addDetectorPixelsIdList(self, name, r=[], names=[]):
        component = self._append_child("idlist", self._root, idname=name)
        for i in range(len(r)):
            for j in range(len(r[i])):
                if (str(r[i][j]) != "nan"):
                    self._append_child("id", component, val=str(names[i][j]))

    def addMonitors(self, distance=[], names=[]):
        """
            Add a list of monitors to the geometry.
        """
        if len(distance) != len(names):
            raise IndexError("Distance and name list must be same size!")

        component = self._append_child("component", self._root, type="monitors", idlist="monitors")
        self._append_child("location", component)

        type_element = self._append_child("type", self._root, name="monitors")
        basecomponent = self._append_child("component", type_element, type="monitor")
        basecomponent.setAttribute("mark-as", "monitor")

        for i in range(len(distance)):
            zi=float(distance[i])
            self._append_child("location", basecomponent, z=distance[i], name=names[i])

    def addComponent(self, type_name, idlist=None, root=None, blank_location=True):
        """
        Add a component to the XML definition. A blank location is added.
        """
        if root is None:
            root = self._root
        comp = None
        if idlist is not None:
            comp = self._append_child("component", root, type=type_name, idlist=idlist)
        else:
            comp = self._append_child("component", root, type=type_name)
        l=comp
        if blank_location:
            l = self._append_child("location", comp)
        return l

    def makeTypeElement(self, name):
        """
        Return a simple type element.
        """
        return self._append_child("type", self._root, name=name)

    def makeDetectorElement(self, name, idlist_type=None, root=None):
        """
        Return a component element.
        """
        if root is not None:
            root_element = root
        else:
            root_element = self._root

        if idlist_type is not None:
            return self._append_child("component", root_element, type=name, idlist=idlist_type)
        else:
            return self._append_child("component", root_element, type=name)

    def makeIdListElement(self, name):
        return self._append_child("idlist", self._root, idname=name)

    def addDetector(self, x, y, z, rot_x, rot_y, rot_z, name, comp_type, usepolar=None):
        """
        Add a detector in a type element for the XML definition.
        """
        type_element = self._append_child("type", self._root, name=name)
        comp_element = self._append_child("component", type_element, type=comp_type)

        if usepolar is not None:
            self.addLocationPolar(comp_element, x, y, z)
        else:
           self.addLocation(comp_element, x, y, z, rot_x, rot_y, rot_z)

    def addSingleDetector(self, root, x, y, z, rot_x, rot_y, rot_z, name=None,
                          id=None, usepolar=None):
        """
        Add a single detector by explicit declaration. The rotation order is
        performed as follows: y, x, z.
        """
        if name is None:
            name = "bank"

        if usepolar is not None:
            self.addLocationPolar(root, x, y, z, name)
        else:
            self.addLocation(root, x, y, z, rot_x, rot_y, rot_z, name)

    def addLocation(self, root, x, y, z, rot_x, rot_y, rot_z, name=None):
        """
        Add a location element to a specific parent node given by root.
        """
        if name is not None:
            pos_loc = self._append_child("location", root, x=str(x), y=str(y), z=str(z), name=name)
        else:
            pos_loc = self._append_child("location", root, x=str(x), y=str(y), z=str(z))

        if rot_y is not None:
            r1 = self._append_child("rot", pos_loc, **{"val":str(rot_y), "axis-x":"0",
                                                       "axis-y":"1", "axis-z":"0"})
        else:
            r1 = pos_loc

        if rot_x is not None:
            r2 = self._append_child("rot", r1, **{"val":str(rot_x), "axis-x":"1",
                                                  "axis-y":"0", "axis-z":"0"})
        else:
            r2 = r1

        if rot_z is not None:
            self._append_child("rot", r2, **{"val":str(rot_z), "axis-x":"0",
                                             "axis-y":"0", "axis-z":"1"})

    def addLocationPolar(self, root, r, theta, phi, name=None):
        if name is not None:
            pos_loc = self._append_child("location", root, r=r, t=theta, p=phi, name=name)
        else:
            pos_loc = self._append_child("location", root, r=r, t=theta, p=phi)

    def addLocationRTP(self, root, r, t, p, rot_x, rot_y, rot_z, name=None):
        """
        Add a location element to a specific parent node given by root, using r, theta, phi coordinates.
        """
        rf=float(r)
        tf=float(f)
        pf=float(p)
        if name is not None:
            pos_loc = self._append_child("location", root, r=r, t=t, p=p, name=name)
        else:
            pos_loc = self._append_child("location", root, r=r, t=t, p=p)
        #add rotx, roty, rotz
        #Regardless of what order rotx, roty and rotz is specified in the IDF,
        #the combined rotation is equals that obtained by applying rotx, then roty and finally rotz.
        if rot_x is not None:
            log = self._append_child("parameter", pos_loc, name="rotx")
            rotxf=float(rot_x)
            self._append_child("value", log, val=rot_x)
        if rot_y is not None:
            log = self._append_child("parameter", pos_loc, name="roty")
            rotyf=float(rot_y)
            self._append_child("value", log, val=rot_y)
        if rot_z is not None:
            log = self._append_child("parameter", pos_loc, name="rotz")
            rotzf=float(rot_z)
            self._append_child("value", log, val=rot_z)

    def addNPack(self, name, num_tubes, tube_width, air_gap, type_name="tube"):
        """
        Add a block of N tubes in a pack. A name for the pack type needs
        to be specified as well as the number of tubes in the pack, the tube
        width and air gap. If there are going to be more than one type tube
        specified later, an optional type name can be given. The default tube
        type name will be tube.
        """
        type_element = self._append_child("type", self._root, name=name)
        self._append_child("properties", type_element)

        component = self._append_child("component", type_element, type=type_name)

        effective_tube_width = tube_width + air_gap

        pack_start = (effective_tube_width / 2.0) * (1 - num_tubes)

        for i in range(num_tubes):
            tube_name = "tube%d" % (i + 1)
            x = pack_start + (i * effective_tube_width)
            self._append_child("location", component, name=tube_name, x=str(x))

    def addPixelatedTube(self, name, num_pixels, tube_height,
                         type_name="pixel"):
        """
        Add a tube of N pixels. If there are going to be more than one pixel
        type specified later, an optional type name can be given. The default
        pixel type name will be pixel.
        """
        type_element = self._append_child("type", self._root, outline="yes", name=name)
        self._append_child("properties", type_element)
        component = self._append_child("component", type_element, type=type_name)

        pixel_width = tube_height / num_pixels
        tube_start = (pixel_width / 2.0) * (1 - num_pixels)

        for i in range(num_pixels):
            pixel_name = "pixel%d" % (i + 1)
            y = tube_start + (i * pixel_width)
            self._append_child("location", component, name=pixel_name, y=str(y))

    def addCylinderPixel(self, name, center_bottom_base, axis, pixel_radius,
                         pixel_height, is_type="detector"):
        """
        Add a cylindrical pixel. The center_bottom_base is a 3-tuple of radius,
        theta, phi. The axis is a 3-tuple of x, y, z.
        """
        type_element = self._append_child("type", self._root, **{"name":name, "is":is_type})
        cylinder = self._append_child("cylinder", type_element, id="cyl-approx")
        self._append_child("centre-of-bottom-base", cylinder,
                           r=str(center_bottom_base[0]),
                           t=str(center_bottom_base[1]),
                           p=str(center_bottom_base[2]))
        self._append_child("axis", cylinder,
                           x=str(axis[0]), y=str(axis[1]), z=str(axis[2]))

        self._append_child("radius", cylinder, val=str(pixel_radius))
        self._append_child("height", cylinder, val=str(pixel_height))
        self._append_child("algebra", type_element, val="cyl-approx")

    def addCuboidPixel(self, name, lfb_pt, lft_pt, lbb_pt, rfb_pt,
                      is_type="detector"):
        """
        Add a cuboid pixel. The origin of the cuboid is assumed to be the
        center of the front face of the cuboid. The parameters lfb_pt, lft_pt,
        lbb_pt, rfb_pt are 3-tuple of x, y, z.
        """
        type_element = self._append_child("type", self._root, **{"name":name, "is":is_type})
        cuboid = self._append_child("cuboid", type_element, id="shape")
        self._append_child("left-front-bottom-point", cuboid, x=str(lfb_pt[0]),
                      y=str(lfb_pt[1]), z=str(lfb_pt[2]))
        self._append_child("left-front-top-point", cuboid, x=str(lft_pt[0]),
                      y=str(lft_pt[1]), z=str(lft_pt[2]))
        self._append_child("left-back-bottom-point", cuboid, x=str(lbb_pt[0]),
                      y=str(lbb_pt[1]), z=str(lbb_pt[2]))
        self._append_child("right-front-bottom-point", cuboid, x=str(rfb_pt[0]),
                      y=str(rfb_pt[1]), z=str(rfb_pt[2]))
        self._append_child("algebra", type_element, val="shape")

    def addDummyMonitor(self, radius, height):
        """
        Add a dummy monitor with some-shape.
        """
        type_element = self._append_child("type", self._root, **{"name":"monitor", "is":"detector"})
        cylinder = self._append_child("cylinder", type_element, id="cyl-approx")
        self._append_child("centre-of-bottom-base", cylinder, x="0.0", y="0.0", z="0.0")
        self._append_child("axis", cylinder, x="0.0", y="0.0", z="1.0")
        self._append_child("radius", cylinder, radius=str(radius))
        self._append_child("height", cylinder, height=str(height))
        self._append_child("algebra", type_element, val="cyl-approx")

    def addCuboidMonitor(self,width,height,depth):
        """
        Add a cuboid monitor
        """
        type_element = self._append_child("type", self._root, **{"name":"monitor", "is":"detector"})
        cuboid = self._append_child("cuboid", type_element, id="shape")
        self._append_child("left-front-bottom-point", cuboid, x=str(-width/2), y=str(-height/2),z=str(-depth/2))
        self._append_child("left-front-top-point", cuboid, x=str(-width/2), y=str(height/2),z=str(-depth/2))
        self._append_child("left-back-bottom-point", cuboid, x=str(-width/2), y=str(-height/2),z=str(depth/2))
        self._append_child("right-front-bottom-point", cuboid, x=str(width/2), y=str(-height/2),z=str(-depth/2))
        self._append_child("algebra", type_element, val="shape")

    def addDetectorIds(self, idname, idlist):
        """
        Add the detector IDs. A list is provided that must be divisible by 3.
        The list should be specified as [start1, end1, step1, start2, end2,
        step2, ...]. If no step is required, use None.
        """
        if len(idlist) % 3 != 0:
            raise IndexError("Please specifiy list as [start1, end1, step1, "\
                             +"start2, end2, step2, ...]. If no step is"\
                             +"required, use None.")
        num_ids = len(idlist) / 3
        id_element = self._append_child("idlist", self._root, idname=idname)
        for i in range(num_ids):
            if idlist[(i*3)+2] is None:
                self._append_child("id", id_element, start=str(idlist[(i*3)]),
                              end=str(idlist[(i*3)+1]))
            else:
                self._append_child("id", id_element, start=str(idlist[(i*3)]),
                              step=str(idlist[(i*3)+2]),
                              end=str(idlist[(i*3)+1]))

    def addMonitorIds(self, ids=[]):
        """
        Add the monitor IDs.
        """
        idElt = self._append_child("idlist", self._root, idname="monitors")
        for i in range(len(ids)):
            self._append_child("id", idElt, val=ids[i])

    def addDetectorParameters(self, component_name, *args):
        """
        Add detector parameters to a particular component name. Args is an
        arbitrary list of 3-tuples containing the following information:
        (parameter name, parameter value, parameter units).
        """
        complink = self._append_child("component-link", self._root, name=component_name)
        for arg in args:
            if len(arg) != 3:
                raise IndexError("Will not be able to parse:", arg)

            par = self._append_child("parameter", complink, name=arg[0])
            self._append_child("value", par, val=str(arg[1]), units=str(arg[2]))
