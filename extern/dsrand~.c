#include "m_pd.h"

//linear congruential method from "Algorithms in C" by Robert Sedgewick
#define m 16777216 // 2^24: 24 digits are 0
#define m1 4096 // 2^12
#define b 2375621 /* arbitrary number ending in ...(even digit)21
                       with 1 digit less than m */

/*below is a program to test the number generator
//chisquare: test if within 128 (2*sqrt(4096)) of 4096

#include <stdio.h>

#define m 16777216 
#define m1 4096
#define b 2375621

//you can (and should) substitute 60000 with other values greater than 40960
#define N 60000

int mult(int p, int q) {
	int p1, p0, q1, q0;
	p1 = p/m1; p0 = p%m1;
	q1 = q/m1; q0 = q%m1;
	return (((p0*q1 + p1*q0)%m1)*m1 + p0*q0)%m;
}

int randomtest(int in) {
	in = (mult(in, b) + 1)%m;
	return in;
}

int tab[m1];

int main (int argc, char **argv) {
	int state = 0, i;
	float result;
	for(i = 0; i < m1; i++) tab[i] = 0;
	for(i = 0; i < N; i++) {
		state = randomtest(state); 
		tab[state/m1]++;
	}
	state = 0;
	for(i = 0; i < m1; i++) {
		state += tab[i]*tab[i];
	}
	printf("state = %i\n", state);
	result = (((float)state)/N)*m1 - N;
	printf("result = %f\n", result);
	printf("difference: %f (absolute value should be < 128)\n", m1 - result);
}

*/

static t_class *dsrand_class;

typedef struct _dsrand {
	t_object x_obj;
	t_float x_f;
	float x_ahead; //second to last value generated
	float x_behind; //last value generated
	int x_state; //current "behind" state (int)
	t_sample x_lastin;
} t_dsrand;

// 579 = b/m1, 4037 = b%m1
static int mult(int p) {
	int p1, p0;
	p1 = p/m1; p0 = p%m1;
	return (((p0*579 + p1*4037)%m1)*m1 + p0*4037)%m;
}

// get a number from -1 to 1
static inline float ritoflt(int toflt) {
	return toflt/8388608.0f - 1;
}

static inline int randlcm(int in) {
	in = (mult(in) + 1)%m;
	return in;
}

static void dsrand_seed(t_dsrand *x, t_floatarg seed) {
	int modman = seed;
	x->x_state = modman%m;
}

static t_int *dsrand_perform(t_int *w) {
	t_dsrand  *x = (t_dsrand *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out1 = (t_sample *)(w[3]);
    t_sample *out2 = (t_sample *)(w[4]);
    int n = (int)(w[5]);
    float ahead = x->x_ahead, behind = x->x_behind;
    t_sample lastin = x->x_lastin;
    int state = x->x_state;
    while(n--) {
    	if(*in < lastin) {
    		state = randlcm(state);
    		ahead = behind;
    		behind = ritoflt(state);
    	}
    	lastin = *in++;
    	*out1++ = behind;
    	*out2++ = ahead;
    }
    x->x_ahead = ahead; x->x_behind = behind;
    x->x_lastin = lastin;
    x->x_state = state;
    return w + 6;
}

static void dsrand_dsp(t_dsrand *x, t_signal **sp) {
    dsp_add(dsrand_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec,
            sp[2]->s_vec, sp[0]->s_n);
}

static void *dsrand_new(t_floatarg f) {
	int state = f;
    t_dsrand *x = (t_dsrand *)pd_new(dsrand_class);
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_lastin = 0;
    x->x_ahead = 0;
    x->x_behind = 0;
    state %= m;
    x->x_state = state;
    return (x);
}

void dsrand_tilde_setup(void) {
    dsrand_class = class_new(gensym("dsrand~"), (t_newmethod)dsrand_new, 0,
        sizeof(t_dsrand), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(dsrand_class, t_dsrand, x_f);
    class_addmethod(dsrand_class, (t_method)dsrand_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(dsrand_class, (t_method)dsrand_seed, gensym("seed"), A_DEFFLOAT, 0);
}