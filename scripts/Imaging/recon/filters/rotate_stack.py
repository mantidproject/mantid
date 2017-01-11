def _rotate_imgs(self, data, cfg):
    """
    NOTE: ONLY WORKS FOR SQUARE IMAGES
    Rotate every image of a stack

    @param data :: image stack as a 3d numpy array
    @param cfg :: pre-processing configuration

    Returns :: rotated data (stack of images)
    """

    self._check_data_stack(data)
    self._debug_print_memory_usage_linux("before rotation.")

    for idx in range(0, data.shape[0]):
        # rot90 rotates counterclockwise; cfg.rotation rotates clockwise
        counterclock_rotations = 4 - cfg.rotation
        data[idx, :, :] = np.rot90(data[idx, :, :], counterclock_rotations)

    self._debug_print_memory_usage_linux("after rotation.")

    return data


def execute(data, cfg, white=None, dark=None):
    """
    Rotates a stack (sample, white and dark images).
    This funciton is usually used on the whole picture, which is a square.
    If the picture is cropped first, the ROI coordinates
    have to be adjusted separately to be pointing at the NON ROTATED image!

    @param data :: stack of sample images
    @param cfg :: pre-processing configuration
    @param white :: stack of white images
    @param white :: stack of dark images

    Returns :: rotated images
    """
    if not cfg or not isinstance(cfg, tomocfg.PreProcConfig):
        raise ValueError(
            "Cannot rotate images without a valid pre-processing configuration"
        )

    if not cfg.rotation or cfg.rotation < 0:
        self.tomo_print(
            " * Note: NOT rotating the input images.", verbosity=2)
        return data, white, dark

    self.tomo_print_timed_start(
        " * Starting rotation step ({0} degrees clockwise), with pixel data type: {1}...".
            format(cfg.rotation * 90, data.dtype))

    data = self._rotate_imgs(data, cfg)
    if white:
        white = self._rotate_imgs(white, cfg)
    if dark:
        dark = self._rotate_imgs(dark, cfg)

    self.tomo_print_timed_stop(
        " * Finished rotation step ({0} degrees clockwise), with pixel data type: {1}.".
            format(cfg.rotation * 90, data.dtype))

    return (data, white, dark)