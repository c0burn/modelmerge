+==============================+
| Modelmerge v1.0              |
| Release date: 4/2/18         |
| by Michael Coburn ("c0burn") |
| michael.s.coburn'gmail.com   |
+==============================+

===========
Description
===========

This is a command line based tool which can combine two Quake 1 Models (".mdl") into a single model file.
It was designed to aid in the creation of visible weapons and such, but it can be used for other purposes.

The source code is available in a single .c file which is included. It should compile in MSVC on Windows and GCC on *nix. C99 features are expected.

Thanks to id software for Quake and the source (https://github.com/id-Software/Quake), David Henry for his great website and description of the MDL format (http://tfc.duke.free.fr/) and Spike, creator of FTEQW for his help on irc (http://fte.triptohell.info/)

=====
Usage
=====

	modelmerge <input1.mdl> <input2.mdl> <output.mdl> (optional)

For example:

	modelmergerge player.mdl g_shot2.mdl new_player.mdl

This will merge player.mdl and g_shot2.mdl into a new model, new_player.mdl

If you don't specify the 3rd argument, the tool will output to "output.mdl" in the current working directory, usually the directory the executable is in.
Please note that it will overwrite any file that already exists without asking if you wish to do so.

============================
Limitations and future plans
============================

- This tool can successfully merge models that qME cannot, for example where one model contains backfacing triangles with the onseam property and one does not. It can also correctly merge models that both don't contain any backfacing triangles. Good examples of this are capnbub's replacement player model and dwere's replacement g_* models.
- No error checking is performed. If you manage to exceed the number of skins, vertices, triangles, skin width or skin height supported by whichever engine you choose to use this tool will not warn you or stop you.
- Models containing skingroups and/or framegroups are not yet supported. If you want to merge such models, remove any skingroups and/or framegroups, merge them, then re-create the skingroups and/or framegroups again using a tool such as qME.
- All animation data (frames) from input1.mdl are kept, all animation data (frames) from input2.mdl are discarded apart from the first frame
- All skin data from input1.mdl and input2.mdl is kept - if the numbers of skins in each model don't match, the first skin is used as a placeholder in subsequent frames
- I might add support for a -scale command line parameter so the second model can be resized.

==========
Change log
==========

1.0 - 4/2/18 - first release
