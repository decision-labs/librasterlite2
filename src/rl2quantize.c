/*

 rl2quantize -- color quantization -- from TrueColor RGB to palette-256

 version 0.1, 2021 July 21

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the RasterLite2 library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2021
the Initial Developer. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#ifndef OMIT_LEPTONICA
#include <leptonica/allheaders.h>
#endif

RL2_DECLARE int
rl2_quantize_color (int width, int height, const unsigned char *rgb,
		    int num_colors, unsigned char **pixbuf,
		    rl2PalettePtr * palette)
{
/* 
 * color quantization based on Leptonica support 
 * 
 * method: two-pass adaptive octree color quantization with no dithering
 * 
*/
#ifndef OMIT_LEPTONICA
    unsigned char *pixels = NULL;
    unsigned char *p_pix;
    rl2PalettePtr plt = NULL;
    const unsigned char *p_rgb;
    int row;
    int col;
    int ret;
    int red;
    int green;
    int blue;
    unsigned int index;
    int n_colors;
    int out_colors;
    PIX *rgb_pix = NULL;
    PIX *palette_pix = NULL;
    PIXCMAP *cmap;

    *pixbuf = NULL;
    *palette = NULL;

/* creating and populating a Leptonica PIX for RGB input */
    rgb_pix = pixCreate (width, height, 32);
    if (rgb_pix == NULL)
	goto error;
    p_rgb = rgb;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		/* populating Leptonica pixels */
		red = *p_rgb++;
		green = *p_rgb++;
		blue = *p_rgb++;
		ret = pixSetRGBPixel (rgb_pix, col, row, red, green, blue);
		if (ret != 0)
		    goto error;
	    }
      }

/* normalizing num_colors in the expected range 128-240 */
    n_colors = num_colors;
    if (n_colors < 128)
	n_colors = 128;
    if (n_colors > 240)
	n_colors = 240;

/* color quantization */
    palette_pix = pixOctreeColorQuant (rgb_pix, n_colors, 0);
    if (palette_pix == NULL)
	goto error;
    pixDestroy (&rgb_pix);
    rgb_pix = NULL;

/* allocating the palette and the pixbuf to be returned */
    cmap = pixGetColormap (palette_pix);
    if (cmap == NULL)
	goto error;
    out_colors = pixcmapGetCount (cmap);
    plt = rl2_create_palette (out_colors);
    if (plt == NULL)
	goto error;
    for (col = 0; col < out_colors; col++)
      {
	  /* populating the palette */
	  ret = pixcmapGetColor (cmap, col, &red, &green, &blue);
	  if (ret != 0)
	      goto error;
	  rl2_set_palette_color (plt, col, red, green, blue);
      }

    pixels = malloc (width * height);
    if (pixels == NULL)
	goto error;
    p_pix = pixels;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		/* populating pixbuf pixels */
		ret = pixGetPixel (palette_pix, col, row, &index);
		if (ret != 0)
		    goto error;
		*p_pix++ = index;
	    }
      }

/* final memory cleanup */
    pixDestroy (&palette_pix);
    *pixbuf = pixels;
    *palette = plt;
    return RL2_OK;

  error:
    if (rgb_pix != NULL)
	pixDestroy (&rgb_pix);
    if (palette_pix != NULL)
	pixDestroy (&palette_pix);
    if (pixels != NULL)
	free (pixels);
    if (plt != NULL)
	rl2_destroy_palette (plt);
#endif
    return RL2_ERROR;
}

RL2_DECLARE char *
rl2_leptonica_version (void)
{
/* returning the Leptonica version string */
#ifndef OMIT_LEPTONICA
    return getLeptonicaVersion ();
#else
    char *version = malloc (16);
    strcpy (version, "unsupported");
    return version;
#endif
}
