# NPPTextFX2
TextFX2 is a Notepad++ plugin which performs a variety of common conversions on selected text.

The original project has been dead since 2008. Now Notepad++ has started to block the plugin without any notice with version 8.4.3, so that it is no longer loaded.

So I grabbed the source code with the aim to bypass the blocking. But in the process I also made some cosmetic changes that bothered me.

- Complete removal of HTML Tidy
- Removal of no longer needed or working menus

The plugin is now loaded by Notepad++ again and is also compatible with the current Scintilla.

This code can be build on Visual Studio 2022 (Community Edition).
The 64 bit version of the plugin can be compiled, but the functions do not work in large parts.

Original source code for version 0.25 (dated 2008) can be fround from here:
https://sourceforge.net/projects/npp-plugins/files/TextFX/.
