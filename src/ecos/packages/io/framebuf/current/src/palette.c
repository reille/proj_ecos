//==========================================================================
//
//      palette.c
//
//      Provide default palettes
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 2008 Free Software Foundation, Inc.                        
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//==========================================================================
//###DESCRIPTIONBEGIN####
//
// Author(s):     bartv
// Date:          2005-03-29
//
//###DESCRIPTIONEND####
//========================================================================

#include <cyg/io/framebuf.h>

// These figures were taken from a real VGA card. The VGA palette
// registers only use six bits so the values were shifted by two,
// then 0xFC was converted to 0xFF to get maximum brightness.

const cyg_uint8 cyg_fb_palette_ega[16 * 3] = {
    0x00, 0x00, 0x00,       // 0    black
    0x00, 0x00, 0xa8,       // 1    blue
    0x00, 0xa8, 0x00,       // 2    green
    0x00, 0xa8, 0xa8,       // 3    cyan
    0xa8, 0x00, 0x00,       // 4    red
    0xa8, 0x00, 0xa8,       // 5    magenta
    0xa8, 0x54, 0x00,       // 6    brown
    0xa8, 0xa8, 0xa8,       // 7    light grey
    0x54, 0x54, 0x54,       // 8    dark grey
    0x54, 0x54, 0xff,       // 9    light blue
    0x54, 0xff, 0x54,       // 10   light green
    0x54, 0xff, 0xff,       // 11   light cyan
    0xff, 0x54, 0x54,       // 12   light red
    0xff, 0x54, 0xff,       // 13   light magenta
    0xff, 0xff, 0x54,       // 14   yellow
    0xff, 0xff, 0xff        // 15   white
};

const cyg_uint8 cyg_fb_palette_vga[256 * 3] = {
    0x00, 0x00, 0x00,       // 0    black
    0x00, 0x00, 0xa8,       // 1    blue
    0x00, 0xa8, 0x00,       // 2    green
    0x00, 0xa8, 0xa8,       // 3    cyan
    0xa8, 0x00, 0x00,       // 4    red
    0xa8, 0x00, 0xa8,       // 5    magenta
    0xa8, 0x54, 0x00,       // 6    brown
    0xa8, 0xa8, 0xa8,       // 7    light grey
    0x54, 0x54, 0x54,       // 8    dark grey
    0x54, 0x54, 0xff,       // 9    light blue
    0x54, 0xff, 0x54,       // 10   light green
    0x54, 0xff, 0xff,       // 11   light cyan
    0xff, 0x54, 0x54,       // 12   light red
    0xff, 0x54, 0xff,       // 13   light magenta
    0xff, 0xff, 0x54,       // 14   yellow
    0xff, 0xff, 0xff,       // 15   white
    0x00, 0x00, 0x00,       // 16
    0x14, 0x14, 0x14,       // 17
    0x20, 0x20, 0x20,       // 18
    0x2c, 0x2c, 0x2c,       // 19
    0x38, 0x38, 0x38,       // 20
    0x44, 0x44, 0x44,       // 21
    0x50, 0x50, 0x50,       // 22
    0x60, 0x60, 0x60,       // 23
    0x70, 0x70, 0x70,       // 24
    0x80, 0x80, 0x80,       // 25
    0x90, 0x90, 0x90,       // 26
    0xa0, 0xa0, 0xa0,       // 27
    0xb4, 0xb4, 0xb4,       // 28
    0xc8, 0xc8, 0xc8,       // 29
    0xe0, 0xe0, 0xe0,       // 30
    0xff, 0xff, 0xff,       // 31
    0x00, 0x00, 0xff,       // 32
    0x40, 0x00, 0xff,       // 33
    0x7c, 0x00, 0xff,       // 34
    0xbc, 0x00, 0xff,       // 35
    0xff, 0x00, 0xff,       // 36
    0xff, 0x00, 0xbc,       // 37
    0xff, 0x00, 0x7c,       // 38
    0xff, 0x00, 0x40,       // 39
    0xff, 0x00, 0x00,       // 40
    0xff, 0x40, 0x00,       // 41
    0xff, 0x7c, 0x00,       // 42
    0xff, 0xbc, 0x00,       // 43
    0xff, 0xff, 0x00,       // 44
    0xbc, 0xff, 0x00,       // 45
    0x7c, 0xff, 0x00,       // 46
    0x40, 0xff, 0x00,       // 47
    0x00, 0xff, 0x00,       // 48
    0x00, 0xff, 0x40,       // 49
    0x00, 0xff, 0x7c,       // 50
    0x00, 0xff, 0xbc,       // 51
    0x00, 0xff, 0xff,       // 52
    0x00, 0xbc, 0xff,       // 53
    0x00, 0x7c, 0xff,       // 54
    0x00, 0x40, 0xff,       // 55
    0x7c, 0x7c, 0xff,       // 56
    0x9c, 0x7c, 0xff,       // 57
    0xbc, 0x7c, 0xff,       // 58
    0xdc, 0x7c, 0xff,       // 59
    0xff, 0x7c, 0xff,       // 60
    0xff, 0x7c, 0xdc,       // 61
    0xff, 0x7c, 0xbc,       // 62
    0xff, 0x7c, 0x9c,       // 63
    0xff, 0x7c, 0x7c,       // 64
    0xff, 0x9c, 0x7c,       // 65
    0xff, 0xbc, 0x7c,       // 66
    0xff, 0xdc, 0x7c,       // 67
    0xff, 0xff, 0x7c,       // 68
    0xdc, 0xff, 0x7c,       // 69
    0xbc, 0xff, 0x7c,       // 70
    0x9c, 0xff, 0x7c,       // 71
    0x7c, 0xff, 0x7c,       // 72
    0x7c, 0xff, 0x9c,       // 73
    0x7c, 0xff, 0xbc,       // 74
    0x7c, 0xff, 0xdc,       // 75
    0x7c, 0xff, 0xff,       // 76
    0x7c, 0xdc, 0xff,       // 77
    0x7c, 0xbc, 0xff,       // 78
    0x7c, 0x9c, 0xff,       // 79
    0xb4, 0xb4, 0xff,       // 80
    0xc4, 0xb4, 0xff,       // 81
    0xd8, 0xb4, 0xff,       // 82
    0xe8, 0xb4, 0xff,       // 83
    0xff, 0xb4, 0xff,       // 84
    0xff, 0xb4, 0xe8,       // 85
    0xff, 0xb4, 0xd8,       // 86
    0xff, 0xb4, 0xc4,       // 87
    0xff, 0xb4, 0xb4,       // 88
    0xff, 0xc4, 0xb4,       // 89
    0xff, 0xd8, 0xb4,       // 90
    0xff, 0xe8, 0xb4,       // 91
    0xff, 0xff, 0xb4,       // 92
    0xe8, 0xff, 0xb4,       // 93
    0xd8, 0xff, 0xb4,       // 94
    0xc4, 0xff, 0xb4,       // 95
    0xb4, 0xff, 0xb4,       // 96
    0xb4, 0xff, 0xc4,       // 97
    0xb4, 0xff, 0xd8,       // 98
    0xb4, 0xff, 0xe8,       // 99
    0xb4, 0xff, 0xff,       // 100
    0xb4, 0xe8, 0xff,       // 101
    0xb4, 0xd8, 0xff,       // 102
    0xb4, 0xc4, 0xff,       // 103
    0x00, 0x00, 0x70,       // 104
    0x1c, 0x00, 0x70,       // 105
    0x38, 0x00, 0x70,       // 106
    0x54, 0x00, 0x70,       // 107
    0x70, 0x00, 0x70,       // 108
    0x70, 0x00, 0x54,       // 109
    0x70, 0x00, 0x38,       // 110
    0x70, 0x00, 0x1c,       // 111
    0x70, 0x00, 0x00,       // 112
    0x70, 0x1c, 0x00,       // 113
    0x70, 0x38, 0x00,       // 114
    0x70, 0x54, 0x00,       // 115
    0x70, 0x70, 0x00,       // 116
    0x54, 0x70, 0x00,       // 117
    0x38, 0x70, 0x00,       // 118
    0x1c, 0x70, 0x00,       // 119
    0x00, 0x70, 0x00,       // 120
    0x00, 0x70, 0x1c,       // 121
    0x00, 0x70, 0x38,       // 122
    0x00, 0x70, 0x54,       // 123
    0x00, 0x70, 0x70,       // 124
    0x00, 0x54, 0x70,       // 125
    0x00, 0x38, 0x70,       // 126
    0x00, 0x1c, 0x70,       // 127
    0x38, 0x38, 0x70,       // 128
    0x44, 0x38, 0x70,       // 129
    0x54, 0x38, 0x70,       // 130
    0x60, 0x38, 0x70,       // 131
    0x70, 0x38, 0x70,       // 132
    0x70, 0x38, 0x60,       // 133
    0x70, 0x38, 0x54,       // 134
    0x70, 0x38, 0x44,       // 135
    0x70, 0x38, 0x38,       // 136
    0x70, 0x44, 0x38,       // 137
    0x70, 0x54, 0x38,       // 138
    0x70, 0x60, 0x38,       // 139
    0x70, 0x70, 0x38,       // 140
    0x60, 0x70, 0x38,       // 141
    0x54, 0x70, 0x38,       // 142
    0x44, 0x70, 0x38,       // 143
    0x38, 0x70, 0x38,       // 144
    0x38, 0x70, 0x44,       // 145
    0x38, 0x70, 0x54,       // 146
    0x38, 0x70, 0x60,       // 147
    0x38, 0x70, 0x70,       // 148
    0x38, 0x60, 0x70,       // 149
    0x38, 0x54, 0x70,       // 150
    0x38, 0x44, 0x70,       // 151
    0x50, 0x50, 0x70,       // 152
    0x58, 0x50, 0x70,       // 153
    0x60, 0x50, 0x70,       // 154
    0x68, 0x50, 0x70,       // 155
    0x70, 0x50, 0x70,       // 156
    0x70, 0x50, 0x68,       // 157
    0x70, 0x50, 0x60,       // 158
    0x70, 0x50, 0x58,       // 159
    0x70, 0x50, 0x50,       // 160
    0x70, 0x58, 0x50,       // 161
    0x70, 0x60, 0x50,       // 162
    0x70, 0x68, 0x50,       // 163
    0x70, 0x70, 0x50,       // 164
    0x68, 0x70, 0x50,       // 165
    0x60, 0x70, 0x50,       // 166
    0x58, 0x70, 0x50,       // 167
    0x50, 0x70, 0x50,       // 168
    0x50, 0x70, 0x58,       // 169
    0x50, 0x70, 0x60,       // 170
    0x50, 0x70, 0x68,       // 171
    0x50, 0x70, 0x70,       // 172
    0x50, 0x68, 0x70,       // 173
    0x50, 0x60, 0x70,       // 174
    0x50, 0x58, 0x70,       // 175
    0x00, 0x00, 0x40,       // 176
    0x10, 0x00, 0x40,       // 177
    0x20, 0x00, 0x40,       // 178
    0x30, 0x00, 0x40,       // 179
    0x40, 0x00, 0x40,       // 180
    0x40, 0x00, 0x30,       // 181
    0x40, 0x00, 0x20,       // 182
    0x40, 0x00, 0x10,       // 183
    0x40, 0x00, 0x00,       // 184
    0x40, 0x10, 0x00,       // 185
    0x40, 0x20, 0x00,       // 186
    0x40, 0x30, 0x00,       // 187
    0x40, 0x40, 0x00,       // 188
    0x30, 0x40, 0x00,       // 189
    0x20, 0x40, 0x00,       // 190
    0x10, 0x40, 0x00,       // 191
    0x00, 0x40, 0x00,       // 192
    0x00, 0x40, 0x10,       // 193
    0x00, 0x40, 0x20,       // 194
    0x00, 0x40, 0x30,       // 195
    0x00, 0x40, 0x40,       // 196
    0x00, 0x30, 0x40,       // 197
    0x00, 0x20, 0x40,       // 198
    0x00, 0x10, 0x40,       // 199
    0x20, 0x20, 0x40,       // 200
    0x28, 0x20, 0x40,       // 201
    0x30, 0x20, 0x40,       // 202
    0x38, 0x20, 0x40,       // 203
    0x40, 0x20, 0x40,       // 204
    0x40, 0x20, 0x38,       // 205
    0x40, 0x20, 0x30,       // 206
    0x40, 0x20, 0x28,       // 207
    0x40, 0x20, 0x20,       // 208
    0x40, 0x28, 0x20,       // 209
    0x40, 0x30, 0x20,       // 210
    0x40, 0x38, 0x20,       // 211
    0x40, 0x40, 0x20,       // 212
    0x38, 0x40, 0x20,       // 213
    0x30, 0x40, 0x20,       // 214
    0x28, 0x40, 0x20,       // 215
    0x20, 0x40, 0x20,       // 216
    0x20, 0x40, 0x28,       // 217
    0x20, 0x40, 0x30,       // 218
    0x20, 0x40, 0x38,       // 219
    0x20, 0x40, 0x40,       // 220
    0x20, 0x38, 0x40,       // 221
    0x20, 0x30, 0x40,       // 222
    0x20, 0x28, 0x40,       // 223
    0x2c, 0x2c, 0x40,       // 224
    0x30, 0x2c, 0x40,       // 225
    0x34, 0x2c, 0x40,       // 226
    0x3c, 0x2c, 0x40,       // 227
    0x40, 0x2c, 0x40,       // 228
    0x40, 0x2c, 0x3c,       // 229
    0x40, 0x2c, 0x34,       // 230
    0x40, 0x2c, 0x30,       // 231
    0x40, 0x2c, 0x2c,       // 232
    0x40, 0x30, 0x2c,       // 233
    0x40, 0x34, 0x2c,       // 234
    0x40, 0x3c, 0x2c,       // 235
    0x40, 0x40, 0x2c,       // 236
    0x3c, 0x40, 0x2c,       // 237
    0x34, 0x40, 0x2c,       // 238
    0x30, 0x40, 0x2c,       // 239
    0x2c, 0x40, 0x2c,       // 240
    0x2c, 0x40, 0x30,       // 241
    0x2c, 0x40, 0x34,       // 242
    0x2c, 0x40, 0x3c,       // 243
    0x2c, 0x40, 0x40,       // 244
    0x2c, 0x3c, 0x40,       // 245
    0x2c, 0x34, 0x40,       // 246
    0x2c, 0x30, 0x40,       // 247
    0x00, 0x00, 0x00,       // 248
    0x00, 0x00, 0x00,       // 249
    0x00, 0x00, 0x00,       // 250
    0x00, 0x00, 0x00,       // 251
    0x00, 0x00, 0x00,       // 252
    0x00, 0x00, 0x00,       // 253
    0x00, 0x00, 0x00,       // 254
    0x00, 0x00, 0x00        // 255
};
