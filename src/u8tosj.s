.title u8tosj - UTF-8 to Shift_JIS converter

# This file is part of src2build
# Copyright (C) 2025 TcbnErik
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


.include doscall.mac
.include iocscall.mac

LF: .equ $0a
CR: .equ $0d
STDERR: .equ 2


.cpu 68000
.text

_CreateU2SArray::
  movem.l d4-d6/a0-a6,-(sp)

  .xref U2STableBitmap
  .xref U2STableOffset
  lea (U2STableBitmap),a6  ;short[16]
  lea (U2STableOffset),a5  ;short[??]

  movea.l (U2STableBufferPtr,pc),a4  ;void*[256]
  lea ($100*4,a4),a0                 ;short[256] sjis
  move.l a0,d4

  move.l (UndefinedCharSjisCode,pc),d0
  moveq #$100/4-1,d6
  @@:
    move.l d0,(a0)+
    move.l d0,(a0)+
  dbra d6,@b

  moveq #$100/16-1,d6
  1:
    swap d6
    move (a6)+,d5  ;bitmap
    move #16-1,d6
    2:
      move.l d4,d0
      add d5,d5
      bcc @f  ;all undefined code point
        move (a5)+,d0
        lea (-2,a5,d0.w),a1  ;U_**xx address

        move.l a1,(a0)
        move.l a0,d0
        lea ($100*2,a0),a0

        not.l d0  ;minus = uninitialized
      @@:
      move.l d0,(a4)+
    dbra d6,2b
    swap d6
  dbra d6,1b

  movem.l (sp)+,d4-d6/a0-a6
  rts


;(4,sp) = length
;(8,sp) = read/write buffer
_Utf8toSjis::
  ~saveregs: .reg d3-d7/a3-a6
  movem.l ~saveregs,-(sp)
  movem.l (.sizeof.(~saveregs)+4,sp),d7/a0
  move.l d7,d0
  beq utf8tosjisEnd  ;d0=0

  move.l a0,d6  ;buffer head
  lea (a0),a4   ;write pointer
  movea.l (U2STableBufferPtr,pc),a6

  utf8tosjisLoop0:
    moveq #0,d0
  utf8tosjisLoop:
    move.b (a0)+,d0
    bmi @f
      move.b d0,(a4)+  ;U+0000..U+007F
      subq.l #1,d7
      bhi utf8tosjisLoop
      bra utf8tosjisLoopEnd
    @@:
      bsr Utf8ToCodePoint
      bmi utf8tosjisInvalid

      bsr CodePointToSjis
      move d0,-(sp)
      move.b (sp)+,d2
      beq @f
        move.b d2,(a4)+
      @@:
      move.b d0,(a4)+

      sub.l d1,d7
      lea (-1,a0,d1.l),a0
      bhi utf8tosjisLoop0
      bra utf8tosjisLoopEnd

  utf8tosjisLoopEnd:
  move.l a4,d0
  sub.l d6,d0  ;sjis length

  utf8tosjisEnd:
  movem.l (sp)+,~saveregs
  rts

  utf8tosjisInvalid:
    subq.l #1,a0
    neg.l d0
    bsr PrintInvalidSequence

    moveq #-1,d0
    bra utf8tosjisEnd


;in d0.l = first byte ($80..$ff)
;   d7.l = rest bytes
;   a0.l = second byte
;out d0.l = code point
;    d1.l = sequence length (2..4)
;break d2-d3/a1

Utf8ToCodePoint:
  move.l d0,d1
  lsr.b #3,d1
  move.b (Utf8LengthTable-16,pc,d1.l),d1
  beq utf8tocodepointInvalid_1  ;continuation byte or overlong encoding
  cmp.l d1,d7
  bcs utf8tocodepointInvalid_d7  ;string ending before end of character

  ;d1=2,3,4
  and.b (Utf8MaskTable-2,pc,d1.l),d0
  lea (a0),a1
  moveq #%0100_0000,d3

  move.b (a1)+,d2
  add.b d3,d2
  add.b d3,d2
  bcc utf8tocodepointInvalid_d1

  lsl #6,d0
  or.b d2,d0

  cmpi.b #3,d1
  beq 3f
  bcs 2f
      move.b (a1)+,d2
      add.b d3,d2
      add.b d3,d2
      bcc utf8tocodepointInvalid_d1

      lsl #6,d0
      or.b d2,d0
    3:
    move.b (a1)+,d2
    add.b d3,d2
    add.b d3,d2
    bcc utf8tocodepointInvalid_d1

    lsl.l #6,d0
    or.b d2,d0
  2:
  move d1,d2
  lsl #2,d2
  cmp.l (Utf8LowerLimitTable-4*2,pc,d2.w),d0
  bcs utf8tocodepointInvalid_d1  ;invalid code point

  tst.l d0
  utf8tocodepointEnd:
  rts

  utf8tocodepointInvalid_1:
    moveq #-1,d0
    bra utf8tocodepointEnd
  utf8tocodepointInvalid_d1:
    move.l d1,d0
    bra @f
  utf8tocodepointInvalid_d7:
    move.l d7,d0
  @@:
    neg.l d0
    bra utf8tocodepointEnd


.quad
.xref U2STableBuffer
U2STableBufferPtr: .dc.l U2STableBuffer

Utf8LowerLimitTable:
  .dc.l $80,$800,$10000
Utf8LengthTable:
  .dc.b 0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,0
Utf8MaskTable:
  .dc.b %0001_1111,%0000_1111,%0000_0111
  .even
UndefinedCharSjisCodeL:
  .dc 0  ;,$81a6
UndefinedCharSjisCode:
  .dc $81a6,$81a6
.quad


;break d2/a0
CodePointToSjis:
  cmpi.l #$ffff,d0
  bls @f
    move.l (UndefinedCharSjisCodeL,pc),d0
    rts
  @@:
  move d0,d2
  clr.b d2
  lsr #8-2,d2  ;highbyte*4

  move.l (a6,d2.w),d2
  movea.l d2,a1  ;short[256] sjis
  bpl 9f
    not.l d2  ;uninitialized
    movea.l d2,a1

    move d0,d2
    clr.b d2
    lsr #8-2,d2
    move.l a1,(a6,d2.w)  ;short[256] sjis

    bsr CreateU2SArray2
  9:
  andi #$ff,d0
  add d0,d0
  move (a1,d0.w),d0
  rts


CreateU2SArray2:
  movem.l d0/d4-d7/a0-a2,-(sp)

  movea.l (a1),a0  ;U_**xx bitmap
  lea ($100/8,a0),a2  ;short[??] sjis

  move (UndefinedCharSjisCode,pc),d4
  moveq #$100/16-1,d7
  1:
    move (a0)+,d5
    moveq #16-1,d6
    2:
      move d4,d0
      add d5,d5
      bcc @f
        move (a2)+,d0
      @@:
      move d0,(a1)+
    dbra d6,2b
  dbra d7,1b

  movem.l (sp)+,d0/d4-d7/a0-a2
  rts


PrintInvalidSequence:
  move.l d3,-(sp)
  link a6,#-64
  move d0,d3   ;length
  lea (a0),a2  ;byte sequence

  lea (InvalidUtf8Sequence,pc),a1
  lea (sp),a0
  bsr Strcpy_a1a0
  @@:
    move.b #' ',(a0)+
    move.b (a2)+,d0
    bsr StringifyHexByte
  subq #1,d3
  bne @b

  lea (NewLine,pc),a1
  bsr Strcpy_a1a0

  lea (sp),a0
  bsr PrintError

  unlk a6
  move.l (sp)+,d3
  rts

Strcpy_a1a0:
  @@:
    move.b (a1)+,(a0)+
    bne @b
  subq.l #1,a0
  rts


PrintError:
  lea (a0),a1
  @@:
    tst.b (a1)+
  bne @b
  subq.l #1,a1
  suba.l a0,a1  ;string length

  move.l a1,-(sp)
  pea (a0)
  move #STDERR,-(sp)
  DOS _WRITE
  lea (10,sp),sp
  rts


StringifyHexByte:
  moveq #2-1,d2
  ror.l #8,d0
  bra StringifyHex
StringifyHexWord:
  moveq #4-1,d2
  swap d2
  bra StringifyHex
StringifyHexLongword:
  moveq #8-1,d2
  bra StringifyHex

StringifyHex:
  @@:
    rol.l #4,d0
    moveq #$f,d1
    and.b d0,d1
    move.b (HexTable,pc,d1.w),(a0)+
    dbra d2,@b
  clr.b (a0)
  rts

HexTable:
  .dc.b '0123456789abcdef'


InvalidUtf8Sequence:
  .dc.b 'Invalid byte sequence in UTF-8:',0
NewLine:
  .dc.b CR,LF,0
  .even


.end
