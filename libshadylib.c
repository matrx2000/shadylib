#include "shadylib.h"

t_float scalerange (t_float a) {
	return (a - ENVELOPE_RANGE)/ENVELOPE_MAX;
}

t_float ain2reala(t_float a) {
	return exp2(log2(ENVELOPE_RANGE)*(1 - a));
}

t_int ms2samps(t_float time, t_float sr)
{
	t_int samp = (t_int)(sr*time/1000.0);
	if(samp < 1) samp = 1;
	return samp;
}

void f2axfade (t_float a, t_stage *stage, int samesamp) {
	if(a > 1.0) a = 1.0;
	else if(a < 0.0) a = 0.0;
	if(a != 1.0) { /*exponential*/
		a = ain2reala(a);
		if(stage->lin == a && samesamp) return;
		stage->lin = a;
		stage->op = exp2(log2(a)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - a);
	} else { /*linear*/
		stage->lin = 1.0;
		stage->op = 1.0;
		stage->base = 1.0/stage->nsamp;
	}
}

void ms2axfade (t_stage *stage) {
	if (stage->lin != 1.0) {
		stage->op = exp2(log2(stage->lin)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - stage->lin);
	} else {
		stage->base = 1.0/stage->nsamp;
		stage->op = 1.0; //linear
	}
}

void f2dxfade(t_float a, t_stage *stage, int samesamp) {
	if(a > 1.0) a = 1.0;
	else if(a < 0.0) a = 0.0;
	if(a != 1.0) {/*exponential*/
		a = ain2reala(a);
		if(stage->lin == scalerange(a) && samesamp) return;
		stage->lin = scalerange(a);
		stage->op = exp2(log2(a)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - stage->lin);
	} else {/*linear*/
		stage->lin = 1.0;
		stage->base = 1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

void ms2dxfade (t_stage *stage) {
	if(stage->lin != 1.0) {
		stage->op = exp2(log2(stage->lin*ENVELOPE_MAX + ENVELOPE_RANGE)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - stage->lin);
	} else {
		stage->base = 1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

void f2rxfade(t_float a, t_stage *stage, int samesamp) {
	if(a > 1.0) a = 1.0;
	else if(a < 0.0) a = 0.0;
	if(a != 1.0) {/*exponential*/
		a = ain2reala(a);
		if(stage->lin == scalerange(a) && samesamp) return;
		stage->lin = scalerange(a);
		stage->op = exp2(log2(a)/stage->nsamp);
		stage->base = stage->lin*(stage->op - 1)/(1 - stage->lin);
	} else {/*linear*/
		stage->lin = 1.0;
		stage->base = -1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

void ms2rxfade (t_stage *stage) {
	if(stage->lin != 1.0) {
		stage->op = exp2(log2(stage->lin*ENVELOPE_MAX + ENVELOPE_RANGE)/stage->nsamp);
		stage->base = stage->lin*(stage->op - 1)/(1 - stage->lin);
	} else {
		stage->base = -1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

/* needed for delay objects */
t_class *sigdelwritec_class;

void sigdelwritec_updatesr (t_sigdelwritec *x, t_float sr) /* added by Mathieu Bouchard */
{
    int nsamps = x->x_deltime * sr * (t_float)(0.001f);
    if (nsamps < 1) nsamps = 1;
    nsamps += ((- nsamps) & (SAMPBLK - 1));
    nsamps += DEFDELVS;
    if (x->x_cspace.c_n != nsamps) {
      x->x_cspace.c_vec = (t_sample *)resizebytes(x->x_cspace.c_vec,
        (x->x_cspace.c_n + XTRASAMPS) * sizeof(t_sample),
        (         nsamps + XTRASAMPS) * sizeof(t_sample));
      x->x_cspace.c_n = nsamps;
      x->x_cspace.c_phase = XTRASAMPS;
    }
}

    /* routine to check that all delwrites/delreads/vds have same vecsize */
void sigdelwritec_checkvecsize(t_sigdelwritec *x, int vecsize)
{
    if (x->x_rsortno != ugen_getsortno())
    {
        x->x_vecsize = vecsize;
        x->x_rsortno = ugen_getsortno();
    }
    /*
        LATER this should really check sample rate and blocking, once that is
        supported.  Probably we don't actually care about vecsize.
        For now just suppress this check. */
#if 0
    else if (vecsize != x->x_vecsize)
        pd_error(x, "delwritec/delreadc/vdhs vector size mismatch");
#endif
}