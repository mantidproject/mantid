# Clone the mantid source code

- **Important**: If you have any existing non-conda mantid development environments,
  do not re-use the source and build directories for your conda environment.
  We recommend that you clone a new instance of the source and keep separate build directories
  to avoid any cmake configuration problems.

- Obtain the mantid source code by either:

  - Using git in a terminal and cloning the codebase by calling
    `git clone git@github.com:mantidproject/mantid.git` in the
    directory you want the code to clone to.
    You will need to follow this
    [Github guide](https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh)
    to configure SSH access.
    You may want to follow this [Git setup guide](https://git-scm.com/book/en/v2/Getting-Started-First-Time-Git-Setup)
    to configure your git environment.
  - Or using [GitKraken](https://www.gitkraken.com/).
