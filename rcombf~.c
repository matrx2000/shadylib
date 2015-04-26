/* recirculating comb filter */
#include "shadylib.h"
#include <string.h>

static t_class *rcombf_class;

typedef struct _rcombf
{
    t_object x_obj;
    t_float x_f;
    t_float x_sr;
    t_float x_ttime;  /* delay line length msec */
    int c_n; /* same in samples */
    int c_phase; /* current write point */
    int norm; /* whether to normalize or not */
    t_sample *c_vec; /* ring buffer for delay line */
} t_rcombf;

static void rcombf_updatesr (t_rcombf *x, t_float sr) /* added by Mathieu Bouchard */
{
    int nsamps = x->x_ttime * sr * (t_float)(0.001f);
    if (nsamps < 1) nsamps = 1;
    nsamps += ((- nsamps) & (SAMPBLK - 1));
    nsamps += DEFDELVS;
    if (x->c_n != nsamps) {
      x->c_vec = (t_sample *)resizebytes(x->c_vec,
        (x->c_n + XTRASAMPS) * sizeof(t_sample),
        (nsamps + XTRASAMPS) * sizeof(t_sample));
      x->c_n = nsamps;
      x->c_phase = XTRASAMPS;
    }
}

static t_int *rcombf_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *time = (t_sample *)(w[2]);
    t_sample *fb = (t_sample *)(w[3]);
    t_sample *norm = (t_sample *)(w[4]);
    t_sample *out = (t_sample *)(w[5]);
    t_rcombf *x = (t_rcombf *)(w[6]);
    int n = (int)(w[7]);

    int phase = x->c_phase, nsamps = x->c_n;
    int idelsamps;
    t_sample limit = nsamps - 3;
    t_sample *vp = x->c_vec, *bp, *wp = vp + phase, 
    	*ep = vp + (nsamps + XTRASAMPS);
    phase += n;
    while (n--)
    {
    	t_sample f = *in++;
        if (PD_BIGORSMALL(f))
            f = 0;
        t_sample delsamps = x->x_sr * (*time++), frac;
        
        t_sample a, b, c, d, cminusb;
        if (!(delsamps >= 1.f))    /* too small or NAN */
            delsamps = 1.f;
        if (delsamps > limit)           /* too big */
            delsamps = limit;
        idelsamps = delsamps;
        frac = delsamps - (t_sample)idelsamps;
        bp = wp - idelsamps;
        if (bp < vp + 4) bp += nsamps;
        d = bp[-3];
        c = bp[-2];
        b = bp[-1];
        a = bp[0];
        cminusb = c-b;
       delsamps = b + frac * (
            cminusb - 0.1666667f * (1.-frac) * (
                (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
            )
        );
        a = fmax(fmin(*fb++, 0x1.fffffp-1), -0x1.fffffp-1);
        b = fmax(fmin(*norm++, 1), -1);
        c = f*b + delsamps*a;
        *wp++ = c;
        *out++ = c;
        if (wp == ep)
        {
            vp[0] = ep[-4];
            vp[1] = ep[-3];
            vp[2] = ep[-2];
            vp[3] = ep[-1];
            wp = vp + XTRASAMPS;
            phase -= nsamps;
        }
    }
    x->c_phase = phase;
    return (w+8);
}

static t_int *nrcombf_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *time = (t_sample *)(w[2]);
    t_sample *fb = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    t_rcombf *x = (t_rcombf *)(w[5]);
    int n = (int)(w[6]);

    int phase = x->c_phase, nsamps = x->c_n;
    int idelsamps;
    t_sample limit = nsamps - 3;
    t_sample *vp = x->c_vec, *bp, *wp = vp + phase, 
    	*ep = vp + (nsamps + XTRASAMPS);
    phase += n;
    while (n--)
    {
    	t_sample f = *in++;
        if (PD_BIGORSMALL(f))
            f = 0;
        t_sample delsamps = x->x_sr * (*time++), frac;
        
        t_sample a, b, c, d, cminusb;
        if (!(delsamps >= 1.f))    /* too small or NAN */
            delsamps = 1.f;
        if (delsamps > limit)           /* too big */
            delsamps = limit;
        idelsamps = delsamps;
        frac = delsamps - (t_sample)idelsamps;
        bp = wp - idelsamps;
        if (bp < vp + 4) bp += nsamps;
        d = bp[-3];
        c = bp[-2];
        b = bp[-1];
        a = bp[0];
        cminusb = c-b;
       delsamps = b + frac * (
            cminusb - 0.1666667f * (1.-frac) * (
                (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
            )
        );
        a = fmax(fmin(*fb++, 0x1.fffffp-1), -0x1.fffffp-1);
        b = 1 - fabs(a);
        c = f*b + delsamps*a;
        *wp++ = c;
        *out++ = c;
        if (wp == ep)
        {
            vp[0] = ep[-4];
            vp[1] = ep[-3];
            vp[2] = ep[-2];
            vp[3] = ep[-1];
            wp = vp + XTRASAMPS;
            phase -= nsamps;
        }
    }
    x->c_phase = phase;
    return (w+7);
}

static void rcombf_dsp(t_rcombf *x, t_signal **sp)
{
	if(x->norm)
		dsp_add(nrcombf_perform, 6, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
    		sp[3]->s_vec, x, sp[0]->s_n);
    else dsp_add(rcombf_perform, 7, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
    	sp[3]->s_vec, sp[4]->s_vec, x, sp[0]->s_n);
   	x->x_sr = sp[0]->s_sr * 0.001;
    rcombf_updatesr(x, sp[0]->s_sr);
}

static void *rcombf_new(t_symbol *s, int argc, t_atom *argv)
{
	t_float time = 1000;
	t_float fb = 0;
	t_symbol *sarg;
	int norm = 0, i = 0;
    t_rcombf *x = (t_rcombf *)pd_new(rcombf_class);
    for(; i < argc; i++)
    	if (argv[i].a_type == A_FLOAT) {
    		time = atom_getfloatarg(i++, argc, argv);
    		break;
    	} else {
    		sarg = atom_getsymbolarg(i, argc, argv);
    		if (!strcmp(sarg->s_name, "-n")) norm = 1;
    		else if(!strcmp(sarg->s_name, "-l")) {
    			i++;
    			x->x_ttime = atom_getfloatarg(i, argc, argv);
    		}
		}
	for(; i < argc; i++)
    	if (argv[i].a_type == A_FLOAT) {
    		fb = atom_getfloatarg(i++, argc, argv);
    		break;
    	} else {
    		sarg = atom_getsymbolarg(i, argc, argv);
    		if (!strcmp(sarg->s_name, "-n")) norm = 1;
    		else if(!strcmp(sarg->s_name, "-l")) {
    			i++;
    			x->x_ttime = atom_getfloatarg(i, argc, argv);
    		}
		}
	for(; i < argc; i++)
    	if (argv[i].a_type == A_FLOAT) {
    		x->x_ttime = atom_getfloatarg(i, argc, argv);
    	} else {
    		sarg = atom_getsymbolarg(i, argc, argv);
    		if (!strcmp(sarg->s_name, "-n")) norm = 1;
		}
    pd_float((t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, 	
    	&s_signal), time);
    pd_float((t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, 	
    	&s_signal),fb);
	if(!norm)
		pd_float((t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, 
			&s_signal), 1.f);
    outlet_new(&x->x_obj, &s_signal);
    x->norm = norm;
    return(x);
}

static void rcombf_clear(t_rcombf *x) {
	memset((char *)(x->c_vec), 0, 
		sizeof(t_sample)*(x->c_n + XTRASAMPS));
}

static void rcombf_free(t_rcombf *x)
{
    freebytes(x->c_vec,
        (x->c_n + XTRASAMPS) * sizeof(t_sample));
}

void rcombf_tilde_setup(void)
{
    rcombf_class = class_new(gensym("rcombf~"), 
        (t_newmethod)rcombf_new, (t_method)rcombf_free,
        sizeof(t_rcombf), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(rcombf_class, t_rcombf, x_f);
    class_addmethod(rcombf_class, (t_method)rcombf_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(rcombf_class, (t_method)rcombf_clear,
    	gensym("clear"), 0);
}