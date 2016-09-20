Based on GhostFS https://github.com/raphaelsc/ghostfs by Raphael S. Carvalho
<br>
I'm very graceful :-)

### The Lion ...
```
       ////\\\              __
      ///   \\\            /,.\
     /// ^ ^ \\\.-.-.-.-.-´/ //
     \\\     ///          /  -
      \\\`-´/// ________  \
       \\\./// /     / // /
       /_/  /_/     /_//_/
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```
# lionfs (Link Over Network File System)

## What is it?

Use something like `ln -s http://website/file.ext local_file` and you will have
<br>
a local file that points to a network file. It is the purpose.
<br>
See the image:

![lionfs-example.png](https://ricardobiehl.github.io/images/lionfs-example.png)

## How it works?

Basically we do the background when you call the *read()* operation in a symlink
<br>
created by lionfs and fill the data with the content received over a http
<br>
request to the host. We don't have cache.

## How do I install it?

First you will need **libfuse** library, which probably is already present in
<br>
your system.

NOTE: Currently we aren't using *make* because I don't known how to implement it
<br>
and I'm with RSI (pt-br LER), typing with only one hand.

Later you simply type `./compile-all.sh` in source directory and if all goes
<br>
right a binary called **lionfs** will be created. See next!

## How do I use it?

You will not execute the binary directly, instead you will execute a script
<br>
called `lion-mount.sh` which is found in the source directory too. Use
<br>
`lion-mount.sh --help` for more details.

NOTE: At the moment we don't have an install script and then you need to execute
<br>
the program in the source directory. Modules are default searched in
<br>
*./modules/* or *\<ld_library_paths\>/lionfs/modules/* directories.

## Supported protocols:

Only **HTTP**

----------

Any contribution is welcome :-)
<br>
Any question mail me at <rbpoficial@gmail.com>
<br>
Any orthographic error ... Sorry by my English, it is not perfect :-(

Cheers!
