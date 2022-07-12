# LCD4Linux supporting current RHEL like systems and improved AX206 DPF options

This is a fork of [MaxWiesel's lcd4linux-max](https://github.com/MaxWiesel/lcd4linux-max) to address my fustrations with trying to use that software.

The best source for information about LCD4Linux appears to be [The unoffical LCD4Linux Wiki](https://wiki.lcd4linux.tk/doku.php/start).

## Objectives

Make it easier to build this software on Rocky, Mint and OpenWRT for support of USB LCD displays and add new options in support of those displays.

My focus is using the now commonly available 3.5" USB LCD displays that are offered at many online stores as 'AIDA64' displays.  These are more correctly called AX206 displays, hence the repo name, see the warning below before buying a display for use with LCD4Linux.  

More details below.

## Current status

Now building and working on Rocky 8.6 with DPF and X11 drivers.

## Warnings

I am an embedded systems engineer more used to writing C code for bare metal systems and basic Posix tools.  The way I do things here may not be the proper way for such a project but the lack of documentation in the original project left me unable to simply build a binary I needed.  Therefore I have simply done what works for me.  If you don't like it I would refer you back to the original project.  I will try and make commits somewhat atomic so they may still be of use in other forks as a reference for potential changes.

An example of the troubles you may have is the first commit was to replace all the C files tabs with spaces.  This was because when I opened the source files with Sublime and the indentation looked terrible.  The core reason is that Sublime, in both it's default settings and my personal prefrences, has a different number of spaces that a tab represents from whatever the original project expects.  I'm sure there is a standards for this but I have only been coding since 1978 so I am yet to work out what that standard is.  However I have found if you set the tab to your prefered indentation as spaces only then all is good with the world and all editor just work.  My pedantic attitude about this is likely to break the ability to merge with the upstream fork, sorry, thems the breaks if you want to use this code.

Be aware that AIDA64 is actually Windows software, not the displays themselves.  So while LCD4Linux will work with the 3.5" AIDA64 displays I have purchased so far it will not work 5" AIDA64 displays as these are actaully HDMI.  The 3.5" USB displays are using the AX206 chipset designed for photo frames but can be hacked for general use.  All 3.5" USB displays I have purchased so far have the correct firmware for use with LCD4Linux as this appears to be what the AIDA64 software needs.  Searching online for AX206 displays yields few results so you are better searching for AIDA64 displays then filtering for USB versions only.

## Core issues addressed

- The generic install instructions don't work.  The default build system tools are ver 1.14 but the current versions in Rocky (cicia 2022) are version 1.16 so moved the build system to use that.
- Code, for AX206 display atleast, uses libusb version 0.1 where as Rocky currently uses 1.0 and gets really pissy if you try to install 0.1 so I updated the DPF source code to use 1.0 USB libraries.

## Core issues to address

- The confiuration options used by lcd4linux at runtime were originally conceived for mono text LCDs, not colour graphics LCDs.  There are bugs with some colour names, such as 'white', not working.  These can be worked around but I wish get to the bottom it so common sense conf files can be used.  Likewise I would like to be able to change the colour of bar graphs depending on the value range e.g. green below 90% and red above that.

## Building

These are the additions to the generic instructions that I needed in able to build the project.

### Rocky

This will probably apply to similar RHEL distros.

In my case I found it helped to install these packages:
- dnf install ncurses-devel ncurses
- dnf install libX11-devel
- dnf install gd-devel.x86_64
- dnf install libjpeg-turbo-devel
- dnf install usblib* 

Configure the build enviroment. To keep it simple I configured only for the DPF driver, which is the one that supports AX206 displays. From the top level of the repo directory:
- aclocal
- libtoolize --copy --force 
- autoheader
- automake --add-missing --copy --foreign 
- autoconf
- ./configure --with-drivers=DPF

You can also support the X11 driver using:
- ./configure --with-drivers=DPF,X11

## Config file information

The best documentation I have found on the configuration file is at [The unoffical LCD4Linux Wiki](https://wiki.lcd4linux.tk/doku.php/start).  Here I have added information I could not find there.

### Upstream repo

This is things that this code has inherited from the up stream repo.

#### TrueType Widget

The most notable of these is the widget class TrueType from Eric Loxat.  Having discovered this I realsied it will save me a lot of work I was planning on improving the larger font appearance with the AX206 displays, thanks Eric.

For the Widget declaration the fields appear to be:
- `class`: Must be 'Truetype'.
- `expression`: The expresion for the text to display.
- `font`: The full path the True Type font to use.
- `size`: The height of the font, not including decenders, in pixels.
- `width`: The width of the box in pixels to fit the text, not the character width, see below.
- `height`: The height of the box in pixels to fit the text, see below.
- `align`: Horizontal alignment of 'L', 'C' or 'R'.
- `center`: Doesn't appear to do anything.
- `fcolor`: Font color as RGB or RGBA.  The background is always transparent.
- `inverted`: Inverts the fcolor value e.g. '0000ff' would become 'ffff00'.
- `update`: Update time in mS, as per other widgets.
- `visible`: Visiblity,  as per other widgets.
- `debugborder`: The colour to use to show the bounding box.  If black or ommited will not be rendered.
NB: If `size` is ommited or 0 the largest font size that will fit in the `width` x `height` box will be used depending on what limited is reached first.  If the font `size` is given the text will be cropped to fit the box if it too big.  The text is always vertically center alligned, ignoring decenders.  While expermenting with layouts setting `debugborder` to 'ffffff' can be handy.

An example:
```
Widget Debug {
    class 'Truetype'
    expression 'Test'
    font '/usr/share/fonts/gnu-free/FreeSans.ttf'
    width 60
    height 80
    fcolor colour_white
}
```

For the layout section the same X Y position format used by images should be used.

### REpo specific additions

Here is where I will document any extensions I make to the code.
