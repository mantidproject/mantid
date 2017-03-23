from __future__ import (absolute_import, division, print_function)
from imgdata.loader import get_folder_names, get_file_names


def execute(config):
    """
    Converts images from one input format to another output format.

    :param config: The full ReconstructionConfig
    """

    output_dir = config.func.output_path
    image_out_format = config.func.out_format
    from imgdata.saver import Saver
    s = Saver(config)
    # fail early if invalid directory
    s.make_dirs_if_needed(s.get_output_path(), s._overwrite_all)

    from imgdata import loader
    sample, flat, dark = loader.load_data(config)

    # save out in the main output directory
    s.save(
        sample,
        output_dir,
        config.func.convert_prefix,
        config.func.swap_axes,
        image_out_format,
        zfill_len=0)
