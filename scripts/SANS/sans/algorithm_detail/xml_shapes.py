from __future__ import (absolute_import, division, print_function)
from math import (pi, cos, sin)


def add_xml_shape(xml, complete_xml_element):
    """
        Add an arbitrary shape to region to be masked
        @param xml: a list of shapes to which we append here
        @param complete_xml_element: description of the shape to add
    """
    if not complete_xml_element.startswith('<'):
        raise ValueError('Excepted xml string but found: ' + str(complete_xml_element))
    xml.append(complete_xml_element)


def infinite_plane(shape_id, plane_pt, normal_pt):
    """
        Generates xml code for an infinite plane
        @param shape_id: a string to refer to the shape by
        @param plane_pt: a point in the plane
        @param normal_pt: the direction of a normal to the plane
        @return the xml string
    """
    return '<infinite-plane id="' + str(shape_id) + '">' + \
           '<point-in-plane x="' + str(plane_pt[0]) + '" y="' + str(plane_pt[1]) + '" z="' + \
           str(plane_pt[2]) + '" />' + \
           '<normal-to-plane x="' + str(normal_pt[0]) + '" y="' + str(normal_pt[1]) + '" z="' + \
           str(normal_pt[2]) + '" />' + \
           '</infinite-plane>\n'


def infinite_cylinder(centre, radius, axis, shape_id='shape'):
    """
        Generates xml code for an infintely long cylinder
        @param centre: a tupple for a point on the axis
        @param radius: cylinder radius
        @param axis: cylinder orientation
        @param shape_id: a string to refer to the shape by
        @return the xml string
    """
    return '<infinite-cylinder id="' + str(shape_id) + '">' + \
           '<centre x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + '" />' + \
           '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
           '<radius val="' + str(radius) + '" /></infinite-cylinder>\n'


def finite_cylinder(centre, radius, height, axis, shape_id='shape'):
    """
        Generates xml code for an infintely long cylinder
        @param centre: a tuple for a point on the axis
        @param radius: cylinder radius
        @param height: cylinder height
        @param axis: cylinder orientation
        @param shape_id: a string to refer to the shape by
        @return the xml string
    """
    return '<cylinder id="' + str(shape_id) + '">' + \
           '<centre-of-bottom-base x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + \
           '" />' + \
           '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
           '<radius val="' + str(radius) + '" /><height val="' + str(height) + '" /></cylinder>\n'


def add_cylinder(xml, radius, xcentre, ycentre, shape_id='shape'):
    """Mask the inside of an infinite cylinder on the input workspace."""
    add_xml_shape(xml, infinite_cylinder([xcentre, ycentre, 0.0], radius, [0, 0, 1], shape_id=shape_id) +
                  '<algebra val="' + str(shape_id) + '"/>')


def add_outside_cylinder(xml, radius, xcentre=0.0, ycentre=0.0, shape_id='shape'):
    """Mask out the outside of a cylinder or specified radius """
    add_xml_shape(xml, infinite_cylinder([xcentre, ycentre, 0.0], radius, [0, 0, 1], shape_id=shape_id) +
                  '<algebra val="#' + str(shape_id + '"/>'))


def create_phi_mask(shape_id, centre, phi_min, phi_max, use_mirror=True):
    """  Mask the detector bank such that only the region specified in the phi range is left unmasked."""
    # Convert all angles to be between 0 and 360
    while phi_max > 360:
        phi_max -= 360
    while phi_max < 0:
        phi_max += 360
    while phi_min > 360:
        phi_min -= 360
    while phi_min < 0:
        phi_min += 360
    while phi_max < phi_min:
        phi_max += 360

    # Convert to radians
    phi_min = pi * phi_min / 180.0
    phi_max = pi * phi_max / 180.0

    shape_id = str(shape_id)
    lim_phi_xml = infinite_plane(shape_id + '_plane1', centre, [cos(-phi_min + pi / 2.0),
                                                                sin(-phi_min + pi / 2.0), 0]) \
                  + infinite_plane(shape_id + '_plane2', centre, [-cos(-phi_max + pi / 2.0),  # noqa
                                                                  -sin(-phi_max + pi / 2.0), 0])

    if use_mirror:
        lim_phi_xml += infinite_plane(shape_id + '_plane3', centre, [cos(-phi_max + pi / 2.0),
                                                                     sin(-phi_max + pi / 2.0), 0]) \
                        + infinite_plane(shape_id + '_plane4', centre, [-cos(-phi_min + pi / 2.0),
                                                                        -sin(-phi_min + pi / 2.0), 0]) \
                        + '<algebra val="#((' + shape_id + '_plane1 ' + shape_id + '_plane2):(' \
                        + shape_id + '_plane3 ' + shape_id + '_plane4))" />'
    else:
        # the formula is different for acute verses obtuse angles
        if phi_max - phi_min > pi:
            # to get an obtuse angle, a wedge that's more than half the area, we need to add the semi-infinite volumes
            lim_phi_xml += '<algebra val="#(' + shape_id + '_plane1:' + shape_id + '_plane2)" />'
        else:
            # an acute angle, wedge is more less half the area, we need to use the intersection of
            #  those semi-infinite volumes
            lim_phi_xml += '<algebra val="#(' + shape_id + '_plane1 ' + shape_id + '_plane2)" />'
    return lim_phi_xml


def create_line_mask(start_point, length, width, angle):
    """  Creates the xml to mask a line of the given width and height at the given angle into the member _line_xml.

     The masking object which is used to mask a line of say a detector array is a finite cylinder
    :param start_point: startPoint of line
    :param length: length of line
    :param width: width of line
    :param angle: angle of line in xy-plane in units of degrees
    :return: return xml shape string
    """
    return finite_cylinder(start_point, width / 2., length, [cos(angle * pi / 180.0), sin(angle * pi / 180.0), 0.0],
                           "arm")
