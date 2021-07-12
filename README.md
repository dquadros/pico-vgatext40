# Pico-VgaText40
Generates 15x40 Alphanumeric Color Text with the Raspberry Pi Pico

The idea here is to emulate the CGA/EGA/VGA text modes, by having
one attribute byte for each alphanumeric character. Attribute defines
the foreground and background colors (that is the color of the lit and
unlit dots).

The original plan was to generate 30 lines of 80 columns, but the
code is not fast enough for that (yet?).

The basic structure of the code is copied from Raspberry examples,
there are a few things that are not used so far.
