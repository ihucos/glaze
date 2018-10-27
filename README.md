# glaze

Distribute containers/applications

## What is glaze?
It's little code but actually many things. You can think of it as a
distribution agnostic way to package applications for linux. The only
requirement on the operating system is a `tar` binary. Another way to view it
is as a standalone container runner or as a way to deploy containers. Let's
look at an example.

## Example
In the examples folder, there is a gimp package, to install it run.
```
curl -fL https://github.com/ihucos/glaze/raw/master/examples/gimp.tar.gz | sudo tar -xzf - -C /
```
Congratulation, you installed gimp. Try to run it
```
gimp
```
As already written, this should work on any linux distribution. Except from the
tar binary to extract it, there are no dependencies, not even a shell is
needed.

## How does this work?
Unpacking this examples basically extract the following:
- /usr/local/bin/gimp
- /opt/gimp/bin/*
- /opt/gimp/rootfs/*

Basically it is just a root filesystem where any user can chroot into via a
statically compiled binary. Files in /opt/gimp/bin are all hard linked, to a
small (~50kb) binary. All programns from the root file system can be accessed
via theese binaries. For example.
```
$ /opt/gimp/sh
# exit
$ /opt/gimp/rm ~/Downloads/mymovie.mov
$ /opt/gimp/bin/gimp --version
GNU Image Manipulation Program version 2.8.22


```
You can even install new programs accessing the container's package manager and
stuff like that. Things like the home folder is automatically mounted. So it's
a little bit like merging the container into your host operating system. The
/usr/local/bin/gimp file is just a symlink to /opt/gimp/bin/gimp.

## How to package
Check out the project
```
$ git clone https://github.com/ihucos/glaze.git
```
Build or download a container, possibly installing your application there.
```
$ (cd /tmp; wget http://dl-cdn.alpinelinux.org/alpine/v3.8/releases/x86_64/alpine-minirootfs-3.8.1-x86_64.tar.gz)
```
Create the distributable glazed tar packaged
```
$ ./glaze.sh /tmp/alpine-minirootfs-3.8.1-x86_64.tar.gz /tmp/alpine.glazed.tar.gz alpine zcat
```

## Status
Concept is nice, needs cleanup, currently prototype charachter.
