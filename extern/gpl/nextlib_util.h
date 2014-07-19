/*
 *   utility functions used in pd externals 
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
 
#include <math.h>
#include "m_pd.h"

#ifndef PD_FLOAT_PRECISION
#define PD_FLOAT_PRECISION 32
#endif

#if defined(__i386__) || defined(__x86_64__) // type punning code:

#if PD_FLOAT_PRECISION == 32

typedef union
{
    unsigned int i;
    t_float f;
} t_flint;

/* check if floating point number is denormal */

//#define IS_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) == 0) 

#define IS_DENORMAL(f) (((((t_flint)(f)).i) & 0x7f800000) == 0)

#elif PD_FLOAT_PRECISION == 64

typedef union
{
    unsigned int i[2];
    t_float f;
} t_flint;

#define IS_DENORMAL(f) (((((t_flint)(f)).i[1]) & 0x7ff00000) == 0)

#endif // endif PD_FLOAT_PRECISION

#else   // if not defined(__i386__) || defined(__x86_64__)
#define IS_DENORMAL(f) 0
#endif // end if defined(__i386__) || defined(__x86_64__)


/* exponential range for envelopes is 60dB */
#define ENVELOPE_RANGE 0.001
#define ENVELOPE_MAX   (1.0 - ENVELOPE_RANGE)

typedef struct _stage {
	t_float lin; //0.001 for fully exponential (60 db), 1.0 for fully linear
	t_float op;  //geometric multiplier or linear subtraction (if lin == 1.0)
	t_int nsamp; //# of samples in stage
} t_stage;        

static inline t_float scalerange (t_float a) {
	return (a - ENVELOPE_RANGE)/ENVELOPE_MAX;
}

static void ain2reala(t_float *a) {
	*a = exp((1 - *a)*log(ENVELOPE_RANGE));
}

static void ms2samps(t_float time, t_stage *stage)
{
	t_int samp = (t_int)(sys_getsr() * time/1000.0);
	if(samp < 1) samp = 1;
	stage->nsamp = samp;
}

static char f2axfade (t_float a, t_stage *stage) {
	if(a > 1.0) a = 1.0;
	else if(a < 0.0) a = 0.0;
	if(a != 1.0) { /*exponential*/
		ain2reala(&a);
		if(stage->lin != a) stage->lin = a;
		else return 0;
		stage->op = exp(log(stage->lin) / stage->nsamp);
		return 1;
	} else { /*linear*/
		if(stage->lin != 1.0)stage->lin = 1.0;
		else return 0;
		stage->op = 1.0/stage->nsamp;
		return 1;
	}
}

static void ms2axfade (t_float time, t_stage *stage) {
	t_int samp = (t_int)(sys_getsr() * time/1000.0);
	if(samp < 1) samp = 1;
	stage->nsamp = samp;
	if (stage->lin != 1.0) stage->op = exp(log(stage->lin) / samp);
	else stage->op = 1.0/samp; //linear
}

static char f2drxfade(t_float a, t_stage *stage) {
	if(a > 1.0) a = 1.0;
	else if(a < 0.0) a = 0.0;
	if(a != 1.0) {/*exponential*/
		ain2reala(&a);
		if(stage->lin != scalerange(a)) stage->lin = scalerange(a);
		else return 0;
		stage->op = exp(log(a) / stage->nsamp);
		return 1;
	}
	else {/*linear*/
		if(stage->lin != 1.0)stage->lin = 1.0;
		else return 0;
		stage->op = 1.0/stage->nsamp;
		return 1;
	}
}

static void ms2drxfade (t_float time, t_stage *stage) {
	t_int samp = (t_int)(sys_getsr() * time/1000.0);
	if(samp < 0) samp = 0;
	stage->nsamp = samp;
	if(stage->lin != 1.0) stage->op = exp(log(stage->lin*ENVELOPE_MAX + ENVELOPE_RANGE) / samp);
	else stage->op = 1.0/samp;
}