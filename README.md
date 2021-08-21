Based on GhostFS https://github.com/raphaelsc/ghostfs by Raphael S.
Carvalho. I'm sincerely grateful for his help.

# The Lion

```
       #######
      ###   ###
     ### ^ ^ ###.-.-.-.-.´%%\
     ###  Y  ###%%%%%%%%%/ \%.
      ###`-´###%%%%%%%%%%\  %\
       #######%/%%%%%%%\%%\ ##
        \%\  \%\   /%/  /%/
._._._._##._.##_._##._.##_._._._._._
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

# lionfs (Link Over Network File System)

## What is it?

This is a small proof-of-concept of a file system which allows creating
symbolic links to remote files that are accessible with an URI.

## How it works?

This program uses an interface of the Linux Kernel for intercepting file
system operations on a given directory (the mount point). It replaces the
original operations (e.g. symlink(), read()) with its own. When user reads
a file, for example, a request is sent to the network resource and the
response is returned to the user.

## How to build it?

Dependencies: `libcurl` and `libfuse` libraries.

After installing the dependencies, run:

`make`

## How do I use it?

1. Mount the lionfs file system on an empty directory:
   `./lion-mount.sh example_directory`
2. Create a symbolic link to a network resource:
   `ln -s https://www.example.com/file local_file`

NOTE: At the moment there is no install script. The program needs to be
executed in the source directory. Modules are default searched in
`./modules/` or `<ld_library_paths>/lionfs/modules/` directories.

## Supported protocols:

See cURL's list of supported protocols.

----------

Any contribution is welcome :-)

Any question mail me at pasqualirb at gmail dot com
