from __future__ import (absolute_import, division, print_function)


def execute(config):
    """
    Execute the image operator. This allows performing operations on single images, or specified images slices.

    Currently available modes:
        - recon - do a reconstruction on a single or multiple slices, --imopr 1 recon, --imopr 10 34 recon
        - sino - visualise the sinogram for single or multiple slices, --imopr 1 sino, --imopr 10 34 sino
        - vis - just visualise the single or multiple images, --imopr 1 vis, --imopr 10 34 vis
        - cor - get the COR for a slice or multiple slices using tomopy find_center, --imopr 1 cor, --imopr 10 34 cor
        - corvo - get the COR for a slice or multiple slices using
                  tomopy find_center_vo, --imopr 1 corvo, --imopr 10 34 corvo
        - corpc - get the COR for a slice or multiple slices using
                  tomopy find_center_pc, --imopr 1 corpc, --imopr 10 34 corpc
        - corwrite - get the COR for a slice or multiple slices using
                  tomopy write_center, --imopr 1 corpc, --imopr 10 34 corwrite
        - opr - do operations on 2 images, that can be:
                sum(also +), --imopr 1 3 sum, --imopr 10 34 +
                subtract(also sub, -), --imopr 1 3 sub, --imopr 10 34 -
                divide(also div, /), --imopr 1 3 div, --imopr 10 34 /
                multiply(also mul, *), --imopr 1 3 mul, --imopr 10 34 *
                mean(also avg, x), --imopr 1 3 avg, --imopr 10 34 x

    COR functions reference: http://tomopy.readthedocs.io/en/latest/api/tomopy.recon.rotation.html

    :param config:
    :return:
    """
    # use [:] to get a copy of the list
    commands = config.func.imopr[:]

    action = get_function(commands.pop())

    # the rest is a list of indices
    indices = [int(c) for c in commands]

    from helper import Helper
    h = Helper(config)
    config.helper = h
    h.check_config_integrity(config)

    from imgdata import loader
    sample, flat, dark = loader.load_data(config, h)
    # the [:] is necessary to get the actual data and not just the nxs header
    # sample = loader.nxsread(config.func.input_path)[:]

    h.tomo_print("Data shape {0}".format(sample.shape))
    flat, dark = None, None

    from recon.recon import pre_processing
    sample, flat, dark = pre_processing(config, sample, flat, dark)
    return action(sample, flat, dark, config, indices)


def get_function(string):
    if string == "recon":
        from imopr import recon
        return recon.execute
    elif string == "sino":
        from imopr import sinogram
        return sinogram.execute
    elif string == "show" or string == "vis":
        from imopr import visualiser
        return visualiser.execute
    elif string == "cor":
        from imopr import cor
        return cor.execute
    elif string == "corvo":
        from imopr import corvo
        return corvo.execute
    elif string == "corpc":
        from imopr import corpc
        return corpc.execute
    elif string == "corwrite":
        from imopr import corwrite
        return corwrite.execute
    else:
        from imopr import opr

        # this version I like more, but it requires more work in opr.py
        # to extract the function as it cannot simply be used for a key
        # if True in [
        #         True if string in operator else False
        #         for operator in opr.get_available_operators()
        # ]:
        # the reasonable version
        if string in opr.get_available_operators():
            return opr.execute
