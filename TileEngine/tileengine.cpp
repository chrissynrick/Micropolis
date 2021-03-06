/* tileengine.cpp
 *
 * Micropolis, Unix Version.  This game was released for the Unix platform
 * in or about 1990 and has been modified for inclusion in the One Laptop
 * Per Child program.  Copyright (C) 1989 - 2007 Electronic Arts Inc.  If
 * you need assistance with this program, you may contact:
 *   http://wiki.laptop.org/go/Micropolis  or email  micropolis@laptop.org.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.  You should have received a
 * copy of the GNU General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 * 
 *             ADDITIONAL TERMS per GNU GPL Section 7
 * 
 * No trademark or publicity rights are granted.  This license does NOT
 * give you any right, title or interest in the trademark SimCity or any
 * other Electronic Arts trademark.  You may not distribute any
 * modification of this program using the trademark SimCity or claim any
 * affliation or association with Electronic Arts Inc. or its employees.
 * 
 * Any propagation or conveyance of this program must include this
 * copyright notice and these terms.
 * 
 * If you convey this program (or any modifications of it) and assume
 * contractual liability for the program to recipients of it, you agree
 * to indemnify Electronic Arts for any liability that those contractual
 * assumptions impose on Electronic Arts.
 * 
 * You may not misrepresent the origins of this program; modified
 * versions of the program must be marked as such and not identified as
 * the original program.
 * 
 * This disclaimer supplements the one included in the General Public
 * License.  TO THE FULLEST EXTENT PERMISSIBLE UNDER APPLICABLE LAW, THIS
 * PROGRAM IS PROVIDED TO YOU "AS IS," WITH ALL FAULTS, WITHOUT WARRANTY
 * OF ANY KIND, AND YOUR USE IS AT YOUR SOLE RISK.  THE ENTIRE RISK OF
 * SATISFACTORY QUALITY AND PERFORMANCE RESIDES WITH YOU.  ELECTRONIC ARTS
 * DISCLAIMS ANY AND ALL EXPRESS, IMPLIED OR STATUTORY WARRANTIES,
 * INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY, SATISFACTORY QUALITY,
 * FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS, AND WARRANTIES (IF ANY) ARISING FROM A COURSE OF DEALING,
 * USAGE, OR TRADE PRACTICE.  ELECTRONIC ARTS DOES NOT WARRANT AGAINST
 * INTERFERENCE WITH YOUR ENJOYMENT OF THE PROGRAM; THAT THE PROGRAM WILL
 * MEET YOUR REQUIREMENTS; THAT OPERATION OF THE PROGRAM WILL BE
 * UNINTERRUPTED OR ERROR-FREE, OR THAT THE PROGRAM WILL BE COMPATIBLE
 * WITH THIRD PARTY SOFTWARE OR THAT ANY ERRORS IN THE PROGRAM WILL BE
 * CORRECTED.  NO ORAL OR WRITTEN ADVICE PROVIDED BY ELECTRONIC ARTS OR
 * ANY AUTHORIZED REPRESENTATIVE SHALL CREATE A WARRANTY.  SOME
 * JURISDICTIONS DO NOT ALLOW THE EXCLUSION OF OR LIMITATIONS ON IMPLIED
 * WARRANTIES OR THE LIMITATIONS ON THE APPLICABLE STATUTORY RIGHTS OF A
 * CONSUMER, SO SOME OR ALL OF THE ABOVE EXCLUSIONS AND LIMITATIONS MAY
 * NOT APPLY TO YOU.
 */


////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.h"


////////////////////////////////////////////////////////////////////////
// Globals


// Interface to PyCairo functions.
Pycairo_CAPI_t *Pycairo_CAPI;


////////////////////////////////////////////////////////////////////////
// TileEngine class


TileEngine::TileEngine() :
  width(0),
  height(0),
  bufData(NULL),
  colBytes(0),
  rowBytes(0),
  typeCode('?'),
  floatOffset((float)0.0),
  floatScale((float)1.0),
  tileMask(~0)
{
}


TileEngine::~TileEngine()
{
}


void TileEngine::setBuffer(
   void *buf)
{
  bufData = buf;
}


unsigned long TileEngine::getValue(
    int row,
    int col)
{
  if ((bufData == NULL) ||
      (col < 0) ||
	  (col >= width) ||
	  (row < 0) ||
	  (row >= height)) {
    return 0;
  }

  switch (typeCode) {

  case 'c':
  case 'b':
  case 'B':
    return
      (unsigned long)(
        *(unsigned char *)(
          (unsigned char *)bufData +
          (col * colBytes) +
          (row * rowBytes))) & 
          tileMask;

  case 'u':
  case 'h':
  case 'H':
  case 'i':
  case 'I':
    return
      (unsigned long)(
        *(unsigned short *)(
          (unsigned char *)bufData +
          (col * colBytes) +
          (row * rowBytes))) & 
          tileMask;

  case 'l':
  case 'L':
    return
      (unsigned long)(
        *(unsigned long *)(
          (unsigned char *)bufData +
          (col * colBytes) +
          (row * rowBytes))) &
          tileMask;

  case 'f':
    return
      (unsigned long)(
        floatOffset +
        (floatScale *
          *(float *)(
            (unsigned char *)bufData +
            (col * colBytes) +
            (row * rowBytes)))) &
          tileMask;

  case 'd':
    return
      (unsigned long)(
        floatOffset +
        (floatScale *
          *(double *)(
            (unsigned char *)bufData +
            (col * colBytes) +
            (row * rowBytes)))) &
          tileMask;

  default:
    return 0;

  }
}


// Render fixed sized tiles, pre-drawn into the Cairo surface "tiles".
void TileEngine::renderTiles(
  cairo_t *ctx,
  cairo_surface_t *tilesSurf,
  int tilesWidth,
  int tilesHeight,
  PyObject *tileMap,
  int tileSize,
  int renderCol,
  int renderRow,
  int renderCols,
  int renderRows,
  double alpha)
{

  // The tileMap should be an array of four byte integers,
  // mapping virtual tiles indices to absolute tile numbers.
  if (!PySequence_Check(tileMap)) {
    PyErr_SetString(
      PyExc_TypeError,
      "tileMap must be an array of four byte integers");
    return;
  }

  unsigned int tileCount =
    (unsigned int)PySequence_Size(tileMap);

  const int *tileMapData = 
	NULL;
  Py_ssize_t tileMapLength = 
	0;

  if (PyObject_AsReadBuffer(
        tileMap,
        (const void **)&tileMapData,
        &tileMapLength) != 0) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected array");
    return;
  }

  int tileMapCount = 
	  (int)tileMapLength / sizeof(unsigned int);

  if (tileMapCount != (int)tileCount) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected array of 4 byte integers");
    return;
  }

  int tilesPerRow = 
    tilesWidth / tileSize;

  int renderX =
    renderCol * tileSize;
  int renderY =
    renderRow * tileSize;

  int r;
  for (r = 0; r < renderRows; r++) {

    int c;
    for (c = 0; c  < renderCols; c++) {

      int col = 
		  (renderCol + c) % width;
      int row = 
		  (renderRow + r) % height;

      unsigned long value =
        getValue(
          row,
          col);

      int tileIndex =
        value % tileCount;

	  // Map virtual tiles index to absolute tile number.
      int tile =
        tileMapData[tileIndex];

      double x = 
        col * tileSize;
      double y =
        row * tileSize;

      // Tiles are arranged in a regular grid. 
	  // Calculate the position of the file in the source tileSurf. 
      int tileCol = 
        tile % tilesPerRow;
      int tileRow =
        tile / tilesPerRow;
      int tileX =
        tileCol * tileSize;
      int tileY =
        tileRow * tileSize;

	  // Draw a tile.

      cairo_save(
        ctx);

      cairo_translate(
        ctx,
        x - renderX,
        y - renderY);

      cairo_rectangle(
        ctx,
        0,
        0,
        tileSize,
        tileSize);

      cairo_clip(
        ctx);

      cairo_set_source_surface(
        ctx,
        tilesSurf,
		-tileX,
		-tileY);

      if (alpha >= 1.0) {
        cairo_paint(
          ctx);
      } else {
        cairo_paint_with_alpha(
          ctx,
          alpha);
      }

      cairo_restore(
        ctx);

    }
  }
}


void TileEngine::renderTilesLazy(
  cairo_t *ctx,
  PyObject *tileMap,
  int tileSize,
  int renderCol,
  int renderRow,
  int renderCols,
  int renderRows,
  double alpha,
  PyObject *tileGenerator,
  PyObject *tileCache,
  PyObject *tileCacheSurfaces)
{

  // The tileMap should be an array of four byte integers,
  // mapping virtual tiles indices to absolute tile numbers.
  if (!PySequence_Check(tileMap)) {
    PyErr_SetString(
      PyExc_TypeError,
      "tileMap must be an array of four byte integers");
    return;
  }

  unsigned int tileCount =
    (unsigned int)PySequence_Size(tileMap);

  const int *tileMapData = 
	NULL;
  Py_ssize_t tileMapLength = 
	0;

  if (PyObject_AsReadBuffer(
        tileMap,
        (const void **)&tileMapData,
        &tileMapLength) != 0) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected array");
    return;
  }

  int tileMapCount = 
	  (int)tileMapLength / sizeof(unsigned int);

  if (tileMapCount != (int)tileCount) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected array of 4 byte integers");
    return;
  }

  // The tileGenerator should be a function that takes one integer tile parameter,
  // and returns a tuple of three integers: a surface index, a tileX and a tileY position.
  if (!PyCallable_Check(
        tileGenerator)) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected callable tileGenerator object");
    return;
  }

  // The tileCache should be an array of integers, four integers per tile.
  // The first is a "cached" flag, the second is a surface index, the third 
  // and fourth are a tileX and tileY position.

  int *tileCacheData = 
	NULL;
  Py_ssize_t tileCacheLength = 
	0;

  if (PyObject_AsWriteBuffer(
        tileCache,
        (void **)&tileCacheData,
        &tileCacheLength) != 0) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected array");
    return;
  }

  int tileCacheCount = 
	  (int)tileCacheLength / sizeof(int);

  if ((tileMapCount * 4) != tileCacheCount) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected array of 4 byte integers, 4 per tile");
    return;
  }

  // The tileCacheSurfaces parameters should be a list of Cairo surfaces.
  if (!PySequence_Check(
        tileCacheSurfaces)) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected sequence");
    return;
  }

  int renderX =
    renderCol * tileSize;
  int renderY =
    renderRow * tileSize;

  int r;
  for (r = 0; r < renderRows; r++) {

    int c;
    for (c = 0; c  < renderCols; c++) {

      int col = 
		(renderCol + c) % width;
      int row = 
		(renderRow + r) % height;

      unsigned long value = 
        getValue(
          row,
          col);

      int tileIndex =
        value % tileCount;

	  // Map virtual tiles index to absolute tile number.
      int tile =
        tileMapData[tileIndex];

      double x = 
        col * tileSize;
      double y =
        row * tileSize;

	  // Get the tile surface index, tileX and tileY from the cache,
	  // or call the tileGenerator function to produce them,
	  // if they are not already cached.

	  int cacheOffset = 
		tile * 4;

	  int tileSurfaceIndex = 0;
	  int tileX = 0;
	  int tileY = 0;

	  if (tileCacheData[cacheOffset + 0]) {

		// The tile is already cached. 
		// Get the values from the tileCache. 

		tileSurfaceIndex = 
		  tileCacheData[cacheOffset + 1];
		tileX =
		  tileCacheData[cacheOffset + 2];
		tileY = 
		  tileCacheData[cacheOffset + 3];

	  } else {

		// The tile has not already been cached. 
		// Call the tileGenerator function to produce the tile,
		// passing it the absolute tile number as a parameter. 

		// Mark the tile as cached, so we don't do this again. 
	    tileCacheData[cacheOffset + 0] = 
	      1;

		PyObject *result =
		  PyObject_CallFunction(
			tileGenerator,
			"i",
			tile);

		// The tile function returns a tuple of three numbers:
		// the surface index (into the tileCacheSurfaces array of Cairo surfaces),
		// the tileX and tileY position in the surface. 

		if (result == NULL) {
		  PyErr_SetString(
		    PyExc_TypeError,
		    "tile generator did not return a result");
		  return;
		}

		int success =
		  PyArg_ParseTuple(
		    result,
		    "iii",
		    &tileSurfaceIndex,
		    &tileX,
		    &tileY);
		if (!success) {
		  PyErr_SetString(
		    PyExc_TypeError,
		    "tile generator return wrong number of results in tuple");
		  return;
		}

		// Cache the returned values. 
		tileCacheData[cacheOffset + 1] = 
		  tileSurfaceIndex;
		tileCacheData[cacheOffset + 2] = 
		  tileX;
		tileCacheData[cacheOffset + 3] = 
		  tileY;

	  }

	  // Get the Python object wrapping the Cairo surface for the tile. 
	  PyObject *tiles =
		PySequence_GetItem(
		  tileCacheSurfaces,
		  tileSurfaceIndex);

	  if (tiles == NULL) {
		PyErr_SetString(
		  PyExc_TypeError,
		  "tile generator returned invalid tile surface index");
		return;
	  }

	  if (!PyObject_TypeCheck(
			tiles,
			&PycairoSurface_Type)) {
		PyErr_SetString(
		  PyExc_TypeError,
		  "expected cairo_surface_t objects in tileCacheSurfaces");
		return;
	  }

	  // Get the cairo_surface_t from the Python object. 
	  cairo_surface_t *tilesSurf =
		PycairoSurface_GET(
		  tiles);
	  Py_DECREF(tiles);

	  // Draw a tile.

	  cairo_save(
		ctx);

	  cairo_translate(
		ctx,
		x - renderX,
		y - renderY);

	  cairo_rectangle(
		ctx,
		0,
		0,
		tileSize,
		tileSize);

	  cairo_clip(
		ctx);

	  cairo_set_source_surface(
		ctx,
		tilesSurf,
		-tileX,
		-tileY);

	  if (alpha >= 1.0) {
		cairo_paint(
		  ctx);
	  } else {
		cairo_paint_with_alpha(
		  ctx,
		  alpha);
	  }

	  cairo_restore(
		ctx);

    }
  }
}


void TileEngine::renderPixels(
    cairo_surface_t *destSurf,
    cairo_surface_t *cmapSurf,
    PyObject *tileMap,
    int renderCol,
    int renderRow,
    int renderCols,
    int renderRows)
{

  unsigned int tileCount =
    (unsigned int)PySequence_Size(tileMap);

  const int *tileMapData =
	NULL;
  Py_ssize_t tileMapLength =
	0;

  if (PyObject_AsReadBuffer(
        tileMap,
        (const void **)&tileMapData,
        &tileMapLength) != 0) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected array");
    return;
  }

  if ((tileMapLength / sizeof(unsigned int)) != tileCount) {
    PyErr_SetString(
      PyExc_TypeError,
      "expected array of 4 byte integers");
    return;
  }

  unsigned char *destData =
    cairo_image_surface_get_data(destSurf);
  int destStride =
    cairo_image_surface_get_stride(destSurf);

  unsigned char *cmapData =
    cairo_image_surface_get_data(cmapSurf);
  int cmapWidth =
    cairo_image_surface_get_width(cmapSurf);
  int cmapStride =
    cairo_image_surface_get_stride(cmapSurf);

  int r;
  for (r = 0; r < renderRows; r++) {

    int c;
    for (c = 0; c  < renderCols; c++) {

      int col = 
		(renderCol + c) % width;
      int row = 
		(renderRow + r) % height;

      unsigned long value = 
        getValue(
          row,
          col);

      int tileIndex =
        value % tileCount;

      int tile =
        tileMapData[tileIndex];

      int sourceX = 
        tile % cmapWidth;
      int sourceY =
        tile / cmapWidth;

      unsigned char *sourcePixel =
        cmapData +
        (sourceX * 4) +
        (sourceY * cmapStride);
      
      unsigned char *destPixel =
        destData +
        (c * 4) +
        (r * destStride);

      *(long *)destPixel = 
		*(long *)sourcePixel;

    }
  }
}


////////////////////////////////////////////////////////////////////////
