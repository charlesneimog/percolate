// rnadom~ -- generates random numbers in a positive range at signal rate.
// 		by r. luke dubois (luke@music.columbia.edu), 
//			computer music center, columbia university, 2000.
//
// objects and source are provided without warranty of any kind, express or implied.
//

// include files...
#include <stdlib.h>
#include "ext.h"
#include "z_dsp.h"

// global for the class definition
void *random_class;

// my object structure
typedef struct _random
{
    t_pxobject r_obj; // this is an msp object...
    long range; // range of random generation
	void *r_proxy[1]; // a proxy inlet for setting the rounding flag
	long r_inletnumber; // inlet number for the proxy
} t_random;

void *random_new(long flag); // creation function
void *random_free(t_random *x); // free function (we use proxies, so can't just be dsp_free()
t_int *random_perform(t_int *w); // dsp perform function
void random_range(t_random *x, long n); // set random range
void random_dsp(t_random *x, t_signal **sp, short *count); // dsp add function
void random_assist(t_random *x, void *b, long m, long a, char *s); // assistance function

void ext_main(void* p)
{
    setup((t_messlist **)&random_class, (method)random_new, (method)random_free, (short)sizeof(t_random), 0L, A_DEFLONG, A_DEFLONG, 0);

	// declare the methods
    addmess((method)random_dsp, "dsp", A_CANT, 0);
    addmess((method)random_assist,"assist",A_CANT,0);
	addmess((method)random_range, "range", A_DEFLONG, 0);
    dsp_initclass(); // required for msp

	post("random~: by r. luke dubois, cmc");
}

// deal with the assistance strings...
void random_assist(t_random *x, void *b, long m, long a, char *s)
{
	switch(m) {
		case 1: // inlet
			switch(a) {
				case 0:
				sprintf(s, "Messages...");
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
void *random_new(long flag)
{	
    t_random *x = (t_random *)newobject(random_class);
    dsp_setup((t_pxobject *)x,0);
    outlet_new((t_pxobject *)x, "signal");
	x->range = 1000; // initialize argument
	
	// if the arguments are there check them and set them
	if(flag) x->range = flag;

    if (x->range<1) x->range=1;
    if (x->range>65535) x->range=65535;

    return (x);
}

void *random_free(t_random *x)
{
	dsp_free(&x->r_obj); // flush the dsp memory
}

void random_range(t_random *x, long n)
{
		x->range = n;
	    if (x->range<1) x->range = 1;
	    if (x->range>65535) x->range = 65535;
}

// go go go...
t_int *random_perform(t_int *w)
{
	t_random *x = (t_random *)w[1]; // get current state of my object class
	t_float *out; // variables for input and output buffer
	int n; // counter for vector size

	// more efficient -- make local copies of class variables so i'm not constantly checking the globals...
	int range = x->range;

	int cval_new; // local variable to get polarity of current sample

	out = (t_float *)(w[2]); // my output vector

	n = (int)(w[3]); // my vector size

	while (--n) { // loop executes a single vector
		*++out = ((float)rand()/(float)RAND_MAX)*(float)range;

	}

	return (w+5); // return one greater than the arguments in the dsp_add call
}

void random_dsp(t_random *x, t_signal **sp, short *count)
{
	// add to the dsp chain -- i need my class, my input vector, my output vector, and my vector size...
	dsp_add(random_perform, 4, x, sp[0]->s_vec-1, sp[0]->s_n+1);
}

