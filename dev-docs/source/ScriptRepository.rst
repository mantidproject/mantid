.. _scriptrepository:

=================
Script Repository
=================

This page describes the implementation of the :ref:`script repository <WorkbenchScriptRepository>`
from a developer perspective.

Purpose
-------

The script repository was designed to enable users to easily share scripts with
other users in the Mantid community without requiring knowledge
or use of an additional system.

The design criteria from the user perspective were:

- ability to list available content without having to download the whole repository
- ability to download just the content selected by the user
- ability to publish content, along with identifying information:

  - ``Author Name``
  - ``Author Email``

- ability to delete content authored by the user

Overview
--------

The script repository implementation is split into three parts as illustrated below:

.. figure:: /images/scriptrepository-architecture.png


1. **Frontend** - the GUI accessed by users through Workbench and described in :ref:`script repository <WorkbenchScriptRepository>`.
2. **Backend** - the server component that acts as an intermediary to the Git repository
3. **GitHub repository** - final storage location of the uploaded content.
   This is currently a `GitHub repository <github-repository_>`_.
   It can be used with standard git commands and the front/backend will respond as
   expected.

Frontend
--------

The frontend seen by users is implemented by the
`ScriptRepositoryView <https://github.com/mantidproject/mantid/blob/main/qt/widgets/common/inc/MantidQtWidgets/Common/ScriptRepositoryView.h>`__.
It implements a Qt-table-based view, with accompanying models, of the content within the `GitHub repository <github-repository_>`_.
Please see the :ref:`script repository <WorkbenchScriptRepository>` for a description of the features.

Backend
-------

The backend is a Python application that receives requests from the frontend and
processes them accordingly, interacting with the final GitHub repository store
when necessary. The source code is available `here <scriptrepository-backend-src_>`_.

It provides capabilities to:

- download an index of the repository without fetching the content itself (see below)
- fetch requested content from the repository
- upload/delete requested content from the repository

The backend requires a clone of the repository in order to push/pull content.
The cloned repository connected to the backend must be configured with an
appropriate SSH key to allow it to publish to GitHub. For the production
setup please see the `Ansible configuration <ansible-linode_>`_
(access is limited to those with server access.)

GitHub Repository
-----------------

This is the ultimate store of the script content that is uploaded.
It is a standard Git repository hosted on `GitHub <github-repository_>`_ and as
such any Git client can interact with the repository as normal. All changes will
be reflected by the backend and ultimately the frontend.

The repository index is created and maintained within repository as a `JSON file <github-repository-index_>`_.
A `GitHub Action <github-repository-action_>`_ ensures that the index is updated each time
a push to the default branch occurs.


.. _scriptrepository-backend-src: https://github.com/mantidproject/scriptrepository-backend
.. _ansible-linode: https://github.com/mantidproject/ansible-linode
.. _github-repository: https://github.com/mantidproject/scriptrepository
.. _github-repository-index: https://github.com/mantidproject/scriptrepository/blob/master/repository.json
.. _github-repository-action: https://github.com/mantidproject/scriptrepository/blob/master/.github/workflows/indexing.yml
