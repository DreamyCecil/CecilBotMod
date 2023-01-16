/* Copyright (c) 2018-2023 Dreamy Cecil
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef _CECILBOTS_PATHPOLYGON_H
#define _CECILBOTS_PATHPOLYGON_H

// [Cecil] 2018-10-24: Bot Path Polygons
class DECL_DLL CPathPolygon {
  public:
    // Original brush polygon
    CBrushPolygon *bppo_bpoPolygon;

    // Vertex positions of this polygon (should always be three)
    CStaticStackArray<FLOAT3D> bppo_avVertices;

    // Constructor & Destructor
    CPathPolygon(void);
    ~CPathPolygon(void);

    // Copy constructor
    CPathPolygon(const CPathPolygon &bppoOther);

    // Writing & Reading
    void WritePolygon(CTStream *strm);
    void ReadPolygon(CTStream *strm);

    // Absolute center position of this polygon
    FLOAT3D Center(void);
};

#endif // _CECILBOTS_PATHPOLYGON_H
