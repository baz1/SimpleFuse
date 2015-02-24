# SimpleFuse

## Presentation

SimpleFuse - alias Simplified Filesystem in Userspace - was created to provide a much simpler interface than [FUSE](http://fuse.sourceforge.net/) for developers.

This simplification does not come without its drawbacks however: many UNIX features are not supported in this library.
Such features include file ownership and special files.
Only two file types are allowed here (compared to seven in today's Linux): directories and regular files.
Even though ownership has also been removed, access rights (read, write and execute) are still handled.
Nevertheless, this limitation is not so bad, since we do not usually need such features for ordinary usage (mainly data storage).
Also, although symbolic links are disabled, hard links are possible.

In addition to this reduction of features, the simplification of the process also comes from the type of programming interface that is provided.
This programming interface is designed to be integrated into a Qt program.

## Instructions

Add the lines `DEFINES += "_FILE_OFFSET_BITS=64"` and `LIBS += -lfuse` to your .pro file.
Implement your own class inheriting QSimpleFuse and write your own version of the virtual functions.
You may inspire yourself from the examples given in the corresponding directory.

