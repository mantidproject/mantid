# Windows Subsystem for Linux (WSL2)

::: {.contents local=""}
:::

The Windows Subsystem for Linux lets developers run a Linux environment
directly on Windows, unmodified, without the overhead of a traditional
virtual machine or dualboot setup.

## Initial Setup

Some initial setup is required on your Windows system before the Linux
environment of your choice can be installed. Before you start this
process make sure you have the latest version of Windows 10.

1.  Install [Docker Desktop for
    Windows](https://hub.docker.com/editions/community/docker-ce-desktop-windows).
    This will install WSL2 for you, and also enables us to get a centos7
    image later on.
2.  In the Windows Start menu open <span class="title-ref">Turn Windows
    features on or off</span>.
3.  Make sure <span class="title-ref">Windows System for Linux</span>
    and <span class="title-ref">Virtual Machine Platform</span> are
    ticked. Click OK.

If you now open a File Explorer and go to the
<span class="title-ref">wsl\$</span> directory. This will be the
location of your linux subsystem.

## Install a Ubuntu-18.04 Subsystem

1.  Go to the Microsoft Store and install Ubuntu 18.04. Notice that a
    new <span class="title-ref">Ubuntu-18.04</span> directory has
    appeared in the <span class="title-ref">wsl\$</span> location. This
    is your Ubuntu 18.04 subsystem.
2.  In the Windows Start menu type Ubuntu 18.04 to open its terminal. If
    it says you are missing a package then follow the link provided to
    install this package.
3.  Enter a username and password in the terminal when prompted.

You are now in an Ubuntu 18.04 terminal. Any files in the
<span class="title-ref">wsl\$Ubuntu-18.04home\<user name\></span>
directory will appear in this Ubuntu environment.

## Install a Centos7 Subsystem

1.  Open the Windows command prompt and run the follow to get the latest
    centos7 image

``` sh
docker image pull centos:centos7
```

2.  Then make a <span class="title-ref">\*.tar</span> file from this
    image, where <span class="title-ref">998e</span> is the first four
    characters of the text outputted by the
    <span class="title-ref">docker create</span> command.

``` sh
docker create -i centos:centos7 bash

docker export 998e > centos7.tar
```

3.  Finally import the file to the wsl directory:

``` sh
wsl --import Centos7 .\CentosImage\ centos7.tar
```

Notice that a new <span class="title-ref">Centos7</span> directory has
appeared in the <span class="title-ref">wsl\$</span> location. Any files
in the <span class="title-ref">wsl\$Centos7root</span> directory will
appear in this Centos7 environment.

## Running a Linux Subsystem

You can run either of these linux subsystems from the Windows command
prompt using

``` sh
wsl -d [OS]
```

where \[OS\] is replaced by <span class="title-ref">Ubuntu-18.04</span>
or <span class="title-ref">Centos7</span>.

## Cloning Mantid

Before you clone Mantid code, follow the
`getting started <GettingStarted>` instructions to install all of the
required dependencies for your linux environment.

The Mantid code can then be retrieved in the usual way using
<span class="title-ref">git clone</span> from the Ubuntu or Centos7
terminal. The first time you do this you might need to set up an [ssh
key](https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh)
for authentication.

You are now ready to CMake and build the Mantid code. Follow the Ubuntu
18.04 or Centos 7 build instructions
[here](https://developer.mantidproject.org/GettingStarted.html#linux).

## Using Graphical User Interfaces (GUI)

WSL does not currently support GUIs. In order to display a GUI from an
application running in WSL you will need to use an XServer. Failure to
do so will result in a crash upon launching
<span class="title-ref">workbench</span>.

To use an XServer:

1.  Download a compatible XServer onto the host Windows machine.
    [MobaXterm](https://mobaxterm.mobatek.net/) is recommended and has
    been confirmed to work with Mantid Workbench.
2.  Install <span class="title-ref">MobaXterm</span> and open the
    MobaXterm app upon completion. Using the
    <span class="title-ref">Settings</span> drop down menu select
    <span class="title-ref">Configure</span>. On the
    <span class="title-ref">X11</span> tab ensure that
    <span class="title-ref">X11 server display mode</span> is set to
    <span class="title-ref">Multiwindow mode</span> and that
    <span class="title-ref">X11 remote access</span> is set to
    <span class="title-ref">full</span>.
3.  The XServer should be running by default - this can be checked by
    clicking the <span class="title-ref">X server</span> icon in the top
    right hand corner of the <span class="title-ref">MobaXterm</span>
    interface.
4.  Configure the <span class="title-ref">DISPLAY</span> variable on
    WSL. The manner in which this is done differs between WSL1 and WSL2.
    To check which version you have use the command
    <span class="title-ref">wsl -l -v</span> in the command prompt on
    the host machine. If you have WSL1, it is recommended to upgrade to
    [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install).

> 1.  On WSL, use <span class="title-ref">cd</span> to navigate to your
>     home directory containing the
>     <span class="title-ref">.bashrc</span> file.
>
> 2.  Using <span class="title-ref">vi .bashrc</span> append the
>     following to the file contents:
>
>     <span class="title-ref">export DISPLAY=\$(route.exe print \| grep
>     0.0.0.0 \| head -1 \| awk '{print \$4}'):0.0</span>.
>
> 3.  Exit and re-enter WSL via the command prompt.
>
> 4.  Echo the <span class="title-ref">DISPLAY</span> variable, you
>     should see the value <span class="title-ref">\<host machine IP
>     Address\>:0.0</span>. You can compare this output IP address to
>     that output when running <span class="title-ref">ipconfig
>     /all</span> through the command line on the host machine.

## Tips

- Make sure you install
  [devtoolset-7](https://developer.mantidproject.org/BuildingWithCMake.html#from-the-command-line)
  for Centos 7 as described in the provided link before CMake and build.
- It might also be necessary to install some addition packages for
  Ubuntu 18.04, including <span class="title-ref">libnexus0-dev</span>.

## Troubleshooting

If your WSL stops responding to commands, disabling and re-enabling it
might be the best solution. To do this, go to ‘Control Panel’, then
‘Programs’, then ‘Turn Windows features on or off,’ and request
administrator access for your laptop. Uncheck ‘Windows Subsystem for
Linux’ and ‘Virtual Machine Platform’. Restart your laptop after
clicking ‘Finish’ in the administrator access app. Request administrator
access again, then re-check both ‘Windows Subsystem for Linux’ and
‘Virtual Machine Platform’.

Be aware that if you maintain administrative access through a reboot,
the session will remain active, but you will not be able to execute
programs in administrator mode.
