// escal~ -- signal operator to round signals in three ways.
// 		by r. luke dubois (luke@music.columbia.edu), 
//			computer music center, columbia university, 2000.
//
// objects and source are provided without warranty of any kind, express or implied.
//

// include files...
#include <math.h>
#include "ext.h"
#include "z_dsp.h"

// global for the class definition
void *escal_class;

// my object structure
typedef struct _escal
{
    t_pxobject e_obj; // this is an msp object...
    long roundstate; // three-way flag for rounding operation
	void *e_proxy[1]; // a proxy inlet for setting the rounding flag
	long e_inletnumber; // inlet number for the proxy
} t_escal;

void *escal_new(long flag); // creation function
void *escal_free(t_escal *x); // free function (we use proxies, so can't just be dsp_free()
t_int *escal_perform(t_int *w); // dsp perform function
void escal_int(t_escal *x, long n); // what to do with an int
void escal_dsp(t_escal *x, t_signal **sp, short *count); // dsp add function
void escal_assist(t_escal *x, void *b, long m, long a, char *s); // assistance function

void ext_main(void* p)
{
    setup((t_messlist **)&escal_class, (method)escal_new, (method)escal_free, (short)sizeof(t_escal), 0L, A_DEFLONG, A_DEFLONG, 0);

	// declare the methods
    addmess((method)escal_dsp, "dsp", A_CANT, 0);
    addint((method)escal_int);
    addmess((method)escal_assist,"assist",A_CANT,0);
    dsp_initclass(); // required for msp

	post("escal~: by r. luke dubois, cmc");
}

// deal with the assistance strings...
void escal_assist(t_escal *x, void *b, long m, long a, char *s)
{
	switch(m) {
		case 1: // inlet
			switch(a) {
				case 0:
				sprintf(s, "(signal) Input");
				break;
				case 1:
				sprintf(s, "(int) Round Type (-1, 0, 1)");
				break;
			}
		break;
		case 2: // outlet
			switch(a) {
				case 0:
				sprintf(s, "(signal) Output");
				break;
			}
		break;
	}
}

// instantiate the object
void *escal_new(long flag)
{	
    t_escal *x = (t_escal *)newobject(escal_class);
    dsp_setup((t_pxobject *)x,1);
	x->e_proxy[0] = proxy_new(x,1L,&x->e_inletnumber);
    outlet_new((t_pxobject *)x, "signal");
	x->roundstate = 1; // initialize argument
	
	// if the arguments are there check them and set them
	if(flag) x->roundstate = flag;

    if (x->roundstate<-1) x->roundstate=-1;
    if (x->roundstate>1) x->roundstate=1;

    return (x);
}

void *escal_free(t_escal *x)
{
	freeobject(x->e_proxy[0]); // flush the proxies
	dsp_free(&x->e_obj); // flush the dsp memory
}

// this routine covers both inlets. It doesn't matter which one is involved
void escal_int(t_escal *x, long n)
{
	if (x->e_inletnumber==1) { // 'minimum' value
		x->roundstate = n;
	    if (x->roundstate<-1) x->roundstate = -1;
	    if (x->roundstate>1) x->roundstate = 1;
	}
}

// go go go...
t_int *escal_perform(t_int *w)
{
	t_escal *x = (t_escal *)w[1]; // get current state of my object class
	t_float *in,*out; // variables for input and output buffer
	int n; // counter for vector size

	// more efficient -- make local copies of class variables so i'm not constantly checking the globals...
	int flag = x->roundstate;

	int cval_new; // local variable to get polarity of current sample

	in = (t_float *)(w[2]); // my input vector
	out = (t_float *)(w[3]); // my output vector

	n = (int)(w[4]); // my vector size

	while (--n) { // loop executes a single vector
		switch(flag) {
			case(-1): // round low
				*++out = (long)*++in;
				break;
			case(0): // round normally
				if(fmod(*++in,1)>=.5) {
					*++out = (long)*++in + 1.;
				}
				else {
					*++out = (long)*++in;
				}
				break;
			case(1): // round up
				if(fmod(*++in,1)!=0) {
					*++out = (long)*++in + 1.;
				}
				else {
					*++out = (long)*++in;
				}
		}
	}

	return (w+5); // return one greater than the arguments in the dsp_add call
}

void escal_dsp(t_escal *x, t_signal **sp, short *count)
{
	// add to the dsp chain -- i need my class, my input vector, my output vector, and my vector size...
	dsp_add(escal_perform, 4, x, sp[0]->s_vec-1, sp[1]->s_vec-1, sp[0]->s_n+1);
}

