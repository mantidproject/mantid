def execute(data_vol, block_size, method='average'):
    """
    Downscale to for example shrink 1Kx1K images to 512x512

    @param data_vol :: 3d volume to downscale
    @param block_size :: make block_size X block_size blocks to downscale
    @param method :: either 'average' (default) or 'sum' to calculate average or sum of blocks

    Returns :: downscaled volume, with the dimensions implied by block_size, and the
    same data type as the input data volume.
    """
    if not isinstance(data_vol, np.ndarray) or 3 != len(data_vol.shape):
        raise ValueError(
            "Wrong data volume when trying to crop (expected a 3d numpy array): {0}".
            format(data_vol))
    if block_size > data_vol.shape[1] or block_size > data_vol.shape[2]:
        raise ValueError(
            "Block size too large when trying to crop data volume. Block size: {0}, "
            "data dimensions: {1}".format(block_size, data_vol.shape))

    if 0 != np.mod(data_vol.shape[1], block_size) or 0 != np.mod(
            data_vol.shape[2], block_size):
        raise ValueError(
            "The block size ({0}) must be an exact integer divisor of the sizes of the "
            "x and y dimensions ({1} and {2} of the input data volume".format(
                data_vol.shape[2], data_vol.shape[1], block_size))

    supported_methods = ['average', 'sum']
    if method.lower() not in supported_methods:
        raise ValueError(
            "The method to combine pixels in blocks must be one of {0}. Got unknown "
            "value: {1}".format(supported_methods, method))

    rescaled_vol = np.zeros(
        (data_vol.shape[0], data_vol.shape[1] // block_size,
         data_vol.shape[2] // block_size),
        dtype=data_vol.dtype)
    # for block averages in every slice/image along the vertical/z axis
    tmp_shape = rescaled_vol.shape[1], block_size, rescaled_vol.shape[
        2], block_size
    for vert_slice in range(len(rescaled_vol)):
        vsl = data_vol[vert_slice, :, :]
        if 'average' == method:
            rescaled_vol[vert_slice, :, :] = vsl.reshape(tmp_shape).mean(
                -1).mean(1)
        elif 'sum' == method:
            rescaled_vol[vert_slice, :, :] = vsl.reshape(tmp_shape).mean(
                -1).mean(1)

    return rescaled_vol