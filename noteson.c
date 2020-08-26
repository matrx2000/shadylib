#include "m_pd.h"

/* from x_list.c in pd sources */
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

#if HAVE_ALLOCA
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)((n) < LIST_NGETBYTE ?  \
        alloca((n) * sizeof(t_atom)) : getbytes((n) * sizeof(t_atom))))
#define ATOMS_FREEA(x, n) ( \
    ((n) < LIST_NGETBYTE || (freebytes((x), (n) * sizeof(t_atom)), 0)))
#else
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)getbytes((n) * sizeof(t_atom)))
#define ATOMS_FREEA(x, n) (freebytes((x), (n) * sizeof(t_atom)))
#endif

static t_class *noteson_class;

typedef struct _notesonelem
{
    struct _notesonelem *e_next;
    float e_value;
} t_notesonelem;

typedef struct _noteson
{
    t_object x_obj;
    float x_velo;
	int mode;
	int size;
    t_notesonelem *x_first;
} t_noteson;

static void noteson_mode(t_noteson *x, t_float mode) {
	int imode = mode;
	if(imode < 0) imode = 0;
	else if(imode > 2) imode = 2;
	x->mode = imode;
}

static void *noteson_new(t_floatarg mode)
{
    t_noteson *x = (t_noteson *)pd_new(noteson_class);
    x->x_velo = 0;
    floatinlet_new(&x->x_obj, &x->x_velo);
    outlet_new(&x->x_obj, &s_list);
    x->x_first = 0;
	noteson_mode(x, mode);
    return (x);
}

static void noteson_highest(t_noteson *x, t_notesonelem *insert) {
	t_notesonelem *current;
	if(insert->e_value > x->x_first->e_value) {
		insert->e_next = x->x_first;
		x->x_first = insert;
		return;
	}
	current = x->x_first;
	while(current->e_next && current->e_next->e_value > insert->e_value)
		current = current->e_next;
	insert->e_next = current->e_next;
	current->e_next = insert;
}

static void noteson_lowest(t_noteson *x, t_notesonelem *insert) {
	t_notesonelem *current;
	if(insert->e_value < x->x_first->e_value) {
		insert->e_next = x->x_first;
		x->x_first = insert;
		return;
	}
	current = x->x_first;
	while(current->e_next && current->e_next->e_value < insert->e_value)
		current = current->e_next;
	insert->e_next = current->e_next;
	current->e_next = insert;
}


static void noteson_float(t_noteson *x, t_float f)
{
    t_notesonelem *notesonelem, *e2, *e3;
	t_atom *outv;
	int outc;
    if (x->x_velo != 0)
    {
        notesonelem = (t_notesonelem *)getbytes(sizeof *notesonelem);
        notesonelem->e_value = f;
		x->size++;
        if (!x->x_first) {
			x->x_first = notesonelem;
			notesonelem->e_next = 0;
		} else    /* LATER replace with a faster algorithm */
			switch(x->mode) {
				case 0:
					notesonelem->e_next = x->x_first;
					x->x_first = notesonelem;
					break;
				case 1:
					noteson_highest(x, notesonelem);
					break;
				case 2:
					noteson_lowest(x, notesonelem);
			}
    }
    else
    {
        if (x->x_first) {
			if (x->x_first->e_value == f)
		    {
			    notesonelem = x->x_first;
		        x->x_first = x->x_first->e_next;
				x->size--;
				freebytes(notesonelem, sizeof(*notesonelem));
			} else
				for (e2 = x->x_first; (e3 = e2->e_next); e2 = e3)
					if (e3->e_value == f) {
						e2->e_next = e3->e_next;
						freebytes(e3, sizeof(*e3));
						x->size--;
					}
        }
    }
	outc = x->size;
	e2 = x->x_first;
	ATOMS_ALLOCA(outv, outc);
	for (int n = 0; n < outc; n++, e2 = e2->e_next)
        SETFLOAT(outv + n, e2->e_value);
	outlet_list(x->x_obj.ob_outlet, &s_list, outc, outv);
    ATOMS_FREEA(outv, outc);
}

static void noteson_clear(t_noteson *x)
{
    t_notesonelem *notesonelem;
    while ((notesonelem = x->x_first))
    {
        x->x_first = notesonelem->e_next;
        freebytes(notesonelem, sizeof(*notesonelem));
    }
	x->size = 0;
}

static void noteson_flush(t_noteson *x)
{
    noteson_clear(x);
	outlet_bang(x->x_obj.ob_outlet);
}

void noteson_setup(void)
{
    noteson_class = class_new(gensym("noteson"),
        (t_newmethod)noteson_new, (t_method)noteson_clear,
        sizeof(t_noteson), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addfloat(noteson_class, noteson_float);
    class_addmethod(noteson_class, (t_method)noteson_flush, gensym("flush"), 0);
    class_addmethod(noteson_class, (t_method)noteson_clear, gensym("clear"), 0);
	class_addmethod(noteson_class, (t_method)noteson_mode, gensym("mode"), A_DEFFLOAT, 0);
}

