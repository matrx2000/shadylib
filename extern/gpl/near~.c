/*
 *   near~.c  -  exponential attack decay envelope 
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

#include <string.h>
#include "nextlib_util.h"

/* pointer to */
t_class *near_class;


/* state data fpr attack/decay dsp plugin */
typedef struct nearctl {
	t_stage c_attack;
	t_stage c_release;
	t_float c_cattack; /* current attack pole (for non-direct) */
	t_float c_state;
	char c_target;
} t_nearctl;

typedef struct near {
	t_object x_obj;
	t_nearctl x_ctl;
	t_float x_sr;
	char direct; /*boolean*/
} t_near;

static void near_float(t_near *x, t_floatarg f) {
	t_float state = x->x_ctl.c_state;
	 if(f == 0.0) {
    	x->x_ctl.c_target = 0;
    } else {
    	x->x_ctl.c_target = 1;
    	if(f > 0.0) {
    		if(x->x_ctl.c_attack.lin != 1.0) {
    			t_float estate = 1 + state*(x->x_ctl.c_attack.lin - 1);
    			if(x->direct) x->x_ctl.c_cattack = x->x_ctl.c_attack.op;
    			else x->x_ctl.c_cattack = (x->x_ctl.c_attack.op/(exp(log(estate)/x->x_ctl.c_attack.nsamp)));
			} else {
				if(x->direct) x->x_ctl.c_cattack = x->x_ctl.c_attack.op;
				else x->x_ctl.c_cattack = ((1 - x->x_ctl.c_state)/x->x_ctl.c_attack.nsamp);
			}
		} else {
    		x->x_ctl.c_state = 0.0;
    		x->x_ctl.c_cattack = x->x_ctl.c_attack.op;
    	}
    }
}

static void near_attack(t_near *x, t_symbol *s, int argc, t_atom *argv) {
	switch(argc) {
		default:;
		case 2: 
			ms2samps(atom_getfloat(argv), &(x->x_ctl.c_attack));
			if(f2axfade(atom_getfloat(argv), &(x->x_ctl.c_attack)))
					if(x->x_ctl.c_target == 1.0) {
						char tdirect = x->direct;
						x->direct = 1;
						near_float(x, 1.0);
						x->direct = tdirect;
					}
			break;
		case 1: ms2axfade(atom_getfloat(argv), &(x->x_ctl.c_attack));
		case 0:;
	}
}

static void near_release(t_near *x, t_symbol *s, int argc, t_atom *argv) {
	switch(argc) {
		default:;
		case 2:
			ms2samps(atom_getfloat(argv), &(x->x_ctl.c_release));
			f2drxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_release));
			break;
		case 1: ms2drxfade(atom_getfloat(argv), &(x->x_ctl.c_release));
		case 0:;
	}
}

static void near_any(t_near *x, t_symbol *s, int argc, t_atom *argv) {
	if(!strcmp(s->s_name, "curves")) { 
		switch(argc) {
			default:;
			case 2: if(argv[1].a_type == A_FLOAT) {
				f2drxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_release));
			}
			case 1: if(argv[0].a_type == A_FLOAT) {
				if(f2axfade(atom_getfloat(argv), &(x->x_ctl.c_attack)))
					if(x->x_ctl.c_target == 1.0) {
						char tdirect = x->direct;
						x->direct = 1;
						near_float(x, 1.0);
						x->direct = tdirect;
					}
			}
			case 0:;
		}
	} else {
		switch(argc) {
			default:;
			case 2: 
				if(argv[1].a_type == A_FLOAT) ms2drxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_release));
			case 1: 
				if(argv[0].a_type == A_FLOAT) ms2axfade(atom_getfloat(argv), &(x->x_ctl.c_attack));
			case 0:;
		}
	}
}

static void near_direct(t_near *x, t_floatarg f) {
	x->direct = (char)(f == 0.0);
}

/* dsp callback function, not a method */
static t_int *near_perform(t_int *w)
{

  /* interprete arguments */
    t_float *out    = (t_float *)(w[3]);
    t_nearctl *ctl  = (t_nearctl *)(w[1]);
    t_float state   = ctl->c_state;
    char target = ctl->c_target;
    t_int n = (t_int)(w[2]);

    
	if(target){
		t_float cattack = ctl->c_cattack;
		if(state == 1.0) {
			while(n--) *out++ = 1.0;
		} else if (ctl->c_attack.lin != 1.0) {
			t_float mlin = (1 - cattack)/(1 - ctl->c_attack.lin);
			while(n--){
				*out++ = state;
				state = state*cattack + mlin;
				if(state >= 1.0) {
					state = 1.0;
					while (n) {
						n--;
						*out++ = state;
					}
				}
			}
		} else {
			while(n){
				n--;
				*out++ = state;
				state += cattack;
				if(state >= 1.0) {
					state = 1.0;
					while (n) {
						n--;
						*out++ = state;
					}
				}
			}
		}
	} else {
		/*release*/
		t_stage release = ctl->c_release;
		if(state == 0.0) while(n--) *out++ = 0.0;
		else if (release.lin != 1.0) {
			t_float mlin = release.lin*(release.op - 1)/(1 - release.lin);
			while(n--){
				*out++ = state;
				state = state*release.op + mlin;
				if(state <= 0.0) {
					state = 0.0;
					while(n) {
						n--;
						*out++ = state;
					}
				}
			}
		} else {
			while(n--){
				*out++ = state;
				state = state - release.op;
				if(state <= 0.0) {
					state = 0.0;
					while(n) {
						n--;
						*out++ = state;
					}
				}
			}
		}
	}
    /* save state */
    ctl->c_state = IS_DENORMAL(state) ? 0 : state;
	ctl->c_target = target;
    return (w+4); 
}


void near_dsp(t_near *x, t_signal **sp) {
  dsp_add(near_perform, 3, &x->x_ctl, sp[0]->s_n, sp[0]->s_vec);
}            

t_class *near_class;                      
 
/* constructor */
void *near_new(t_floatarg attack, t_floatarg release, t_floatarg direct)
{
    t_near *x = (t_near *)pd_new(near_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("attack"));  
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("release"));  
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_ctl.c_state = 0;
    x->x_ctl.c_target = 0;
    near_direct(x, direct);
    ms2samps(attack, &(x->x_ctl.c_attack));
    f2axfade(1-(log(1.0/3.0)/log(ENVELOPE_RANGE)), &(x->x_ctl.c_attack)); /* 1/3 by default */
    ms2samps(release, &(x->x_ctl.c_release));
    f2drxfade(0.0, &(x->x_ctl.c_release));
	return (void *)x;
}

void near_tilde_setup(void)
{
    near_class = class_new(gensym("near~"), (t_newmethod)near_new,
    	0, sizeof(t_near), 0,  A_DEFFLOAT, 
			    A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(near_class, (t_method)near_float,
		    gensym("float"), A_FLOAT, 0);
    class_addmethod(near_class, (t_method)near_dsp, gensym("dsp"), 0); 
    class_addmethod(near_class, (t_method)near_attack,
		    gensym("attack"), A_GIMME, 0);
    class_addmethod(near_class, (t_method)near_release,
		    gensym("release"), A_GIMME, 0);
   class_addmethod(near_class, (t_method)near_direct,
		    gensym("ndirect"), A_FLOAT, 0);
	class_addanything(near_class, (t_method)near_any);
}