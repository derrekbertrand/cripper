cripper
=======

A crappy tile ripper.

Selects a raster image (BMP or PNG) constructed of tiles and tries to find the unique ones using horrible linear algorithms.

Usage
-----

    cripper

Assumes 8x8 tiles and outputs "output.png"

    cripper [-tw x] [-o y]

Works with tiles x pixels in size. Outputs to file named y. It is best power of two sizes; most games work in 8, 16, or 32 pixel combo sizes.

Requirements
------------

Allegro 5 + imag addon, GNU Make, and a Linux box.


License
-------
Copyright (C) 2013  Derrek Bertrand

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
