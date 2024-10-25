# A python script generated to perform a simultaneous fit
from mantid.api import AnalysisDataService
from mantid.simpleapi import Fit
import matplotlib.pyplot as plt

# List of tuples [ (workspace_name, workspace_index, start_x, end_x) ]
input_data = [
    ("Name1", 0, 0.500000, 1.500000),
    ("Name2", 1, 0.600000, 1.600000)
]

# Fit function as a string
function = \
    "composite=MultiDomainFunction,NumDeriv=true;" \
    "name=GausOsc,A=0.2,Sigma=0.2,Frequency=1,Phi=0,$domains=i;" \
    "name=GausOsc,A=0.2,Sigma=0.2,Frequency=1,Phi=0,$domains=i"

# Fitting options
max_iterations = 500
minimizer = "Levenberg-Marquardt"
cost_function = "Least squares"
evaluation_type = "CentrePoint"
output_base_name = "Output_Fit"

# Perform a simultaneous fit
input_workspaces = [domain[0] for domain in input_data]
domain_data = [domain[1:] for domain in input_data]

fit_output = \
    Fit(Function=function,
        InputWorkspace=input_workspaces[0], WorkspaceIndex=domain_data[0][0], StartX=domain_data[0][1], EndX=domain_data[0][2],
        InputWorkspace_1=input_workspaces[1], WorkspaceIndex_1=domain_data[1][0], StartX_1=domain_data[1][1], EndX_1=domain_data[1][2],
        MaxIterations=max_iterations, Minimizer=minimizer, CostFunction=cost_function,
        EvaluationType=evaluation_type, Output=output_base_name)

output_workspaces = []
for i in range(len(input_workspaces)):
    output_workspaces.append(output_base_name + "_Workspace_" + str(i))

# Plot the results of the fit
fig, axes = plt.subplots(nrows=2,
                         ncols=len(output_workspaces),
                         sharex=True,
                         gridspec_kw={"height_ratios": [2, 1]},
                         subplot_kw={"projection": "mantid"})

for i, workspace_name in enumerate(output_workspaces):
    workspace = AnalysisDataService.retrieve(workspace_name)
    axes[0, i].errorbar(workspace, "rs", wkspIndex=0, label="Data", markersize=2)
    axes[0, i].errorbar(workspace, "b-", wkspIndex=1, label="Fit")
    axes[0, i].set_title(workspace_name)
    axes[0, i].set_xlabel("")
    axes[0, i].tick_params(axis="both", direction="in")
    axes[0, i].legend()

    axes[1, i].errorbar(workspace, "ko", wkspIndex=2, markersize=2)
    axes[1, i].set_ylabel("Difference")
    axes[1, i].tick_params(axis="both", direction="in")

fig.subplots_adjust(hspace=0)
fig.show()
