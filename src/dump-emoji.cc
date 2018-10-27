/*
 * Copyright © 2018  Ebrahim Byagowi
 * Copyright © 2018  Khaled Hosny
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include "hb-static.cc"
#include "hb-ot-color-cbdt-table.hh"
#include "hb-ot-color-colr-table.hh"
#include "hb-ot-color-cpal-table.hh"
#include "hb-ot-color-sbix-table.hh"
#include "hb-ot-color-svg-table.hh"

#include "hb-ft.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <cairo.h>
#include <cairo-ft.h>
#include <cairo-svg.h>

#ifdef HAVE_GLIB
#include <glib.h>
#endif
#include <stdlib.h>
#include <stdio.h>

static void
cbdt_callback (const uint8_t* data, unsigned int length,
	       unsigned int group, unsigned int gid)
{
  char output_path[255];
  sprintf (output_path, "out/cbdt-%d-%d.png", group, gid);
  FILE *f = fopen (output_path, "wb");
  fwrite (data, 1, length, f);
  fclose (f);
}

static void
sbix_dump (hb_face_t *face)
{
  OT::sbix::accelerator_t sbix;
  sbix.init (face);
  unsigned int length = 0;
  unsigned int *available_ppems = sbix.get_available_ppems (&length);
  unsigned int num_glyphs = face->num_glyphs;
  for (unsigned int group = 0; group < length; group++)
    for (unsigned int glyph_id = 0; glyph_id < num_glyphs; glyph_id++)
    {
      hb_blob_t *blob;
      blob = sbix.reference_blob_for_glyph (glyph_id, 0, available_ppems[group],
					    HB_TAG('p','n','g',' '), NULL, NULL);
      if (hb_blob_get_length (blob) == 0) continue;

      char output_path[255];
      sprintf (output_path, "out/sbix-%d-%d.png", available_ppems[group], glyph_id);
      FILE *f = fopen (output_path, "wb");
      unsigned int length;
      const char* data = hb_blob_get_data (blob, &length);
      fwrite (data, 1, length, f);
      fclose (f);
    }
  sbix.fini ();
}

static void
svg_dump (hb_face_t *face)
{
  unsigned glyph_count = hb_face_get_glyph_count (face);

  for (unsigned int glyph_id = 0; glyph_id < glyph_count; glyph_id++)
  {
    hb_blob_t *blob = hb_ot_color_glyph_reference_blob_svg (face, glyph_id);

    if (hb_blob_get_length (blob) == 0) continue;

    unsigned int length;
    const char *data = hb_blob_get_data (blob, &length);

    char output_path[256];
    sprintf (output_path, "out/svg-%d.svg%s",
	     glyph_id,
	     // append "z" if the content is gzipped, https://stackoverflow.com/a/6059405
	     (length > 2 && (data[0] == '\x1F') && (data[1] == '\x8B')) ? "z" : "");

    FILE *f = fopen (output_path, "wb");
    fwrite (data, 1, length, f);
    fclose (f);

    hb_blob_destroy (blob);
  }
}

static void
colr_cpal_dump (hb_face_t *face, cairo_font_face_t *cairo_face)
{
  unsigned int upem = hb_face_get_upem (face);

  unsigned glyph_count = hb_face_get_glyph_count (face);
  for (hb_codepoint_t gid = 0; gid < glyph_count; ++gid)
  {
    unsigned int num_layers = hb_ot_color_glyph_get_layers (face, gid, 0, nullptr, nullptr);
    if (!num_layers)
      continue;

    hb_ot_color_layer_t *layers = (hb_ot_color_layer_t*) malloc (num_layers * sizeof (hb_ot_color_layer_t));

    hb_ot_color_glyph_get_layers (face, gid, 0, &num_layers, layers);
    if (num_layers)
    {
      // Measure
      cairo_text_extents_t extents;
      {
	cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cairo_t *cr = cairo_create (surface);
	cairo_set_font_face (cr, cairo_face);
	cairo_set_font_size (cr, upem);

	cairo_glyph_t *glyphs = (cairo_glyph_t *) calloc (num_layers, sizeof (cairo_glyph_t));
	for (unsigned int j = 0; j < num_layers; ++j)
	  glyphs[j].index = layers[j].glyph;
	cairo_glyph_extents (cr, glyphs, num_layers, &extents);
	free (glyphs);
	cairo_surface_destroy (surface);
	cairo_destroy (cr);
      }

      // Add a slight margin
      extents.width += extents.width / 10;
      extents.height += extents.height / 10;
      extents.x_bearing -= extents.width / 20;
      extents.y_bearing -= extents.height / 20;

      // Render
      unsigned int palette_count = hb_ot_color_palette_get_count (face);
      for (unsigned int palette = 0; palette < palette_count; palette++) {
	char output_path[255];

	unsigned int num_colors = hb_ot_color_palette_get_colors (face, palette, 0, nullptr, nullptr);
	if (!num_colors)
	  continue;

	hb_color_t *colors = (hb_color_t*) calloc (num_colors, sizeof (hb_color_t));
	hb_ot_color_palette_get_colors (face, palette, 0, &num_colors, colors);
	if (num_colors)
	{
	  // If we have more than one palette, use a simpler naming
	  if (palette_count == 1)
	    sprintf (output_path, "out/colr-%d.svg", gid);
	  else
	    sprintf (output_path, "out/colr-%d-%d.svg", gid, palette);

	  cairo_surface_t *surface = cairo_svg_surface_create (output_path, extents.width, extents.height);
	  cairo_t *cr = cairo_create (surface);
	  cairo_set_font_face (cr, cairo_face);
	  cairo_set_font_size (cr, upem);

	  for (unsigned int layer = 0; layer < num_layers; ++layer)
	  {
	    hb_color_t color = 0x000000FF;
	    if (layers[layer].color_index != 0xFFFF)
	      color = colors[layers[layer].color_index];
	    cairo_set_source_rgba (cr,
				   hb_color_get_red (color) / 255.,
				   hb_color_get_green (color) / 255.,
				   hb_color_get_blue (color) / 255.,
				   hb_color_get_alpha (color) / 255.);

	    cairo_glyph_t glyph;
	    glyph.index = layers[layer].glyph;
	    glyph.x = -extents.x_bearing;
	    glyph.y = -extents.y_bearing;
	    cairo_show_glyphs (cr, &glyph, 1);
	  }

	  cairo_surface_destroy (surface);
	  cairo_destroy (cr);
	}
	free (colors);
      }
    }

    free (layers);
  }
}

static void
dump_glyphs (cairo_font_face_t *cairo_face, unsigned int upem,
	     unsigned int num_glyphs)
{
  // Dump every glyph available on the font
  return; // disabled for now
  for (unsigned int i = 0; i < num_glyphs; ++i)
  {
    cairo_text_extents_t extents;
    cairo_glyph_t glyph = {0};
    glyph.index = i;

    // Measure
    {
      cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
      cairo_t *cr = cairo_create (surface);
      cairo_set_font_face (cr, cairo_face);
      cairo_set_font_size (cr, upem);

      cairo_glyph_extents (cr, &glyph, 1, &extents);
      cairo_surface_destroy (surface);
      cairo_destroy (cr);
    }

    // Add a slight margin
    extents.width += extents.width / 10;
    extents.height += extents.height / 10;
    extents.x_bearing -= extents.width / 20;
    extents.y_bearing -= extents.height / 20;

    // Render
    {
      char output_path[255];
      sprintf (output_path, "out/%d.svg", i);
      cairo_surface_t *surface = cairo_svg_surface_create (output_path, extents.width, extents.height);
      cairo_t *cr = cairo_create (surface);
      cairo_set_font_face (cr, cairo_face);
      cairo_set_font_size (cr, upem);
      glyph.x = -extents.x_bearing;
      glyph.y = -extents.y_bearing;
      cairo_show_glyphs (cr, &glyph, 1);
      cairo_surface_destroy (surface);
      cairo_destroy (cr);
    }
  }
}

int
main (int argc, char **argv)
{
  if (argc != 2) {
    fprintf (stderr, "usage: %s font-file.ttf\n"
		     "run it like `rm -rf out && mkdir out && %s font-file.ttf`\n",
		     argv[0], argv[0]);
    exit (1);
  }


  FILE *font_name_file = fopen ("out/_font_name_file.txt", "r");
  if (font_name_file != nullptr)
  {
    fprintf (stderr, "Purge or move ./out folder in order to run a new dump\n");
    exit (1);
  }

  font_name_file = fopen ("out/_font_name_file.txt", "w");
  if (font_name_file == nullptr)
  {
    fprintf (stderr, "./out is not accessible as a folder, create it please\n");
    exit (1);
  }
  fwrite (argv[1], 1, strlen (argv[1]), font_name_file);
  fclose (font_name_file);

  hb_blob_t *blob = hb_blob_create_from_file (argv[1]);
  hb_face_t *face = hb_face_create (blob, 0);
  hb_font_t *font = hb_font_create (face);

  OT::CBDT::accelerator_t cbdt;
  cbdt.init (face);
  cbdt.dump (cbdt_callback);
  cbdt.fini ();

  sbix_dump (face);

  if (hb_ot_color_has_svg (face))
    svg_dump (face);

  cairo_font_face_t *cairo_face;
  {
    FT_Library library;
    FT_Init_FreeType (&library);
    FT_Face ftface;
    FT_New_Face (library, argv[1], 0, &ftface);
    cairo_face = cairo_ft_font_face_create_for_ft_face (ftface, 0);
  }
  if (hb_ot_color_has_layers (face) && hb_ot_color_has_palettes (face))
    colr_cpal_dump (face, cairo_face);

  unsigned int num_glyphs = hb_face_get_glyph_count (face);
  unsigned int upem = hb_face_get_upem (face);
  dump_glyphs (cairo_face, upem, num_glyphs);

  hb_font_destroy (font);
  hb_face_destroy (face);
  hb_blob_destroy (blob);

  return 0;
}
