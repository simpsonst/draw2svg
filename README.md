# Draw2SVG Converter

`draw2svg` is a RISC OS command to convert RISC OS Draw files to [Scalable Vector Graphics (SVGs)](http://www.w3.org/SVG/).

This is a RISC OS program for converting RISC OS Drawfiles to Scalable Vector Graphics (SVG) files.
It should be 32-bit ready.

Contact [Steven Simpson](https://github.com/simpsonst) for discussion of this program.
For further updates, see [the website](https://www.lancaster.ac.uk/~simpsons/software/pkg-draw2svg).


# Installation from RISC OS binary

Copy the supplied `!Boot` directory over your own `!Boot`.
It adds a file to `!Boot.Library` so that you can use `draw2svg` at the command line.


## Files

* `!Boot`
  * `Library`
    * `draw2svg`   &mdash;  command-line program
* `Draw2SVG`
  * `Docs`
    * `LICENSE`    &mdash;  GNU GPL
    * `HISTORY`    &mdash;  Bug fixes and stuff
    * `VERSION`    &mdash;  Version number
  * `Source`
    * `draw2svg/c` &mdash;  old C source code
    * `c.*`, `h.*`   &mdash;  new C source code
  * `Extras`
    * `draw2svg`   &mdash;  Configuration for !ComndCTRL
    * `Sprites`    &mdash;  Dodgy low-res sprites for SVG type
  * `Tests`        &mdash;  Some tests

Only the `!Boot.Library.draw2svg` binary is functionally necessary.


## File type

You might wish to add these to your boot sequence:

    Set File$Type_AAD SVG
    DOSMap SVG SVG


# Source dependencies

The code is known to compile with [GCCSDK](https://gccsdk.riscos.info/).
[CSWIs](https://github.com/simpsonst/cswis) is required to provide the symbolic SWI names.
`lynx` and `markdown` commands are required to build the plain-text version of this documentation.


# Installation from source

The source is meant to be cross-compiled from Linux, using GCCSDK, for example.
It ultimately builds a ZIP which contains the directory structure for installation on RISC OS.

The makefile attempts to read from `config.mk` in the same directory, then from `draw2svg-env.mk` in the search path (which can be set with `-I` on the command line, or by setting the environment variable `MAKEFLAGS`).
If you have GCCSDK installed in `~/.local/opt/gccsdk`, and the dependencies compiled and installed in `~/.local/opt/riscos-devel`, you could put this in `config.mk`:

    RISCOS_PREFIX=$(HOME)/.local/opt/riscos-devel
    GCCSDK=$(HOME)/.local/opt/gccsdk
    
    CPPFLAGS += -static
    APPTYPE=ff8
    APPTYPE=e1f
    
    APPCP=$(GCCSDK)/cross/arm-unknown-riscos/bin/elf2aif
    ELF2AIF=$(APPCP)
    CMHG=$(GCCSDK)/cross/bin/cmunge
    CC=$(GCCSDK)/cross/bin/arm-unknown-riscos-gcc
    CXX=$(GCCSDK)/cross/bin/arm-unknown-riscos-g++
    AR=$(GCCSDK)/cross/bin/arm-unknown-riscos-ar
    AS=$(GCCSDK)/cross/bin/arm-unknown-riscos-as
    RANLIB=$(GCCSDK)/cross/bin/arm-unknown-riscos-ranlib
    
    ARCHIVE_DIR=$(RISCOS_PREFIX)/export
    RISCOS_ZIP=$(GCCSDK)/env/bin/zip -,
    UNZIP=$(GCCSDK)/env/bin/unzip
    
    PREFIX=$(RISCOS_PREFIX)
    LDFLAGS += -L$(RISCOS_PREFIX)/lib
    CPPFLAGS += -I$(RISCOS_PREFIX)/include

Then you can compile with:

    make

This builds `out/draw2svg-riscos.zip`, which can be unpacked on RISC OS for installation.

If you do this:

    make install

It will install the files directly in `$(PREFIX)/apps`.
You can use this to make the program directly available to an emulator or physical machine that can access that directory.


# Usage

Either use the command line:

    draw2svg [options] <infile> <outfile>

or use the [`!ComndCTRL`](https://armclub.org.uk/free/commandctrl.zip) configuration file `Draw2SVG.Extras.draw2svg` for a WIMP front-end.

Options include:

* `--help` of `-h` &ndash; Display options.

* `--thin <width><units>` &ndash; Set width of thin, non-transparent lines.
  The default is one draw unit.

* `--units <units>` or `-u <units>` &ndash; Set units for output.
  `in` is the default.

* `-/+xy` &ndash; Enable/disable `x` and `y` attributes for top-level `<svg>` element.
  Disabled by default.

* `-/+g` &ndash; Enable/disable translation of Drawfile groups into `<g>` elements.
  Enabled by default.

* `-z` Set absolute sizing.
  `width` and `height` attributes are included on the root `<svg>` element using the preferred units (`-u`).

* `+z` &ndash; Disable sizing.
  width and height attributes are omitted from the root `<svg>` element.

* `--pcsize` &ndash; Set percentage sizing.
  `width` and `height` attributes are included on the root `<svg>` element expressed as percentages.
  The longer dimension will be 100%, and `width:height` expresses the aspect ratio.
  This is the default.

* `-/+it` &ndash; Enable/disable check input for Drawfile type (hex `AFF`, `Drawfile`).
  Enabled by default.

* `-/+ot` &ndash; Enable/disable setting of output type (hex `AAD`, `SVG`).
  Enabled by default.

* `-bg #rrggbb` or `+bg` &ndash; Set/cancel background colour.
  Cancelled by default.

* `--scale x[,y]` or `--fit *|width,*|height` or `--no-fit|--no-scale|--fit *,*` &ndash; Set scale factors or fit to width/height/both, or reset.
  `1,1` by default.

* `--margin width[,height]` or `--no-margin` &ndash; Set/cancel margin.
      
`<units>` are `pt` (points), `in` (inches), `mm` (millimetres), or `cm` (centimetres).

Scaling is performed first, then the margin is added, then the background is added.


# Features

Converts:

* path objects to SVG paths (with caps)
* text objects to SVG paths
* sprite objects to SVG rectangles
  * just replace `<rect style="fill: #777"` with `<image xlink:href="your image URI"`


# To do

* Convert text area objects.

* Use [`<foreignObject>`](http://www.w3.org/TR/SVG11/extend.html#ForeignObjectElement) to represent multiline text areas.

* Apply the kerning property according to settingsm in the Drawfile.
  Set to `auto` for 'on' and `0` for 'off'.

* Set `textLength` attribute to actual length of the text object.

* Extract sprite and JPEG objects, converting sprites to PNGs.
  Ideas for image handling:

  * For all images, keep track of identical images in the same document that have already been converted.
    Declare or identify the first, and make evrything else a `<use>`.

  * For small images, compute the data: URI.

  * Try converting indexed images with few colours, and preferably only two alpha levels, to a path using the edges library.

  * Permit a configurable stack of index databases which map images to URIs.
    Use a hash table to search quickly.

  * Convert remaining images, and place in a subdirectory.
