# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as mantid


def save_gda(d_spacing_group, output_path, gsas_calib_filename, grouping_scheme, raise_warning):
    if raise_warning:
        raise RuntimeWarning(
            "Could not save gda file, as GSAS calibration file was not found. It should be present at " + gsas_calib_filename
        )
        return
    input_ws = mantid.CropWorkspace(InputWorkspace=d_spacing_group, XMax=10, StoreInADS=False)
    mantid.SaveGDA(InputWorkspace=input_ws, GSASParamFile=gsas_calib_filename, GroupingScheme=grouping_scheme, OutputFilename=output_path)


def save_maud(d_spacing_group, output_path):
    for i, ws in enumerate(d_spacing_group):
        mantid.SaveFocusedXYE(
            InputWorkspace=ws, Filename=output_path, SplitFiles=False, StartAtBankNumber=i, Append=i > 0, IncludeHeader=True, Format="MAUD"
        )


def save_angles(d_spacing_group, output_path):
    mantid.SaveBankScatteringAngles(InputWorkspace=d_spacing_group, Filename=output_path)


def save_maud_calib(d_spacing_group, output_path, gsas_calib_filename, grouping_scheme, raise_warning):
    if raise_warning:
        raise RuntimeWarning(
            "Could not save MAUD calibration file, as GSAS calibration file was not found. It should be present at " + gsas_calib_filename
        )
        return
    mantid.SaveGEMMAUDParamFile(
        InputWorkspace=d_spacing_group, GSASParamFile=gsas_calib_filename, GroupingScheme=grouping_scheme, OutputFilename=output_path
    )
