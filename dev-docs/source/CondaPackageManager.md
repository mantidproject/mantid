# Conda Package Manager

```{contents}
:local:
```

Mantid uses [conda](https://docs.conda.io/en/latest/) as its package
management system. This document gives a developer overview on how we
use the conda package manager, including tips on how to debug dependency
issues, and our policy towards using `pip` packages (it is strongly
discouraged).

## Creating Environments

Creating an empty environment called `myenv`:

``` sh
conda create --name myenv
```

Creating a Python environment:

``` sh
conda create --name myenv python=3.10
```

Creating an environment from an environment file:

``` sh
conda create --name myenv -f environment_file.yml
```

Creating an environment from package:

``` sh
conda create --name myenv -c mantid mantidworkbench
```

Creating an in-place environment:

``` sh
conda create --prefix $PWD/myenv python=3.10
```

## Finding a broken dependency

The nightly pipelines can sometimes fail for obscure reasons, seemingly
unrelated to the changes made within Mantid. In this case, it is
probable that a conda dependency has updated, and the new update is
"Broken" (if it is a minor or patch update) or no longer compatible with
Mantid (if it is a major update).

To find the dependency which has changed, you can run the
`tools/Jenkins/dependency_spotter.py` script. This script takes two
Jenkins build numbers, and optionally the OS label, the pipeline name,
and the name of the file to compare. It will then output the changes in
conda package versions used in the two builds, if there are any. A few
examples on how to use it:

``` sh
python dependency_spotter.py -f 593 -s 598
python dependency_spotter.py -f 593 -s 598 -os win-64
python dependency_spotter.py -f 593 -s 598 --pipeline main_nightly
```

Another useful command for investigating the dependencies of specific
packages is [conda
search](https://docs.conda.io/projects/conda/en/latest/commands/search.html).
To find the dependencies of a package:

``` sh
conda search -i <package_name>
```

## Fixing a broken dependency

After identifying the conda dependency and version that is causing the
unwanted behaviour, there are several options we can take to fix the
issue. The following options are in order of preference:

1.  Raise an issue in the dependencies
    [feedstock](https://conda-forge.org/docs/maintainer/adding_pkgs.html#feedstock-repository-structure)
    repository with a minimum reproducible example. If appropriate,
    request that they mark the package version as "Broken". See
    [Removing broken
    packages](https://conda-forge.org/docs/maintainer/updating_pkgs.html#maint-fix-broken-packages)
    to understand this procedure.
2.  If we need a fix urgently, you can consider pinning the package in
    question. This is not an ideal solution, and so you should also open
    an issue to un-pin the package in future. When pinning a package,
    consider using the not-equals-to operator `!=x.y.z` because this
    allows the package to upgrade automatically when a new version
    arrives (which is hopefully a working version).

## Pip package policy

We have a strict policy with regards to using PyPi packages within
Mantid. This policy can be summarised as follows:

``` none
We strongly encourage PyPi dependencies be built into conda packages and uploaded to conda-forge. PyPi packages
will not be automatically installed into our Mantid conda environments, and should instead be installed by
users of the software, if required.
```

We do not want to include pip packages as dependencies in our conda
recipes because there is no guarantee of compatibility between the two
package managers. In the past, attempting to resolve compatible package
versions when two package managers are involved have caused broken
Mantid installations. Furthermore, the original motivation for moving
towards conda was so that we had a unified package manager rather than
using several different systems or mechanisms. Including pip packages in
our dependencies would be a backwards step.

The other solution we considered was installing our pip dependencies
downstream within our DAaaS workspace configuration repository. We
decided against this because it feels like bad practise to have a
formalised way of installing dependencies of a software in a way which
is completely detached. The prevailing message is this: please only use
conda packages. We provide [pip install instructions](pip-install-ref)
for users if they would like to take the risk.
