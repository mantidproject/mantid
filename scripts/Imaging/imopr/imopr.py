from __future__ import (absolute_import, division, print_function)


def execute(config):
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
