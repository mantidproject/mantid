# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
#! /usr/bin/python
# Convert an acor file into VTK format (specifically a vtp file)
from xml.dom import minidom


def convertToVTU(infile, outpath):
    # first need to find some things from the file
    datafile = open(infile, "r")
    datalist = []
    planelist = []
    npoints = 0
    for line in datafile:
        numbers = line.split()
        if len(numbers) != 4:
            continue
        if npoints == 0:
            curz = numbers[2]
        if numbers[2] != curz:
            datalist.append(planelist)
            curz = numbers[2]
            planelist = []
        planelist.append(numbers)
        npoints += 1

    # Append last set
    datalist.append(planelist)
    datafile.close()

    ncells = len(datalist)
    doc = minidom.Document()

    vtkfile = doc.createElement("VTKFile")
    doc.appendChild(vtkfile)
    vtkfile.setAttribute("type", "UnstructuredGrid")
    vtkfile.setAttribute("version", "0.1")
    vtkfile.setAttribute("byte_order", "LittleEndian")

    ugrid = doc.createElement("UnstructuredGrid")
    vtkfile.appendChild(ugrid)
    piece = doc.createElement("Piece")
    ugrid.appendChild(piece)

    piece.setAttribute("NumberOfPoints", str(npoints))
    piece.setAttribute("NumberOfCells", str(ncells))

    # First the PointData element
    point_data = doc.createElement("PointData")
    piece.appendChild(point_data)
    point_data.setAttribute("Scalars", "Intensity")

    data_array = doc.createElement("DataArray")
    point_data.appendChild(data_array)
    data_array.setAttribute("type", "Float32")
    data_array.setAttribute("Name", "Intensity")
    data_array.setAttribute("format", "ascii")

    for plane in datalist:
        for point in plane:
            txt = doc.createTextNode(str(point[3]))
            data_array.appendChild(txt)

    # Now the Points element
    points = doc.createElement("Points")
    piece.appendChild(points)

    data_array = doc.createElement("DataArray")
    points.appendChild(data_array)
    data_array.setAttribute("type", "Float32")
    data_array.setAttribute("NumberOfComponents", "3")
    data_array.setAttribute("format", "ascii")
    for plane in datalist:
        for point in plane:
            txt = doc.createTextNode(str(point[0]) + " " + str(point[1]) + " " + str(point[2]))
            data_array.appendChild(txt)

    cells = doc.createElement("Cells")
    piece.appendChild(cells)

    data_array = doc.createElement("DataArray")
    cells.appendChild(data_array)
    data_array.setAttribute("type", "Int32")
    data_array.setAttribute("Name", "connectivity")
    data_array.setAttribute("format", "ascii")

    i = 0
    for plane in datalist:
        for point in plane:
            txt = doc.createTextNode(str(i))
            data_array.appendChild(txt)
            i += 1

    data_array = doc.createElement("DataArray")
    cells.appendChild(data_array)
    data_array.setAttribute("type", "Int32")
    data_array.setAttribute("Name", "offsets")
    data_array.setAttribute("format", "ascii")

    i = 0
    for plane in datalist:
        i += len(plane)
        txt = doc.createTextNode(str(i))
        data_array.appendChild(txt)

    data_array = doc.createElement("DataArray")
    cells.appendChild(data_array)
    data_array.setAttribute("type", "Int32")
    data_array.setAttribute("Name", "types")
    data_array.setAttribute("format", "ascii")

    for plane in datalist:
        txt = doc.createTextNode("4")
        data_array.appendChild(txt)

    # print doc.toprettyxml(newl="\n")
    shortname = infile.split("/")
    name = outpath + shortname[len(shortname) - 1] + ".vtu"
    handle = open(name, "w")
    doc.writexml(handle, newl="\n")
    handle.close()

    del datalist
    del planelist
    del doc


def writeParallelVTU(files, prefix):
    doc = minidom.Document()

    vtkfile = doc.createElement("VTKFile")
    doc.appendChild(vtkfile)
    vtkfile.setAttribute("type", "PUnstructuredGrid")
    vtkfile.setAttribute("version", "0.1")
    vtkfile.setAttribute("byte_order", "LittleEndian")

    pugrid = doc.createElement("PUnstructuredGrid")
    vtkfile.appendChild(pugrid)
    pugrid.setAttribute("GhostLevel", "0")

    ppointdata = doc.createElement("PPointData")
    pugrid.appendChild(ppointdata)
    ppointdata.setAttribute("Scalars", "Intensity")

    data_array = doc.createElement("PDataArray")
    ppointdata.appendChild(data_array)
    data_array.setAttribute("type", "Float32")
    data_array.setAttribute("Name", "Intensity")

    ppoints = doc.createElement("PPoints")
    pugrid.appendChild(ppoints)
    data_array = doc.createElement("PDataArray")
    ppoints.appendChild(data_array)
    data_array.setAttribute("type", "Float32")
    data_array.setAttribute("NumberOfComponents", "3")

    for name in files:
        piece = doc.createElement("Piece")
        pugrid.appendChild(piece)
        piece.setAttribute("Source", name + ".vtu")

    #    print doc.toprettyxml(newl="\n")
    filename = prefix + files[0].split(".")[0] + ".pvtu"
    #    print filename
    handle = open(filename, "w")
    doc.writexml(handle, newl="\n")
    handle.close()
