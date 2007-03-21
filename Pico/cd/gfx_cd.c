// TODO...

// #include <string.h>
#include "../PicoInt.h"

#define rot_comp Pico_mcd->rot_comp

static const int Table_Rot_Time[] =
{
	0x00054000, 0x00048000, 0x00040000, 0x00036000,          //; 008-032               ; briefing - sprite
	0x0002E000, 0x00028000, 0x00024000, 0x00022000,          //; 036-064               ; arbre souvent
	0x00021000, 0x00020000, 0x0001E000, 0x0001B800,          //; 068-096               ; map thunderstrike
	0x00019800, 0x00017A00, 0x00015C00, 0x00013E00,          //; 100-128               ; logo défoncé

	0x00012000, 0x00011800, 0x00011000, 0x00010800,          //; 132-160               ; briefing - map
	0x00010000, 0x0000F800, 0x0000F000, 0x0000E800,          //; 164-192
	0x0000E000, 0x0000D800, 0x0000D000, 0x0000C800,          //; 196-224
	0x0000C000, 0x0000B800, 0x0000B000, 0x0000A800,          //; 228-256               ; batman visage

	0x0000A000, 0x00009F00, 0x00009E00, 0x00009D00,          //; 260-288
	0x00009C00, 0x00009B00, 0x00009A00, 0x00009900,          //; 292-320
	0x00009800, 0x00009700, 0x00009600, 0x00009500,          //; 324-352
	0x00009400, 0x00009300, 0x00009200, 0x00009100,          //; 356-384

	0x00009000, 0x00008F00, 0x00008E00, 0x00008D00,          //; 388-416
	0x00008C00, 0x00008B00, 0x00008A00, 0x00008900,          //; 420-448
	0x00008800, 0x00008700, 0x00008600, 0x00008500,          //; 452-476
	0x00008400, 0x00008300, 0x00008200, 0x00008100,          //; 480-512
};


#if 1*0
typedef struct
{
	unsigned int Reg_58;	// Stamp_Size
	unsigned int Reg_5A;
	unsigned int Reg_5C;
	unsigned int Reg_5E;
	unsigned int Reg_60;
	unsigned int Reg_62;
	unsigned int Reg_64;	// V_Dot
	unsigned int Reg_66;

	unsigned int Stamp_Map_Adr;
	unsigned int Buffer_Adr;
	unsigned int Vector_Adr;
	unsigned int Function;	// Jmp_Adr;
	unsigned int Float_Part;
	unsigned int Draw_Speed;

	unsigned int XS;
	unsigned int YS;
	/*unsigned*/ int DXS;
	/*unsigned*/ int DYS;
	unsigned int XD;
	unsigned int YD;
	unsigned int XD_Mul;
	unsigned int H_Dot;
} Rot_Comp;
#endif

static void gfx_cd_start(void)
{
	int upd_len;

	dprintf("gfx_cd_start()");

	rot_comp.XD_Mul = ((rot_comp.Reg_5C & 0x1f) + 1) * 4;
	rot_comp.Function = (rot_comp.Reg_58 & 7) | (Pico_mcd->s68k_regs[3] & 0x18);	// Jmp_Adr
	rot_comp.Buffer_Adr = (rot_comp.Reg_5E & 0xfff8) << 2;
	rot_comp.YD = (rot_comp.Reg_60 >> 3) & 7;
	rot_comp.Vector_Adr = (rot_comp.Reg_66 & 0xfffe) << 2;

	upd_len = (rot_comp.Reg_62 >> 3) & 0x3f;
	upd_len = Table_Rot_Time[upd_len];
	rot_comp.Draw_Speed = rot_comp.Float_Part = upd_len;

	rot_comp.Reg_58 |= 0x8000;	// Stamp_Size,  we start a new GFX operation

	switch (rot_comp.Reg_58 & 6)	// Scr_16?
	{
		case 0:	// ?
			rot_comp.Stamp_Map_Adr = (rot_comp.Reg_5A & 0xff80) << 2;
			break;
		case 2: // .Dot_32
			rot_comp.Stamp_Map_Adr = (rot_comp.Reg_5A & 0xffe0) << 2;
			break;
		case 4: // .Scr_16
			rot_comp.Stamp_Map_Adr = 0x20000;
			break;
		case 6: // .Scr_16_Dot_32
			rot_comp.Stamp_Map_Adr = (rot_comp.Reg_5A & 0xe000) << 2;
			break;
	}

	gfx_cd_update();
}


static void gfx_completed(void)
{
	rot_comp.Reg_58 &= 0x7fff;	// Stamp_Size
	rot_comp.Reg_64  = 0;
	if (Pico_mcd->s68k_regs[0x33] & (1<<1))
	{
		dprintf("gfx_cd irq 1");
		SekInterruptS68k(1);
	}
}


static void gfx_do(void)
{
	unsigned int eax, ebx, ecx, edx, esi, edi, pixel = 0;
	unsigned int func = rot_comp.Function;
	unsigned short *ptrs;

	rot_comp.XD = rot_comp.Reg_60 & 7;
	rot_comp.Buffer_Adr = ((rot_comp.Reg_5E & 0xfff8) + rot_comp.YD) << 2;
	rot_comp.H_Dot = rot_comp.Reg_62 & 0x1ff;
	ecx = *(unsigned int *)(Pico_mcd->word_ram2M + rot_comp.Vector_Adr);
	edx = ecx >> 16;
	ecx = (ecx & 0xffff) << 8;
	edx <<= 8;
	rot_comp.DXS = *(int *)(Pico_mcd->word_ram2M + rot_comp.Vector_Adr + 4);
	rot_comp.DYS =  rot_comp.DXS >> 16;
	rot_comp.DXS = (rot_comp.DXS << 16) >> 16; // sign extend
	rot_comp.Vector_Adr += 8;
	//if ((rot_comp.H_Dot & 0x1ff) == 0)
	//	goto nothing_to_draw;

	// MAKE_IMAGE_LINE
	while (rot_comp.H_Dot)
	{
		if (func & 2)		// mode 32x32 dot
		{
			if (func & 4)	// 16x16 screen
			{
				eax = (ecx >> (11+5)) & 0x7f;
				ebx = (edx >> (11-2)) & 0x3f80;
			}
			else		// 1x1 screen
			{
				eax = (ecx >> (11+5)) & 0x0f;
				ebx = (edx >> (11+2)) & 0x38;
			}
		}
		else			// mode 16x16 dot
		{
			if (func & 4)	// 16x16 screen
			{
				eax = (ecx >> (11+4)) & 0xff;
				ebx = (edx >> (11-4)) & 0xff00;
			}
			else		// 1x1 screen
			{
				eax = (ecx >> (11+4)) & 0x0f;
				ebx = (edx >> (11+0)) & 0xf0;
			}
		}
		ebx += eax;

		esi = rot_comp.Stamp_Map_Adr;
		ptrs = (unsigned short *) (Pico_mcd->word_ram2M + rot_comp.Stamp_Map_Adr + ebx*2);
		edi = ptrs[0] | (ptrs[1] << 16);

		// MAKE_IMAGE_PIXEL
		if (!(func & 1))	// NOT TILED
		{
			if (func & 4)	// 16x16 screen
			{
				if ((edx & 0x00800000) || (ecx & 0x00800000)) goto Pixel_Out;
			}
			else		// 1x1 screen
			{
				if ((edx & 0x00f80000) || (ecx & 0x00f80000)) goto Pixel_Out;
			}
		}
		esi = edi;
		edi >>= (11+1);
		esi = (esi & 0x7ff) << 7;
		if (!esi) goto Pixel_Out;
		edi &= (0x1c>>1);
		eax = ecx;
		ebx = edx;
		if (func & 2) edi |= 1;	// 32 dots?
		switch (edi)
		{
			case 0x00:	// No_Flip_0, 16x16 dots
				ebx = (ebx >> 9) & 0x3c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x1000;		// bswap
				eax = ((eax >> 8) & 0x40) + ebx;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x01:	// No_Flip_0, 32x32 dots
				ebx = (ebx >> 9) & 0x7c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x1000;		// bswap
				eax = ((eax >> 7) & 0x180) + ebx;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x02:	// No_Flip_90, 16x16 dots
				eax = (eax >> 9) & 0x3c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x2800;		// bswap
				eax += ((ebx >> 8) & 0x40) ^ 0x40;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x03:	// No_Flip_90, 32x32 dots
				eax = (eax >> 9) & 0x7c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x2800;		// bswap
				eax += (ebx >> 7) & 0x180;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x04:	// No_Flip_180, 16x16 dots
				ebx = ((ebx >> 9) & 0x3c) ^ 0x3c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x2800;		// bswap and flip
				eax = (((eax >> 8) & 0x40) ^ 0x40) + ebx;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x05:	// No_Flip_180, 32x32 dots
				ebx = ((ebx >> 9) & 0x7c) ^ 0x7c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x2800;		// bswap and flip
				eax = (((eax >> 7) & 0x180) ^ 0x180) + ebx;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x06:	// No_Flip_270, 16x16 dots
				eax = ((eax >> 9) & 0x3c) ^ 0x3c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x1000;		// bswap
				eax += (ebx >> 8) & 0x40;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x07:	// No_Flip_270, 32x32 dots
				eax = ((eax >> 9) & 0x7c) ^ 0x7c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x1000;		// bswap
				eax += (ebx >> 7) & 0x180;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x08:	// Flip_0, 16x16 dots
				ebx = ((ebx >> 9) & 0x3c) ^ 0x3c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x2800;		// bswap, flip
				eax = (((eax >> 8) & 0x40) ^ 0x40) + ebx;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x09:	// Flip_0, 32x32 dots
				ebx = ((ebx >> 9) & 0x7c) ^ 0x7c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x2800;		// bswap, flip
				eax = (((eax >> 7) & 0x180) ^ 0x180) + ebx;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x0a:	// Flip_90, 16x16 dots
				eax = ((eax >> 9) & 0x3c) ^ 0x3c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x2800;		// bswap, flip
				eax += ((ebx >> 8) & 0x40) ^ 0x40;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x0b:	// Flip_90, 32x32 dots
				eax = ((eax >> 9) & 0x7c) ^ 0x7c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x2800;		// bswap, flip
				eax += ((ebx >> 7) & 0x180) ^ 0x180;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x0c:	// Flip_180, 16x16 dots
				ebx = ((ebx >> 9) & 0x3c) ^ 0x3c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x1000;		// bswap
				eax = ((eax >> 8) & 0x40) + ebx;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x0d:	// Flip_180, 32x32 dots
				ebx = ((ebx >> 9) & 0x7c) ^ 0x7c;
				ebx += esi;
				edi = (eax & 0x3800) ^ 0x1000;		// bswap
				eax = ((eax >> 7) & 0x180) + ebx;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x0e:	// Flip_270, 16x16 dots
				eax = (eax >> 9) & 0x3c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x1000;		// bswap, flip
				eax += (ebx >> 8) & 0x40;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
			case 0x0f:	// Flip_270, 32x32 dots
				eax = (eax >> 9) & 0x7c;
				eax += esi;
				edi = (ebx & 0x3800) ^ 0x1000;		// bswap, flip
				eax += (ebx >> 7) & 0x180;
				pixel = *(Pico_mcd->word_ram2M + (edi >> 12) + eax);
				if (!(edi & 0x800)) pixel >>= 4;
				break;
		}

Pixel_Out:
//		pixel = 0;
//Finish:
		pixel &= 0x0f;
		if (!pixel && (func & 0x18)) goto Next_Pixel;
		esi = rot_comp.Buffer_Adr + ((rot_comp.XD>>1)^1);	// pixel addr
		eax = *(Pico_mcd->word_ram2M + esi);			// old pixel
		if (rot_comp.XD & 1)
		{
			if ((eax & 0x0f) && (func & 0x18) == 0x08) goto Next_Pixel; // underwrite
			*(Pico_mcd->word_ram2M + esi) = pixel | (eax & 0xf0);
		}
		else
		{
			if ((eax & 0xf0) && (func & 0x18) == 0x08) goto Next_Pixel; // underwrite
			*(Pico_mcd->word_ram2M + esi) = (pixel << 4) | (eax & 0xf);
		}


Next_Pixel:
		ecx += rot_comp.DXS;
		edx += rot_comp.DYS;
		rot_comp.XD++;
		if (rot_comp.XD >= 8)
		{
			rot_comp.Buffer_Adr += ((rot_comp.Reg_5C & 0x1f) + 1) << 5;
			rot_comp.XD = 0;
		}
		rot_comp.H_Dot--;
	}


//nothing_to_draw:
	rot_comp.YD++;
	// rot_comp.V_Dot--; // will be done by caller
}


void gfx_cd_update(void)
{
	unsigned char *V_Dot = (unsigned char *) &rot_comp.Reg_64;
	int jobs;

	dprintf("gfx_cd_update, Reg_64 = %04x", rot_comp.Reg_64);

	if (!*V_Dot)
	{
		gfx_completed();
		return;
	}

	jobs = rot_comp.Float_Part >> 16;

	if (!jobs)
	{
		rot_comp.Float_Part += rot_comp.Draw_Speed;
		return;
	}

	rot_comp.Float_Part &= 0xffff;
	rot_comp.Float_Part += rot_comp.Draw_Speed;

	while (jobs--)
	{
		if (PicoOpt & 0x1000)
			gfx_do();	// jmp [Jmp_Adr]:

		(*V_Dot)--;		// dec byte [V_Dot]

		if (!*V_Dot)
		{
			// GFX_Completed:
			gfx_completed();
			return;
		}
	}
}


unsigned int gfx_cd_read(unsigned int a)
{
	unsigned int d = 0;

	switch (a) {
		case 0x58: d = rot_comp.Reg_58; break;
		case 0x5A: d = rot_comp.Reg_5A; break;
		case 0x5C: d = rot_comp.Reg_5C; break;
		case 0x5E: d = rot_comp.Reg_5E; break;
		case 0x60: d = rot_comp.Reg_60; break;
		case 0x62: d = rot_comp.Reg_62; break;
		case 0x64: d = rot_comp.Reg_64; break;
		case 0x66: break;
		default: dprintf("gfx_cd_read FIXME: unexpected address: %02x", a); break;
	}

	dprintf("gfx_cd_read(%02x) = %04x", a, d);

	return 0;
}

void gfx_cd_write16(unsigned int a, unsigned int d)
{
	dprintf("gfx_cd_write16(%x, %04x)", a, d);

	switch (a) {
		case 0x58: // .Reg_Stamp_Size
			rot_comp.Reg_58 = d & 7;
			return;

		case 0x5A: // .Reg_Stamp_Adr
			rot_comp.Reg_5A = d & 0xffe0;
			return;

		case 0x5C: // .Reg_IM_VCell_Size
			rot_comp.Reg_5C = d & 0x1f;
			return;

		case 0x5E: // .Reg_IM_Adr
			rot_comp.Reg_5E = d & 0xFFF8;
			return;

		case 0x60: // .Reg_IM_Offset
			rot_comp.Reg_60 = d & 0x3f;
			return;

		case 0x62: // .Reg_IM_HDot_Size
			rot_comp.Reg_62 = d & 0x1ff;
			return;

		case 0x64: // .Reg_IM_VDot_Size
			rot_comp.Reg_64 = d & 0xff;	// V_Dot, must be 32bit?
			return;

		case 0x66: // .Reg_Vector_Adr
			rot_comp.Reg_66 = d & 0xfffe;
			if (Pico_mcd->s68k_regs[3]&4) return; // can't do tanformations in 1M mode
			gfx_cd_start();
			return;

		default: dprintf("gfx_cd_write16 FIXME: unexpected address: %02x", a); return;
	}
}


void gfx_cd_reset(void)
{
	memset(&rot_comp.Reg_58, 0, sizeof(rot_comp));
}


// --------------------------------

#include "cell_map.c"

typedef unsigned short u16;

// check: Heart of the alien, jaguar xj 220
void DmaSlowCell(unsigned int source, unsigned int a, int len, unsigned char inc)
{
  unsigned char *base;
  unsigned int asrc, a2;
  u16 *r;

  base = Pico_mcd->word_ram1M[Pico_mcd->s68k_regs[3]&1];

  switch (Pico.video.type)
  {
    case 1: // vram
      r = Pico.vram;
      for(; len; len--)
      {
        asrc = cell_map(source >> 2) << 2;
        asrc |= source & 2;
        // if(a&1) d=(d<<8)|(d>>8); // ??
        r[a>>1] = *(u16 *)(base + asrc);
	source += 2;
        // AutoIncrement
        a=(u16)(a+inc);
      }
      rendstatus|=0x10;
      break;

    case 3: // cram
      Pico.m.dirtyPal = 1;
      r = Pico.cram;
      for(a2=a&0x7f; len; len--)
      {
        asrc = cell_map(source >> 2) << 2;
        asrc |= source & 2;
        r[a2>>1] = *(u16 *)(base + asrc);
	source += 2;
        // AutoIncrement
        a2+=inc;
        // good dest?
        if(a2 >= 0x80) break;
      }
      a=(a&0xff00)|a2;
      break;

    case 5: // vsram[a&0x003f]=d;
      r = Pico.vsram;
      for(a2=a&0x7f; len; len--)
      {
        asrc = cell_map(source >> 2) << 2;
        asrc |= source & 2;
        r[a2>>1] = *(u16 *)(base + asrc);
	source += 2;
        // AutoIncrement
        a2+=inc;
        // good dest?
        if(a2 >= 0x80) break;
      }
      a=(a&0xff00)|a2;
      break;
  }
  // remember addr
  Pico.video.addr=(u16)a;
}
