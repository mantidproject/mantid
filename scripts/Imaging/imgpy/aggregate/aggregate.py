from __future__ import (absolute_import, division, print_function)
import helper as h


def execute(config):
    """
    Aggregates images.

    Energy levels can be selected with --agregate 0 100, this selects energy levels from 0 to 100
    Angles can be selected with --aggregate-angles 0 10, this selects angles 0 to 10
    Output can be in a single folder --aggregate-single-folder-output, or in a separate folder for each angle

    :param config: The full reconstruction config
    """
    input_path = config.func.input_path
    img_format = config.func.in_format
    output_path = config.func.output_path
    single_folder = config.func.aggregate_single_folder_output

    # get the aggregation method {sum, avg}
    commands = config.func.aggregate
    agg_method = commands.pop()
    do_sanity_checks(output_path, agg_method, commands)

    # get the range of energy levels, in pairs of 2, start end
    energy_levels = [int(c) for c in commands]
    selected_indices = get_indices_for_energy_levels(energy_levels)

    # create the label that will be appended to the image file names
    if len(energy_levels) == 0:
        energies_label = ''
    else:
        energies_label = str(selected_indices[0]) + '_' + str(
            selected_indices[len(selected_indices) - 1])

    # get the angle folders
    selected_angles = config.func.aggregate_angles
    angle_folders, selected_angles = get_angle_folders(input_path, img_format,
                                                       selected_angles)
    h.tomo_print_note('Applying aggregating method {0} on angles: {1}'.format(
        agg_method, angle_folders))

    # generate the file names in each angle folder
    angle_image_paths = get_image_files_paths(input_path, angle_folders,
                                              img_format, selected_indices)
    # create an enumerator for the angle folders
    if selected_angles:
        angle_image_paths = enumerate(
            angle_image_paths, start=selected_angles[0])
    else:
        angle_image_paths = enumerate(angle_image_paths)

    do_aggregating(angle_image_paths, img_format, agg_method, energies_label,
                   single_folder, config)


def do_aggregating(angle_image_paths, img_format, agg_method, energies_label,
                   single_folder, config):
    import os
    import numpy as np
    from imgdata import loader, saver
    s = saver.Saver(config)
    parallel_load = config.func.parallel_load
    s.make_dirs_if_needed(s.get_output_path(), s._overwrite_all)

    for angle, image_paths in angle_image_paths:
        # load all the images from angle, [0] to discard flat and dark
        h.pstart("Loading data for angle {0} from path {1}".format(
            angle, os.path.dirname(image_paths[0])))
        images = loader.load(
            file_names=image_paths,
            img_format=img_format,
            parallel_load=parallel_load)[0]
        # sum or average them
        if 'sum' == agg_method:
            acc = images.sum(axis=0, dtype=np.float32)
        else:
            acc = images.mean(axis=0, dtype=np.float32)

        h.pstop("Finished loading.")

        if not single_folder:
            name = 'out_' + agg_method
            name_postfix = energies_label
            subdir = 'angle_' + agg_method + str(angle)
            custom_index = ''
        else:
            name = 'out_' + agg_method + '_' + energies_label + '_'
            name_postfix = ''
            subdir = ''
            custom_index = str(angle)

        s.save_single_image(acc.reshape(1, acc.shape[0], acc.shape[1]), subdir=subdir, name=name,
                            custom_index=custom_index, name_postfix=name_postfix, use_preproc_folder=False)


def do_sanity_checks(output_path, agg_method, commands):
    if not output_path:
        raise ValueError(
            "The flag -o/--output-path MUST be passed for this IMOPR COR mode!")

    if agg_method not in ['sum', 'avg']:
        raise ValueError(
            "Invalid method provided for --aggregate, the allowed methods are: {sum, avg}"
        )

    # the length of energy levels must be an even number
    if len(commands) % 2 != 0:
        raise ValueError(
            "The length of energy levels must be an even number, the submission format is \
            --aggregate <start> <end>... <method:{sum, avg}>: --aggregate 1 100 101 200 201 300 sum"
        )


def get_image_files_paths(input_path, angle_folders, img_format,
                          selected_indices):
    from imgdata.loader import get_file_names

    import os
    angle_image_paths = []
    for folder in angle_folders:
        angle_fullpath = os.path.join(input_path, folder)
        # get the names of all images
        all_images = get_file_names(angle_fullpath, img_format)

        if len(selected_indices) == 0:  # if none are excluded, select all
            angle_image_paths.append(all_images)
        else:  # exclude all image paths that are not selected
            current_angle_names = []
            for i, img_path in enumerate(all_images):
                if i in selected_indices:
                    current_angle_names.append(img_path)

            angle_image_paths.append(current_angle_names)

    return angle_image_paths


def get_indices_for_energy_levels(energy_levels):
    # generate the ranges for the energy levels
    selected_indices = []
    for i in range(0, len(energy_levels), 2):
        selected_indices.append(expand_index_range(energy_levels[i:i + 2]))

    # flatten the list
    selected_indices = [
        indices for sublist in selected_indices for indices in sublist
    ]
    return selected_indices


def get_angle_folders(input_path, img_format, selected_angles):
    from imgdata.loader import get_folder_names
    # get all the angles folders
    angle_folders = get_folder_names(input_path)

    # if --aggregate-angles <id1> <id2> specifies anything,
    # exclude all other angle folders
    selected_folders = None
    if selected_angles:
        selected_folders = expand_index_range(selected_angles)
        try:
            angle_folders = [angle_folders[i] for i in selected_folders]
        except IndexError:
            raise IndexError(
                "The selected angles are not present in the input directory!")
    return angle_folders, selected_folders

    # remove the files from the files list that are not in the range of index 1 -> index 2
    # what's left is the images that we WANT to sum/average


def expand_index_range(_list):
    assert len(_list) == 2, "This must only be used with <start> <end> indices"
    return range(int(_list[0]), int(_list[1]) + 1)
