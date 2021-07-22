from mantid.simpleapi import \
( CreateSampleWorkspace, \
  MoveInstrumentComponent, \
  RotateInstrumentComponent, \
  AlignAndFocusPowder, \
  ConvertUnits )

# Create the basic input_workspace
input_workspace = CreateSampleWorkspace( WorkspaceType = "Event", \
                                   Function = "Powder Diffraction", \
                                   XMin = 300.0, \
                                   XMax = 16666.0, \
                                   BinWidth = 1.0, \
                                   NumBanks = 1, \
                                   BankPixelWidth = 12, \
                                   NumEvents = 10000 )

component_name = "bank1"
# Rotate the instrument
RotateInstrumentComponent( Workspace = input_workspace, \
                           ComponentName = component_name, \
                           Y = 1.0, \
                           Angle = 90.0 )

# Move the instrument
MoveInstrumentComponent( Workspace = input_workspace, \
                         ComponentName = component_name, \
                         X = 5.0, \
                         Y = -0.1, \
                         Z = 0.1, \
                         RelativePosition = False )

# empty input_workspace to be modified
filtered_workspace_removed_max = "filtered_workspace_removed_max"
unfiltered_workspace = "unfiltered_workspace"
filtered_workspace_minus_2 = "filtered_workspace_minus_2"

# Do not apply resonance limits ( this is the control). This will display all
# peaks, aligned and focused.
AlignAndFocusPowder( InputWorkspace = input_workspace, \
                     OutputWorkspace = unfiltered_workspace, \
                     Dspacing = False, \
                     Params = -0.01 )

# next, prepare another run; however, this time use the resonance filtering capability to
# remove 2 peaks from the data.
# examine a value near the top. Get the index in the vector represented
# by that by examination in workbench

max_peak = [ 3.7, 4.2 ]

# Apply resonance limits to remove the max peak from the original input_workspace:
AlignAndFocusPowder( InputWorkspace = input_workspace, \
                     OutputWorkspace = filtered_workspace_removed_max, \
                     Dspacing = False, \
                     ResonanceFilterLowerLimits = max_peak[0], \
                     ResonanceFilterUpperLimits = max_peak[1], \
                     Params = -0.01 )

# Define the ranges of the peaks to be removed:
peak_2 = [ 1.0, 1.5 ]
peak_5 = [ 2.4, 2.9 ]

# is this a sufficient array type?
lower_peak_limits = [ peak_2[0], peak_5[0] ]
upper_peak_limits = [ peak_2[1], peak_5[1] ]

# Apply resonance limits to remove peaks x and y from the original input_workspace
AlignAndFocusPowder( InputWorkspace = input_workspace, \
                     OutputWorkspace = filtered_workspace_minus_2, \
                     Dspacing = False, \
                     ResonanceFilterLowerLimits = lower_peak_limits, \
                     ResonanceFilterUpperLimits = upper_peak_limits, \
                     Params = -0.01 )

# In-place conversion of workspace x-units to Wavelength
shared_target = "Wavelength"
input_workspace = ConvertUnits( input_workspace, Target = shared_target )
filtered_workspace_removed_max = ConvertUnits( filtered_workspace_removed_max, \
                                               Target = shared_target )
unfiltered_workspace = ConvertUnits( unfiltered_workspace, Target = shared_target )
filtered_workspace_minus_2 = ConvertUnits( filtered_workspace_minus_2, Target = shared_target )
