#include "ext.h"
#include "z_dsp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define TWO_PI 6.283185307
//#define ONE_OVER_RANDLIMIT 0.00006103516 // constant = 1. / 16384.0
#define ONE_OVER_RANDLIMIT 1./RAND_MAX
#define ATTACK 0
#define DECAY 1
#define SUSTAIN 2
#define RELEASE 3
#define MAX_RANDOM RAND_MAX
#define MAX_SHAKE 1.0

#define BAMB_SOUND_DECAY 0.95
#define BAMB_SYSTEM_DECAY 0.99995
#define BAMB_NUM_TUBES 5
#define BAMB_BASE_FREQ  2800

#define TAMB_SOUND_DECAY 0.95
#define TAMB_SYSTEM_DECAY 0.9985
#define TAMB_NUM_TIMBRELS 32
#define TAMB_SHELL_FREQ 2300
#define TAMB_SHELL_GAIN 0.1
#define TAMB_CYMB_FREQ  5600
#define TAMB_CYMB_FREQ2 8100
#define TAMB_CYMB_RESON 0.99

void *tamb_class;

typedef struct _tamb
{
	//header
    t_pxobject x_obj;
    
    //user controlled vars	    

	float shakeEnergy;
	float input,output[2];
	float coeffs[2];
	float input1,output1[2];
	float coeffs1[2];
	float input2,output2[2];
	float coeffs2[2];
	float sndLevel;
	float gain;
	float soundDecay;
	float systemDecay;
	float cymb_rand;
	
	long  num_objects;
    float shake_damp;
    float shake_max;
	float res_freq;
	    
    long num_objectsSave;	//number of beans	
    float res_freqSave;	//resonance
    float shake_dampSave; 	//damping
    float shake_maxSave;
    float res_freq1, res_freq2, res_freq1Connected, res_freq2Connected;
    float res_freq1Save, res_freq2Save;

	float totalEnergy;
	float finalZ[3];	
    
    //signals connected? or controls...
    short num_objectsConnected;
    short res_freqConnected;
    short shake_dampConnected;
    short shake_maxConnected;
   
    float srate, one_over_srate;
} t_tamb;

/****PROTOTYPES****/

//setup funcs
void *tamb_new(double val);
void tamb_dsp(t_tamb *x, t_signal **sp, short *count);
void tamb_float(t_tamb *x, double f);
void tamb_int(t_tamb *x, int f);
void tamb_bang(t_tamb *x);
t_int *tamb_perform(t_int *w);
void tamb_assist(t_tamb *x, void *b, long m, long a, char *s);

void tamb_setup(t_tamb *x);
float tamb_tick(t_tamb *x);

//noise maker
float noise_tick();
int my_random(int max) ;

/****FUNCTIONS****/

void tamb_setup(t_tamb *x) {	
	
  x->num_objects = x->num_objectsSave = TAMB_NUM_TIMBRELS;
  x->soundDecay = TAMB_SOUND_DECAY;
  x->systemDecay = TAMB_SYSTEM_DECAY;
  x->gain = 24.0 / x->num_objects;
  x->coeffs[0] = -0.96 * 2.0 * cos(TAMB_SHELL_FREQ * TWO_PI / x->srate);
  x->coeffs[1] = 0.96*0.96;
  x->coeffs1[0] = -TAMB_CYMB_RESON * 2.0 * cos(TAMB_CYMB_FREQ * TWO_PI / x->srate);
  x->coeffs1[1] = TAMB_CYMB_RESON * TAMB_CYMB_RESON;
  x->coeffs2[0] = -TAMB_CYMB_RESON * 2.0 * cos(TAMB_CYMB_FREQ2 * TWO_PI / x->srate);
  x->coeffs2[1] = TAMB_CYMB_RESON * TAMB_CYMB_RESON;
}

float tamb_tick(t_tamb *x) {
  float data;
  x->shakeEnergy *= x->systemDecay;    // Exponential System Decay
  if (my_random(1024) < x->num_objects) {     
    x->sndLevel += x->gain *  x->shakeEnergy;   
    x->cymb_rand = noise_tick() * x->res_freq1 * 0.05;
    x->coeffs1[0] = -TAMB_CYMB_RESON * 2.0 * 
      cos((x->res_freq + x->cymb_rand) * TWO_PI / x->srate);
    x->cymb_rand = noise_tick() * x->res_freq2 * 0.05;
    x->coeffs2[0] = -TAMB_CYMB_RESON * 2.0 * 
      cos((x->res_freq2 + x->cymb_rand) * TWO_PI / x->srate);
  }
  x->input = x->sndLevel;
  x->input *= noise_tick();         // Actual Sound is Random
  x->input1 = x->input * 0.8;
  x->input2 = x->input;
  x->input *= TAMB_SHELL_GAIN;
	
  x->sndLevel *= x->soundDecay;          // Each (all) event(s) 
  // decay(s) exponentially 
  x->input -= x->output[0]*x->coeffs[0];
  x->input -= x->output[1]*x->coeffs[1];
  x->output[1] = x->output[0];
  x->output[0] = x->input;
  data = x->output[0];
  x->input1 -= x->output1[0]*x->coeffs1[0];
  x->input1 -= x->output1[1]*x->coeffs1[1];
  x->output1[1] = x->output1[0];
  x->output1[0] = x->input1;
  data += x->output1[0];
  x->input2 -= x->output2[0]*x->coeffs2[0];
  x->input2 -= x->output2[1]*x->coeffs2[1];
  x->output2[1] = x->output2[0];
  x->output2[0] = x->input2;
  data += x->output2[0];
	
  x->finalZ[2] = x->finalZ[1];
  x->finalZ[1] = x->finalZ[0];
  x->finalZ[0] = data;
  data = x->finalZ[2] - x->finalZ[0];
		
  return data;
}

int my_random(int max)  {   //  Return Random Int Between 0 and max
	unsigned long temp;
  	temp = (unsigned long) rand();
	temp *= (unsigned long) max;
	temp >>= 15;
	return (int) temp; 
}

//noise maker
float noise_tick() 
{
	float output;
	output = (float)rand() - 16384.;
	output *= ONE_OVER_RANDLIMIT;
	return output;
}

//primary MSP funcs
void ext_main(void* p)
{
    setup((struct messlist **)&tamb_class, (method)tamb_new, (method)dsp_free, (short)sizeof(t_tamb), 0L, A_DEFFLOAT, 0);
    addmess((method)tamb_dsp, "dsp", A_CANT, 0);
    addmess((method)tamb_assist,"assist",A_CANT,0);
    addfloat((method)tamb_float);
    addint((method)tamb_int);
    addbang((method)tamb_bang);
    dsp_initclass();
    rescopy('STR#',9335);
}

void tamb_assist(t_tamb *x, void *b, long m, long a, char *s)
{
	assist_string(9335,m,a,1,7,s);
}

void tamb_float(t_tamb *x, double f)
{
	if (x->x_obj.z_in == 0) {
		x->num_objects = (long)f;
	} else if (x->x_obj.z_in == 3) {
		x->res_freq = f;
	} else if (x->x_obj.z_in == 1) {
		x->shake_damp = f;
	} else if (x->x_obj.z_in == 2) {
		x->shake_max = f;
	} else if (x->x_obj.z_in == 4) {
		x->res_freq1 = f;
	} else if (x->x_obj.z_in == 5) {
		x->res_freq2 = f;
	}
}

void tamb_int(t_tamb *x, int f)
{
	tamb_float(x, (double)f);
}

void tamb_bang(t_tamb *x)
{
	int i;
	post("tamb: zeroing delay lines");
	for(i=0; i<2; i++) {
		x->output[i] = 0.;
		x->output1[i] = 0.;
		x->output2[i] = 0.;
		//x->output3[i] = 0.;
		//x->output4[i] = 0.;
	}
	x->input = 0.0;
	x->input1 = 0.0;
	x->input2 = 0.0;
	//x->input3 = 0.0;
	//x->input4 = 0.0;
	for(i=0; i<3; i++) {
		x->finalZ[i] = 0.;
	}
}

void *tamb_new(double initial_coeff)
{
	int i;
	
    t_tamb *x = (t_tamb *)newobject(tamb_class);
    //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_pxobject);i<sizeof(t_tamb);i++)  
                ((char *)x)[i]=0; 
	} 
    dsp_setup((t_pxobject *)x,6);
    outlet_new((t_object *)x, "signal");
    
    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    
	x->shakeEnergy = 0.0;
	for(i=0; i<2; i++) {
		x->output[i] = 0.;
		x->output1[i] = 0.;
		x->output2[i] = 0.;

	}
	x->input = 0.0;
	x->input1 = 0.0;
	x->input2 = 0.0;
	x->sndLevel = 0.0;
	x->soundDecay = 0.0;
	x->systemDecay = 0.0;
	x->cymb_rand = 0.0;
	x->shake_damp = x->shake_dampSave = 0.9;
	x->shake_max = x->shake_maxSave = 0.9;
	x->totalEnergy = 0.;
	x->res_freq = x->res_freq1 = x->res_freq2 = 3000.;
	x->res_freqSave = x->res_freq1Save = x->res_freq2Save = 3000.;
	
 	for(i=0; i<3; i++) {
		x->finalZ[i] = 0.;
	}
    
    tamb_setup(x);
    
    srand(0.54);
    
    return (x);
}


void tamb_dsp(t_tamb *x, t_signal **sp, short *count)
{
	x->num_objectsConnected = count[0];
	x->shake_dampConnected = count[1];
	x->shake_maxConnected = count[2];
	x->res_freqConnected = count[3];
	x->res_freq1Connected = count[4];
	x->res_freq2Connected = count[5];
	
	x->srate = sp[0]->s_sr;
	x->one_over_srate = 1./x->srate;
	
	dsp_add(tamb_perform, 9, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, \
								sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec, sp[0]->s_n);	
	
}

t_int *tamb_perform(t_int *w)
{
	t_tamb *x = (t_tamb *)(w[1]);
	
	float num_objects	= x->num_objectsConnected	? 	*(float *)(w[2]) : x->num_objects;
	float shake_damp 	= x->shake_dampConnected	? 	*(float *)(w[3]) : x->shake_damp;
	float shake_max 	= x->shake_maxConnected		? 	*(float *)(w[4]) : x->shake_max;
	float res_freq 		= x->res_freqConnected		? 	*(float *)(w[5]) : x->res_freq;
	float res_freq1 	= x->res_freq1Connected		? 	*(float *)(w[6]) : x->res_freq1;
	float res_freq2 	= x->res_freq2Connected		? 	*(float *)(w[7]) : x->res_freq2;
	
	float *out = (float *)(w[8]);
	long n = w[9];

	float lastOutput, temp;
	long temp2;

		if(num_objects != x->num_objectsSave) {
			if(num_objects < 1.) num_objects = 1.;
			x->num_objects = (long)num_objects;
			x->num_objectsSave = (long)num_objects;
			x->gain = 24.0 / x->num_objects;
		}
		
		if(res_freq != x->res_freqSave) {
			x->res_freqSave = x->res_freq = res_freq;
			x->coeffs[0] = -0.96 * 2.0 * cos(res_freq * TWO_PI / x->srate);
		}
		
		if(shake_damp != x->shake_dampSave) {
			x->shake_dampSave = x->shake_damp = shake_damp;
			x->systemDecay = .998 + (shake_damp * .002);
		}
		
		if(shake_max != x->shake_maxSave) {
			x->shake_maxSave = x->shake_max = shake_max;
		 	x->shakeEnergy += shake_max * MAX_SHAKE * 0.1;
	    	if (x->shakeEnergy > MAX_SHAKE) x->shakeEnergy = MAX_SHAKE;
		}	

		if(res_freq1 != x->res_freq1Save) {
			x->res_freq1Save = x->res_freq1 = res_freq1;
			//x->coeffs1[0] = -TAMB_CYMB_RESON * 2.0 * cos(res_freq1 * TWO_PI / x->srate);
		}
		
		if(res_freq2 != x->res_freq2Save) {
			x->res_freq2Save = x->res_freq2 = res_freq2;
			//x->coeffs2[0] = -TAMB_CYMB_RESON * 2.0 * cos(res_freq2 * TWO_PI / x->srate);
		}	
		
		while(n--) {
			lastOutput = tamb_tick(x);		
			*out++ = lastOutput;
		}
	return w + 10;
}	

