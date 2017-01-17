from __future__ import (absolute_import, division, print_function)


def execute(imgs_angles, config):
    """
    Transform pixel values as $- ln (Is/I0)$, where $Is$ is the pixel (intensity) value and $I0$ is a
    reference value (pixel/intensity value for open beam, or maximum in the stack or the image).

    This produces a projection image, $ p(s) = -ln\\frac{I(s)}{I(0)} $,
    with $ I(s) = I(0) e^{-\\int_0^s \\mu(x)dx} $
    where:
    $p(s)$ represents the sum of the density of objects along a line (pixel) of the beam
    I(0) initital intensity of netron beam (white images)
    I(s) neutron count measured by detector/camera

    The integral is the density along the path through objects.
    This is required for example when pixels have neutron count values.

    @param imgs_angles :: stack of images (angular projections) as 3d numpy array. Every image will be
    processed independently, using as reference intensity the maximum pixel value found across all the
    images.

    @param config :: pre-processing configuration set up for a reconstruction

    Returns :: projected data volume (image stack)
    """
    from recon.helper import Helper
    h = Helper(config)

    h.check_data_stack(imgs_angles)

    if not config.pre.line_projection:
        h.tomo_print_note("NOT applying line projection.")
        return imgs_angles

    h.pstart(
        "Starting to apply line projection on {0} images...".format(imgs_angles.shape[0]))

    imgs_angles = imgs_angles.astype('float32')
    for idx in range(0, imgs_angles.shape[0]):
        max_img = np.amax(imgs_angles[idx, :, :])
        to_log = np.true_divide(imgs_angles[idx, :, :], max_img)
        if False:
            print(
                "   Initial image max: {0}. Transformed to log scale, min: {1}, max: {2}.".
                format(max_img, np.amin(to_log), np.amax(to_log)))
        imgs_angles[idx, :, :] = -np.log(to_log + 1e-6)

    h.pstop(
        "Finished applying line projection on {0} images. ".format(imgs_angles.shape[0]))

    return imgs_angles
