# SimpleFuse

SimpleFuse - alias Simplified Filesystem in Userspace - was created to provide a much more simple interface than [FUSE](http://fuse.sourceforge.net/) itself for the developers.

This simplification does not come without some drawbacks however: many UNIX features are not supported in this library.
Such features include file ownership and special files.
Only two file types are allowed here (compared to seven in today's Linux): directories and regular files.
Even though ownership has also been removed, access rights (read, write and execute) are still handled.
This limitation is nonetheless not so bad, since we don't usually need those features for an ordinary usage (data storage mainly).
Also, although symbolic links are disabled, hard links are possible.

In addition to this reduction of features, the simplification of the interface also comes from the type of programming interface that is provided.
No need to same the application context in a single memorized structure anymore.
The program using fuse is in fact separate from the user application. They're only connected through some memory sharing managed by this library only.
