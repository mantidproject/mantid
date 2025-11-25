from enum import Enum


class ProjectionType(str, Enum):
    THREE_D = "3D"
    SPHERICAL_X = "Spherical X"
    SPHERICAL_Y = "Spherical Y"
    SPHERICAL_Z = "Spherical Z"
    CYLINDRICAL_X = "Cylindrical X"
    CYLINDRICAL_Y = "Cylindrical Y"
    CYLINDRICAL_Z = "Cylindrical Z"
    SIDE_BY_SIDE = "Side by Side"
