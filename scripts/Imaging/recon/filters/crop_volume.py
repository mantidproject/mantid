def execute(data_vol, coords):
    """
    Crops a data volume by a rectangle defined by two corner
    coordinates. Crops along the z axis (outermost numpy array index)

    @param data_vol :: 3D data volume
    @param coords :: coordinates of the corners that define a rectangle box (crop to this
    box, as when cropping to the regions of interest).
    Returns :: cropped data volume
    """
    # if nothing is provided make the user aware
    if not isinstance(coords, list) or 4 != len(coords):
        raise ValueError(
            "Wrong coordinates object when trying to crop: {0}".format(coords))
    elif not isinstance(data_vol, np.ndarray) or 3 != len(data_vol.shape):
        raise ValueError("Wrong data volume when trying to crop: {0}".format(
            data_vol))
    # move into named variables for ease of use
    left = coords[0]
    top = coords[1]
    right = coords[2]
    bottom = coords[3]

    cropped_data = None
    if not any(coords) or top > bottom or left > right:
        # skip if for example: 0, 0, 0, 0 (empty selection)
        print(" ! No coordinates given, not cropping the images")
        return data_vol
    elif not all(isinstance(crd, int) for crd in coords):
        raise ValueError(
            "Cannot use non-integer coordinates to crop images. Got "
            "these coordinates: {0}".format(coords))
    else:
        cropped_data = data_vol[:, top:bottom, left:right]

    return cropped_data