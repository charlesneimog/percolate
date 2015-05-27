#include "ext.h"
#include "z_dsp.h"
#include "buffer.h"

void *pokef_class;

typedef struct _pokef
{
    t_pxobject l_obj;
    t_symbol *l_sym;
    t_buffer *l_buf;
    long l_chan;
    long length;

    
} t_pokef;

long Constrain(long v, long lo, long hi);
t_int *pokef_perform(t_int *w);
void pokef_dsp(t_pokef *x, t_signal **sp);
void pokef_set(t_pokef *x, t_symbol *s);
void *pokef_new(t_symbol *s, long chan);
void pokef_in1(t_pokef *x, long n);
void pokef_assist(t_pokef *x, void *b, long m, long a, char *s);
void pokef_dblclick(t_pokef *x);
void pokef_length(t_pokef *x, Symbol *s, short argc, Atom *argv);

t_symbol *ps_buffer;

void ext_main(void* p)
{
	setup((struct messlist **)&pokef_class, (method)pokef_new, (method)dsp_free, (short)sizeof(t_pokef), 0L, 
		A_SYM, A_DEFLONG, 0);
	addmess((method)pokef_dsp, "dsp", A_CANT, 0);
	addmess((method)pokef_set, "set", A_SYM, 0);
	addmess((method)pokef_length, "length", A_GIMME, 0);
	addinx((method)pokef_in1,3);
	addmess((method)pokef_assist, "assist", A_CANT, 0);
	addmess((method)pokef_dblclick, "dblclick", A_CANT, 0);
	dsp_initclass();
	ps_buffer = gensym("buffer~");
	rescopy('STR#',19987);
}

long Constrain(long v, long lo, long hi)
{
	if (v < lo)
		return lo;
	else if (v > hi)
		return hi;
	else
		return v;
}

t_int *pokef_perform(t_int *w)
{
    t_pokef *x = (t_pokef *)(w[1]);
    float *in = (float *)(w[2]);
    float *index = (float *)(w[3]);
    float *fcoeff = (float *)(w[4]);
    float *out = (float *)(w[5]);
    int n = (int)(w[6]);
    
    double alpha, om_alpha, output;
    
	t_buffer *b = x->l_buf;
	float *tab;
	float temp, input, coeff, bufsample;
	float chan, frames, nc, length;
	
	long pokef, pokef_next, pokef_nextnext, pokef_nextnextnext;
	
	if (x->l_obj.z_disabled)
		goto out;
	if (!b)
		goto zero;
	if (!b->b_valid)
		goto zero;
		
		
	tab = b->b_samples;
	chan = (float)x->l_chan;
	frames = (float)b->b_frames;
	nc = (float)b->b_nchans;
	length = (float)x->length;
	if (length <= 0.) length = frames;
	else if (length >= frames) length = frames;
	
	while (n--) {
		input 	= *in++;
		temp 	= *index++;
		coeff 	= *fcoeff++;
				
		temp += 0.5;
		
		if (temp < 0.)
			temp = 0.;
		else while (temp >= length)
			temp -= length;
	
		temp = temp * nc + chan;
			
		pokef = (long)temp;	
		//bufsample = tab[pokef];
						
		alpha = temp - (double)pokef;
		om_alpha = 1. - alpha;
		
		pokef_next = pokef + nc;
		while (pokef_next >= length*nc) pokef_next -= length*nc;
		pokef_nextnext = pokef_next + nc;
		while (pokef_nextnext >= length*nc) pokef_nextnext -= length*nc;
		pokef_nextnextnext = pokef_nextnext + nc;
		while (pokef_nextnextnext >= length*nc) pokef_nextnextnext -= length*nc;
		
		//output two ahead of record point...., with interpolation
		//*out++ = tab[pokef_nextnext]*om_alpha + tab[pokef_nextnextnext]*alpha;
		//*out++ = tab[pokef_next]*om_alpha + tab[pokef_nextnext]*alpha;
		*out++ = tab[pokef_next];
		
		//interpolate recording...
		//tab[pokef] = coeff * tab[pokef] + om_alpha*input;
		//tab[pokef_next] = coeff * tab[pokef_next] + alpha*input;
		//or not....
		tab[pokef] = coeff * tab[pokef] + input;

		//*out++ = bufsample;
		//tab[pokef] = coeff * bufsample + input;

	}

zero:
out:
	return (w+7);
}

void pokef_set(t_pokef *x, t_symbol *s)
{
	t_buffer *b;
	
	x->l_sym = s;
	if ((b = (t_buffer *)(s->s_thing)) && ob_sym(b) == ps_buffer) {
		x->l_buf = b;
	} else {
		error("pokef~: no buffer~ %s", s->s_name);
		x->l_buf = 0;
	}
}

void pokef_in1(t_pokef *x, long n)
{
	if (n)
		x->l_chan = Constrain(n,1,4) - 1;
	else
		x->l_chan = 0;
}

void pokef_dsp(t_pokef *x, t_signal **sp)
{
    pokef_set(x,x->l_sym);
    //dsp_add(pokef_perform, 5, x, sp[0]->s_vec-1, sp[1]->s_vec-1, sp[2]->s_vec-1, sp[0]->s_n+1);
    dsp_add(pokef_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}

void pokef_length(t_pokef *x, Symbol *s, short argc, Atom *argv)
{
	short i;
	int temp;
	t_buffer *b = x->l_buf;
	for (i=0; i < argc; i++) {
		switch (argv[i].a_type) {
			case A_LONG:
				temp = argv[i].a_w.w_long;
				x->length = temp;
				break;
			case A_FLOAT:
				temp = (int)argv[i].a_w.w_float;
				x->length = temp;
				break;
		}
	}
}

void pokef_dblclick(t_pokef *x)
{
	t_buffer *b;
	
	if ((b = (t_buffer *)(x->l_sym->s_thing)) && ob_sym(b) == ps_buffer)
		mess0((struct object *)b,gensym("dblclick"));
}

void pokef_assist(t_pokef *x, void *b, long m, long a, char *s)
{
	assist_string(19987,m,a,1,4,s);
}

void *pokef_new(t_symbol *s, long chan)
{
	t_pokef *x = (t_pokef *)newobject(pokef_class);
	//dsp_setup((t_pxobject *)x, 1);
    intin((t_object *)x, 3);
	dsp_setup((t_pxobject *)x, 3);
	outlet_new((t_object *)x, "signal");
	x->l_sym = s;
	pokef_in1(x,chan);
	//x->l_obj.z_misc = Z_NO_INPLACE;
	return (x);
}

