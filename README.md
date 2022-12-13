# NPPTextFX2
TextFX2 is a Notepad++ plugin which performs a variety of common conversions on selected text.

The original project has been dead since 2008. Now Notepad++ has started to block the plugin with version 8.4.3, so that it is no longer loaded.

So I grabbed the source code with the aim to bypass the blocking. But in the process I also made some cosmetic changes that bothered me:

- Complete removal of HTML Tidy
- Removal of no longer needed or working menus
- Updates for current Scintilla
- 64 bit version is unsupported

This code can be build on Visual Studio 2022 (Community Edition).
The 64 bit version of the plugin can be compiled, but the functions do not work in large parts.

Original source code for version 0.25 (dated 2008) can be fround from here:
https://sourceforge.net/projects/npp-plugins/files/TextFX/.


## Installation instructions:

A Notepad++ plugin must always be located in a separate folder with the same name as the DLL, otherwise it will not be loaded.

Examples:

- \Notepad++\plugins\NppTextFX64\NppTextFX64.dll (64 bit version)
- \Notepad++\plugins\NppTextFX\NppTextFX.dll (32 bit version)

You can also rename the 64 bit plugin to NppTextFX.dll, then the folder name must be the same as for the 32 bit plugin.
