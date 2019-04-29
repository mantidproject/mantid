.. _JenkinsConfiguration:

=====================
Jenkins Configuration
=====================

.. contents::
  :local:

Summary
#######

Mantid uses the `Jenkins <https://jenkins.io/>`__ automation server to
support the continuous integration requirements of Mantid. This document
aims to describe the general setup of the system.

Introduction
############

Jenkins works on a 'master->slave' principle. The master node is
responsible for orchestrating jobs and managing the slave nodes, where the
work is actually performed. The master node is located at
http://builds.mantidproject.org and each facility is responsible for providing
hardware to act as slaves for the various required configurations.

General Setup
#############

The master node is set to a fixed TCP port for JNLP slave agents under
http://builds.mantidproject.org/configureSecurity.

The anonymous jenkins user has the following rights: Overall Read,
Slave Connect, Job Read, View Read.

Setting up a New Slave
######################

Machine Setup
-------------

Set up a local ``builder`` account that will be used by the slave.

Install the :ref:`required prerequisites <GettingStarted>` for the relevant OS.

.. note::
   For Windows the `Command line Visual C++ build tools <https://visualstudio.microsoft.com/downloads/>`__
   may be used in place of a full Visual Studio install from version 2017 onwards (the 2015 tools contain a broken `vcvarsall.bat`). The same
   options should be used as for the full install.

Windows
-------

* Ensure that the location of ``msbuild.exe`` (``C:\Windows\Microsoft.NET\Framework64\v4.0.30319``) is on the ``PATH``

Slave Connection
^^^^^^^^^^^^^^^^

There are following additional steps required to be able to connect a
Windows slave using JNLP as a Windows service :

#. Setup the slave on the master node using "New Node" under
   http://builds.mantidproject.org/computer/. If "New Node" is not visible
   then you do not have the required permissions - ask an admin for help. It is
   recommended that you copy from an existing node of a similar type.
#. Once configured on the master, remote desktop to the slave, open a browser and connect to the webpage of the
   slave, .e.g. http://builds.mantidproject.org/computer/ornl-pc73896/
#. Click on the **connect** button to launch the JNLP client.
#. Once the client is launched, you can select "Install as Windows
   Service" from the clients menu. If you have a proxy then see the
   section below for further configuration steps.
#. Once installed change the recovery behaviour for
   the service. Do this by right-clicking on the Jenkins
   service->Properties->Recovery. Change the first, second, subsequent
   failures to restart the service.
#. Change the account that the service uses to log on by right-clicking
   on the Jenkins service->Properties->Log On and enter the details in
   the builder account
#. Change the "Startup type:" of the service to be "Automatic (Delayed Start)"
#. Ensure the msbuild directory is on the ``PATH``
#. Finally reboot the slave (this is the easiest way for the Jenkins to
   start trying to reconnect to it)

Note that changes to ``PATH`` require restarting the Jenkins service in
order to be reflected in the build environment.

Connecting Through a Proxy Server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is a little more tricky to windows slaves connected through a
proxy.To do this you must modify the java arguments that are used to
start the ``jenkins-slave`` process. Once the "Install as a Windows
Service" has completed you should

#. Find a directory on the machine such as ``C:\Jenkins``` or whatever
   was configured in the slave config.
#. Open the ``jenkins-slave.xml`` file
#. Edit the tag and add ``-Dhttp.proxyHost=PROXYHOST``
   ``-Dhttp.proxyPort=PROXYPORT`` to the list
#. Save the file and restart the service (or machine)


Linux
-----

Install an ssh server, ``ccache``, ``curl`` and ``xvfb``.

From the ``builder`` account run ``ccache --max-size=20G``.

Any machines acting as performance test servers will require ``mysqldb`` to be installed.

Ubuntu
^^^^^^

Configure `automatic security updates <https://help.ubuntu.com/community/AutomaticSecurityUpdates>`__.

Install ``gdebi-core`` package to allow installing ``.deb`` files.

The ``builder`` account must be setup to be able to run ``gdebi`` non-interactively. Use ``visudo`` to add the following
exception got ``builder``::

    # Allow no password for gdebi
    builder       ALL=(ALL)NOPASSWD:/usr/bin/gdebi, /usr/bin/dpkg
    ## Disable tty requirement for gdebi and dpkg command
    Defaults!/usr/bin/gdebi !requiretty
    Defaults!/usr/bin/dpkg  !requiretty

Red Hat
^^^^^^^

The ``builder`` account must be setup to be able to run ``yum`` non-interactively. Use ``visudo`` to add the following
exception got ``builder``::

    ## Allow no password for yum
    builder       ALL = NOPASSWD: /usr/bin/yum,/bin/rpm
    ## Disable tty requirement for yum command
    Defaults!/bin/rpm       !requiretty
    Defaults!/usr/bin/yum       !requiretty

Mac OS
------

Enable `SSH ("Remote Login") and VNC ("Remote Management") <https://apple.stackexchange.com/a/73919>`__.  If you have
connection issues from a non-OS X client then try adjusting your color depth settings (True Color 32bpp works on Remmina).

Install ``cppcheck`` from brew.

The ``builder`` account must be setup to be able to run ``gdebi`` non-interactively. Use ``visudo`` to add the following
exception got ``builder``::


    # Allow builder to install packages without a password
    builder  ALL=(ALL)NOPASSWD:/usr/sbin/installer, /bin/rm
    # Disable tty requirement
    Defaults!/usr/sbin/installer    !requiretty
    Defaults!/bin/rm        !requiretty

In order to run the MantidPlot tests, which require a connection to the windowing system, the user that is running the jenkins slave must
have logged in. This is most easily done by VNC - connect, log in,
then disconnect. If you see errors such as::

    _RegisterApplication(), FAILED TO establish the default connection to the WindowServer,
    _CGSDefaultConnection() is NULL.

then no one is logged in to the system.

Linux/Mac Connection Notes
--------------------------

The jenkins JNLP connections are maintained by a crontab entry. The
script is in the `mantid repository
<https://github.com/mantidproject/mantid/blob/master/buildconfig/Jenkins/jenkins-slave.sh>`__.

The comments at the top describe a typical crontab entry for the script. This needs to be manually set for each slave. Ensure the script is
marked executable after downloading it. Also ensure the entry in the crontab
has the correct ``PATH`` setting (by default cron uses a reduced ``PATH`` entry). On macOS ``latex`` and ``sysctl``
should be available.

Post-Connection Setup - All Systems
-----------------------------------

Ensure the new machine is added to the relevant `ParaView build job <http://builds.mantidproject.org/view/ParaView/>`__ and build
ParaView. Set the ``PARAVIEW_DIR`` & ``PARAVIEW_NEXT_DIR`` variables
(it's easiest to just look at the configuration for one of the other
nodes of a similar type.

Misc Groovy Scripts
###################

The following is a collection of groovy scripts that can be run either
at http://builds.mantidproject.org/script (for master node) or on a
given node, e.g `isis-mantidx3 <http://builds.mantidproject.org/computer/isis-mantidlx3/script>`__.
You must have admin privileges to run them.

https://github.com/jenkinsci/jenkins-scripts/tree/master/scriptler was helpful for coming up with some of these.

Print the Value of an Environment Variable on All Nodes
-------------------------------------------------------

.. code-block:: groovy

    import jenkins.model.*
    import hudson.model.*
    import hudson.slaves.*

    VARIABLE_NAME = "PARAVIEW_DIR"

    nodes = Jenkins.instance.getNodes()
    println("Displaying values of " + VARIABLE_NAME + " on all nodes")
    println()
    for(node in nodes) {
      node_props = node.nodeProperties.getAll(hudson.slaves.EnvironmentVariablesNodeProperty.class)
      if(node_props.size() == 1) {
      env_vars = node_props[0].getEnvVars()
      if(env_vars.containsKey(VARIABLE_NAME)) {
      pv_dir = env_vars.get(VARIABLE_NAME, "")
      } else {
      pv_dir = VARIABLE_NAME + " not set."
      }
      println(node.getDisplayName() + ": " + pv_dir)
      } else {
      pv_dir = VARIABLE_NAME + " not set."
      }
    }

Update ParaView variables on nodes
----------------------------------

**After running this script the variables look like they are updated but
are in fact cached on the slaves so the new values don't take effect
without disconnecting and forcing each slave to reconnect**

.. code-block:: groovy

    import jenkins.model.*
    import hudson.model.*
    import hudson.slaves.*

    VARIABLE_NAME = "PARAVIEW_NEXT_DIR"
    VERSION = "ParaView-5.1.2"

    jenkins = Jenkins.instance
    nodes = jenkins.getNodes()
    println("Displaying values of " + VARIABLE_NAME + " on all nodes")
    println()
    for(node in nodes) {
      node_props = node.nodeProperties.getAll(hudson.slaves.EnvironmentVariablesNodeProperty.class)
      if(node_props.size() == 1) {
      env_vars = node_props[0].getEnvVars()
      if(env_vars.containsKey(VARIABLE_NAME)) {
        def pv_dir = node.createPath(env_vars.get(VARIABLE_NAME, ""));
        if(pv_dir) {
          def pv_build_dir = pv_dir.getParent();
          def pv_dir_new = pv_build_dir.child(VERSION);
          println(node.getDisplayName() + ": Updating $VARIABLE_NAME from '" + pv_dir.toString() + "' to '" + pv_dir_new.toString() + "'");
          env_vars.put(VARIABLE_NAME, pv_dir_new.toString());
        }
        else {
          println(node.getDisplayName() + " has variable set but " + env_vars.get(VARIABLE_NAME, "") + " does not exist");
        }
      } else {
        println(node.getDisplayName() + ": $VARIABLE_NAME " +  "not set.")
      }
      } else {
        println(node.getDisplayName() + ": $VARIABLE_NAME " +  "not set.")
      }
    }
    jenkins.save();

Check existence of ParaView builds
----------------------------------

.. code-block:: groovy

    import hudson.model.*

    nodes = Jenkins.instance.slaves

    PV_VERSION = "5.1.2"

    for (node in nodes) {
      FilePath root = node.getRootPath();
      if(root) {
        FilePath fp = root.getParent();
        // assume this is $HOME on osx/linux & drive: on Windows
        if(fp.toString().startsWith("C:")) {
          fp = fp.child("Builds")
        } else {
          fp = fp.child("build");
        }
        fp = fp.child("ParaView-$PV_VERSION");
        if(!fp.exists()) {
          println(node.getDisplayName() + " does not have PV 5.1.2")
        }
      }
    }

Remove directories across multiple nodes
----------------------------------------

It is advised to ensure nothing is running and pause the build queue.

Master Incremental
^^^^^^^^^^^^^^^^^^

.. code-block:: groovy

    import hudson.model.*

    nodes = Jenkins.instance.slaves

    JOBNAME = "master_incremental"


    for (node in nodes) {
      labels = ["osx-10.10-build", "rhel6-build", "rhel7-build", "ubuntu-14.04-build", "ubuntu-16.04-build", "win7"];
      for (nodeLabel in labels) {
        FilePath fp = node.createPath(node.getRootPath().toString() + File.separator + "workspace" + File.separator + JOBNAME + File.separator + "label" + File.separator + nodeLabel + File.separator + "build");
        if(fp!=null && fp.exists()) {
          println(fp.toString())
          fp.deleteRecursive()
        }
      }
    }

Pull Requests
^^^^^^^^^^^^^

.. code-block:: groovy

    import hudson.model.*

    nodes = Jenkins.instance.slaves

    JOB_PREFIX = "pull_requests-"
    suffixes = ["win7", "osx", "ubuntu", "ubuntu-python3", "rhel7"];

    for (node in nodes) {
      for (suffix in suffixes) {
        FilePath fp = node.createPath(node.getRootPath().toString() + File.separator + "workspace" + File.separator + JOB_PREFIX + suffix + File.separator +  "build");
        if(fp!=null && fp.exists()) {
          println(fp.toString())
          fp.deleteRecursive()
        }
      }
    }

Update Branches For Jobs
------------------------

.. code-block:: groovy

    import hudson.plugins.git.GitSCM
    import hudson.plugins.git.BranchSpec
    import static com.google.common.collect.Lists.newArrayList;

    def NEW_BRANCH = "*/release-next"

    // Access to the Hudson Singleton
    def jenkins = jenkins.model.Jenkins.instance;

    // Retrieve matching jobs
    def allItems = jenkins.items
    def chosenJobs = allItems.findAll{job -> job.name =- /release_/};

    println "Updating branch for chosen jobs to $NEW_BRANCH"
    println ""
    // Do work
    chosenJobs.each { job ->
        def scm = job.scm;
        if (scm instanceof GitSCM && job.name != "release_nightly_deploy" ) {
          //def newScm = scm.clone()
          println "Updating branch for " + job.name
          scm.branches = newArrayList(new BranchSpec(NEW_BRANCH))
          println "Branch for " + job.name + ": " + scm.branches
          println ""
        }
    }

List All SCM Urls
-----------------

.. code-block:: groovy

    import jenkins.model.*;
    import hudson.model.*;
    import hudson.tasks.*;
    import hudson.plugins.git.*;
    import org.eclipse.jgit.transport.RemoteConfig;
    import org.eclipse.jgit.transport.URIish;

    for(project in Hudson.instance.items) {
      scm = project.scm;
      if (scm instanceof hudson.plugins.git.GitSCM) {
        for (RemoteConfig cfg : scm.getRepositories()) {
          for (URIish uri : cfg.getURIs()) {
            println("SCM " + uri.toString() + " for project " + project);
          }
        }
      }
    }

Print All Loggers
-----------------

.. code-block:: groovy

    import java.util.logging.*;

    LogManager.getLogManager().getLoggerNames().each() {
      println "${it}";
    }

Run a Process On a Single Node
------------------------------

.. code-block:: groovy

    Process p = "cmd /c dir".execute()
    println "${p.text}"

    // kill process on windows slave
    Process p = "cmd /c Taskkill /F /IM MantidPlot.exe".execute()
    println "${p.text}"

Run a Process Across All Nodes
------------------------------

.. code-block:: groovy

    import hudson.util.RemotingDiagnostics;

    for (slave in hudson.model.Hudson.instance.slaves) {
       println slave.name;
       // is it connected?
       if(slave.getChannel()) {
        println RemotingDiagnostics.executeGroovy("println \"ls\".execute().text", slave.getChannel());
      }
    }


Update default values for job parameters
----------------------------------------

.. code-block:: groovy

    import hudson.model.*

    def SUFFIX_VARIABLE = "PACKAGE_SUFFIX"
    def NEW_SUFFIX = "nightly"

    // Access to the Hudson Singleton
    def jenkins = jenkins.model.Jenkins.instance;

    // Retrieve matching jobs
    def chosenJobs = ["release_clean-rhel7"] //, "release_clean-ubuntu-16.04", "release_clean-ubuntu"]

    println "Updating default package suffix for chosen jobs to ${NEW_SUFFIX}"
    println ""
    // Do work
    chosenJobs.each { jobName ->
      job = jenkins.getItem(jobName)
      println(job)
      paramsDef = job.getAction(ParametersDefinitionProperty)
      params = paramsDef.getParameterDefinitions()
      params.each { it ->
        if(it.getName() == SUFFIX_VARIABLE) {
          println("Updating default value of '${SUFFIX_VARIABLE}' variable to '${NEW_SUFFIX}'")
          it.setDefaultValue(NEW_SUFFIX)
        }
      }

    }
