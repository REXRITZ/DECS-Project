# DECS-Project
## Abstract
Design a file-sharing service that can be accessed by multiple users for read, write, create and delete. Users connect to the service via a client process and can browse, checkout, commit, add, delete, writelock files to the repository.

On checkout, a local copy is created on the client's machine. Changes have to be committed, and each commit has to be synchronized across users using the writelock over the network.

The client can list all the files available on the server with the number of current readers/writers. A commit operation is required to update a file on the server.
Clients can read shared files simultaneously, but only one user can write at a time, etc.

A write to a file should notify clients who have checked out the file about the update event.

Changes to a file can be pushed to or pulled by the client — this is a design choice to explore tradeoffs. Further, is the entire file pushed, or just the delta is also a design choice.
