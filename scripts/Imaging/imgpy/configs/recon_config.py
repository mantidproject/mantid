from configs.functional_config import FunctionalConfig
from configs.preproc_config import PreProcConfig
from configs.postproc_config import PostProcConfig


class ReconstructionConfig(object):
    """
    Full configuration (pre-proc + tool/algorithm + post-proc.
    """

    def __init__(self, functional_config, preproc_config, postproc_config):
        # just some sanity checks
        if not isinstance(functional_config, FunctionalConfig):
            raise ValueError(
                "Functional config is invalid type. The script might be corrupted."
            )

        if not isinstance(preproc_config, PreProcConfig):
            raise ValueError(
                "Preproc config is invalid type. The script might be corrupted."
            )

        if not isinstance(postproc_config, PostProcConfig):
            raise ValueError(
                "Postproc config is invalid type. The script might be corrupted."
            )

        self.func = functional_config
        self.pre = preproc_config
        self.post = postproc_config

        # THIS MUST BE THE LAST THING THIS FUNCTION DOES
        self.handle_special_arguments()

    def handle_special_arguments(self):
        if not self.func.input_path:
            raise ValueError(
                "Cannot run a reconstruction without setting the input path")

        if (self.func.save_preproc or self.func.convert or
                self.func.aggregate) and not self.func.output_path:
            raise ValueError(
                "An option was specified that requires an output directory, but no output directory was given!\n\
                The options that require output directory are:\n\
                -s/--save-preproc, --convert, --aggregate")

        if self.func.cors is None \
                and not self.func.only_preproc \
                and not self.func.imopr \
                and not self.func.aggregate\
                and not self.func.convert\
                and not self.func.only_postproc\
                and not self.func.gui:
            raise ValueError(
                "If running a reconstruction a Center of Rotation MUST be provided"
            )

        # if the reconstruction is ran on already cropped images, then no ROI should be provided
        if self.func.cors and self.pre.region_of_interest:
            # the COR is going to be related to the full image
            # as we are going to be cropping it, we subtract the crop
            left = self.pre.region_of_interest[0]

            # subtract from all the cors
            self.func.cors = map(lambda cor: cor - left, self.func.cors)

        # if we're doing only postprocessing then we should skip pre-processing
        if self.func.only_postproc:
            self.func.reuse_preproc = True

    def __str__(self):
        return str(self.func) + str(self.pre) + str(self.post)

    @staticmethod
    def empty_init():
        """
        Create and return a ReconstructionConfig with all the default values.

        This function is provided here to create a config with the defaults, 
        but not go through the hassle of importing every single config and 
        then constructing it manually. 
        This method does that for you!
        """
        # workaround to all the checks we've done
        func = FunctionalConfig()
        func.input_path = '~/temp/'
        func.imopr = 'something'

        return ReconstructionConfig(func, PreProcConfig(), PostProcConfig())
