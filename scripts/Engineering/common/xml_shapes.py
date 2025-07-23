# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

# utility functions for creating standard shape xml strings


def get_cube_xml(name, side_len, centre=(0.0, 0.0, 0.0)):
    return f"""
    <cuboid id='{name}'> \
    <height val='{side_len}'  /> \
    <width val='{side_len}' />  \
    <depth  val='{side_len}' />  \
    <centre x='{centre[0]}' y='{centre[1]}' z='{centre[2]}'  />  \
    </cuboid>  \
    <algebra val='{name}' /> \\ """
