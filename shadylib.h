#include "m_pd.h"
#include <math.h>

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

#ifdef IRIX
#include <sys/endian.h>
#endif

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
#include <machine/endian.h>
#endif

#if defined(__linux__) || defined(__CYGWIN__) || defined(__GNU__) || defined(ANDROID)
#include <endian.h>
#endif

#ifdef __MINGW32__
#include <sys/param.h>
#endif

#ifdef _MSC_VER
/* _MSVC lacks BYTE_ORDER and LITTLE_ENDIAN */
#define LITTLE_ENDIAN 0x0001
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)                         
#error No byte order defined                                                    
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define HIOFFSET 1     
# define LOWOFFSET 0                                                       
#else                                                                           
# define HIOFFSET 0    /* word offset to find MSB */                             
# define LOWOFFSET 1    /* word offset to find LSB */                            
#endif

#define TRUE 1
#define FALSE 0

#define NORMHIPART 1094189056 //will this work on big-endian?

union shadylib_tabfudge
{
    double tf_d;
    int32_t tf_i[2];
};

/* union for setting floats or receiving signals */
union shadylib_floatpoint
{
	t_sample *vec;
	t_float val;
};

typedef struct oscctl
{
	union shadylib_floatpoint invals[2];
	int num; /* number of msg inlets: 0, 1, or 2 
		0 is all signals, 1 is an add inlet,
		2 is a multiply inlet and an add inlet */
} shadylib_t_oscctl;

typedef struct delwritectl
{
    int c_n;
    t_sample *c_vec;
    int c_phase;
} shadylib_t_delwritectl;

typedef struct _sigdelwritec
{
    t_object x_obj;
    t_symbol *x_sym;
    t_float x_deltime;  /* delay in msec (added by Mathieu Bouchard) */
    shadylib_t_delwritectl x_cspace;
    int x_sortno;   /* DSP sort number at which this was last put on chain */
    int x_rsortno;  /* DSP sort # for first delread or write in chain */
    int x_vecsize;  /* vector size for delread~ to use */
    t_float x_f;
} t_sigdelwritec;

EXTERN int ugen_getsortno(void);
EXTERN void sigdelwritec_checkvecsize(t_sigdelwritec *x, int vecsize);
EXTERN void sigdelwritec_updatesr (t_sigdelwritec *x, t_float sr);
#define XTRASAMPS 4
#define SAMPBLK 4
#define DEFDELVS 64            /* LATER get this from canvas at DSP time */

EXTERN t_class *sigdelwritec_class;

/*
 *   utility functions used in pd EXTERNals 
 *   Copyright (c) 2000-2014 by Tom Schouten & Seb Shader
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef PD_FLOAT_PRECISION
#define PD_FLOAT_PRECISION 32
#endif

#if defined(__i386__) || defined(__x86_64__) // type punning code:

#if PD_FLOAT_PRECISION == 32

typedef union
{
    unsigned int i;
    t_float f;
} shadylib_t_flint;

/* check if floating point number is denormal */

//#define IS_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) == 0) 

#define IS_DENORMAL(f) (((((shadylib_t_flint)(f)).i) & 0x7f800000) == 0)

#elif PD_FLOAT_PRECISION == 64

typedef union
{
    unsigned int i[2];
    t_float f;
} shadylib_t_flint;

#define IS_DENORMAL(f) (((((t_flint)(f)).i[1]) & 0x7ff00000) == 0)

#endif // endif PD_FLOAT_PRECISION

#else   // if not defined(__i386__) || defined(__x86_64__)
#define IS_DENORMAL(f) 0
#endif // end if defined(__i386__) || defined(__x86_64__)

EXTERN void shadylib_checkalign(void);

/* exponential range for envelopes is 60dB */
#define ENVELOPE_RANGE 0.001
#define ENVELOPE_MAX   (1.0 - ENVELOPE_RANGE)

typedef struct _stage {
	t_float lin; // 0.001 for fully exponential (60 db), 1.0 for fully linear
	t_float op;  // geometric multiplier or linear subtraction (if lin == 1.0)
	t_float base;// addition for each stage
	t_int nsamp; // # of samples in stage
} shadylib_t_stage;     

EXTERN t_class *sigdelwritec_class;

EXTERN t_int shadylib_ms2samps(t_float time, t_float sr);
EXTERN void shadylib_f2axfade (t_float a, shadylib_t_stage *stage, int samesamp);
EXTERN void shadylib_ms2axfade (shadylib_t_stage *stage);
EXTERN void shadylib_f2dxfade(t_float a, shadylib_t_stage *stage, int samesamp);
EXTERN void shadylib_ms2dxfade (shadylib_t_stage *stage);
EXTERN void shadylib_f2rxfade(t_float a, shadylib_t_stage *stage, int samesamp);
EXTERN void shadylib_ms2rxfade (shadylib_t_stage *stage);

#define SHABLESIZE 2048 /* size of tables in shadylook~ */

typedef enum _tabtype {
	REXP,
	GAUS,
	CAUCH
} shadylib_t_tabtype;

EXTERN t_float shadylib_readtab(shadylib_t_tabtype type, t_float index);

EXTERN void shadylib_maketabs(void);
EXTERN void shadylib_freetabs(t_class *dummy);

/*buzz stuff */
EXTERN void shadylib_makebuzz(void);
EXTERN void shadylib_freebuzz(t_class *dummy);

EXTERN t_int *shadylib_opd_perf0(t_int *w);
EXTERN t_int *shadylib_opd_perf1(t_int *w);
EXTERN t_int *shadylib_opd_perf2(t_int *w);

EXTERN t_int *shadylib_recd_perf0(t_int *w);
EXTERN t_int *shadylib_recd_perf1(t_int *w);
EXTERN t_int *shadylib_recd_perf2(t_int *w);

EXTERN t_int *shadylib_trid_perf0(t_int *w);
EXTERN t_int *shadylib_trid_perf1(t_int *w);
EXTERN t_int *shadylib_trid_perf2(t_int *w);

EXTERN unsigned char shadylib_aligned;
EXTERN t_sample *shadylib_sintbl;
EXTERN t_sample *shadylib_cosectbl;

/* used in the cosecant table for values very close to 1/0 */
#define BADVAL 1e20f

/* size of tables */
#define BUZZSIZE 8192

/* maximum harmonics for small frequencies */
#define MAXHARM ((int)((4294967295/(2*BUZZSIZE)) - 2))

#include <string.h>

#ifdef _WIN32
# include <malloc.h> /* MSVC or mingw on windows */
#elif defined(__linux__) || defined(__APPLE__)
# include <alloca.h> /* linux, mac, mingw, cygwin */
#else
# include <stdlib.h> /* BSDs for example */
#endif

#ifndef HAVE_ALLOCA     /* can work without alloca() but we never need it */
#define HAVE_ALLOCA 1
#endif

#define LIST_NGETBYTE 100 /* bigger that this we use alloc, not alloca */

/* -------------- utility functions: storage, copying  -------------- */
    /* List element for storage.  Keep an atom and, in case it's a pointer,
        an associated 'gpointer' to protect against stale pointers. */
typedef struct _listelem
{
    t_atom l_a;
    t_gpointer l_p;
} t_listelem;

typedef struct _alist
{
    t_pd l_pd;          /* object to point inlets to */
    int l_n;            /* number of items */
    int l_npointer;     /* number of pointers */
    t_listelem *l_vec;  /* pointer to items */
} t_alist;

#if HAVE_ALLOCA
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)((n) < LIST_NGETBYTE ?  \
        alloca((n) * sizeof(t_atom)) : getbytes((n) * sizeof(t_atom))))
#define ATOMS_FREEA(x, n) ( \
    ((n) < LIST_NGETBYTE || (freebytes((x), (n) * sizeof(t_atom)), 0)))
#else
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)getbytes((n) * sizeof(t_atom)))
#define ATOMS_FREEA(x, n) (freebytes((x), (n) * sizeof(t_atom)))
#endif

EXTERN void atoms_copy(int argc, t_atom *from, t_atom *to);
EXTERN t_class *alist_class;
EXTERN void alist_init(t_alist *x);
EXTERN void alist_clear(t_alist *x);
EXTERN void alist_copyin(t_alist *x, t_symbol *s, int argc, t_atom *argv,
    int where);
EXTERN void alist_list(t_alist *x, t_symbol *s, int argc, t_atom *argv);
EXTERN void alist_anything(t_alist *x, t_symbol *s, int argc, t_atom *argv);
EXTERN void alist_toatoms(t_alist *x, t_atom *to, int onset, int count);
EXTERN void alist_clone(t_alist *x, t_alist *y, int onset, int count);
EXTERN void alist_setup(void);