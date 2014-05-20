#include "m_pd.h"
#include "math.h"

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
//# define LOWOFFSET 0                                                       
#else                                                                           
# define HIOFFSET 0    /* word offset to find MSB */                             
//# define LOWOFFSET 1    /* word offset to find LSB */                            
#endif

#ifdef _MSC_VER
 typedef __int32 int32_t; /* use MSVC's internal type */
#elif defined(IRIX)
 typedef long int32_t;  /* a data type that has 32 bits */
#else
# include <stdint.h>  /* this is where int32_t is defined in C99 */
#endif

#define TRUE 1
#define FALSE 0

union tabfudge
{
    double tf_d;
    int32_t tf_i[2];
};

/* -------------------------- sampphase~ ------------------------------ */
static t_class *sampphase_class;

#if 1   /* in the style of R. Hoeldrich (ICMC 1995 Banff) */

typedef struct _sampphase
{
    t_object x_obj;
    unsigned char x_samptrue;
    double x_phase;
    float x_conv;
    float x_held;
    float x_f;      /* scalar frequency */
} t_sampphase;

static void *sampphase_new(t_floatarg f, t_floatarg t)
{
    t_sampphase *x = (t_sampphase *)pd_new(sampphase_class);
    x->x_samptrue = (t == 0);
    x->x_f = f;
    x->x_held = f;
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_phase = 0;
    x->x_conv = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    return (x);
}

static t_int *sampphase_perform(t_int *w)
{
    t_sampphase *x = (t_sampphase *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out1 = (t_float *)(w[3]);
    t_float *out2 = (t_float *)(w[4]);
    int n = (int)(w[5]);
    unsigned char bool = x->x_samptrue;
    union tabfudge tf;
    tf.tf_d = UNITBIT32;
    int normhipart = tf.tf_i[HIOFFSET];
    tf.tf_d = x->x_phase + (double)UNITBIT32;
    
    t_float conv = x->x_conv;
	
    if (bool) {
    	t_float samp = x->x_held;
    	while (n--) {
    		tf.tf_d += samp * conv;
    		if ((tf.tf_i[HIOFFSET] != normhipart) ||\
				 (samp == 0.0 && *in != 0.0)) samp = *in;
			in++;
			tf.tf_i[HIOFFSET] = normhipart;
    		*out1++ = tf.tf_d - UNITBIT32;
    		*out2++ = samp;
		}
		x->x_held = samp;
    } else {
    	while (n--) {
    		tf.tf_d += *in * conv;
    		tf.tf_i[HIOFFSET] = normhipart;
    		*out2++ = *in++;
    		*out1++ = tf.tf_d - UNITBIT32;
    	}
	}
    x->x_phase = out1[-1];
    return (w+6);
}

static void sampphase_dsp(t_sampphase *x, t_signal **sp)
{ 
	
    
    x->x_conv = 1./sp[0]->s_sr;
    dsp_add(sampphase_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

static void sampphase_ft1(t_sampphase*x, t_float f)
{
    x->x_phase = f;
}

static void sampphase_hold(t_sampphase*x, t_float f)
{
    x->x_samptrue = (f != 0.);
}

void sampphase_tilde_setup(void)
{
    sampphase_class = class_new(gensym("sampphase~"), (t_newmethod)sampphase_new, 0, sizeof(t_sampphase), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(sampphase_class, t_sampphase, x_f);
    class_addmethod(sampphase_class, (t_method)sampphase_dsp, gensym("dsp"), 0);
    class_addmethod(sampphase_class, (t_method)sampphase_ft1,
        gensym("ft1"), A_FLOAT, 0);
    class_addmethod(sampphase_class, (t_method)sampphase_hold,
        gensym("hold"), A_FLOAT, 0);
}

#endif  /* Hoeldrich version */
