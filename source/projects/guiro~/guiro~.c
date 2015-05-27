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
#define MAX_RANDOM 32768
#define MAX_SHAKE 1.0

#define GUIR_SOUND_DECAY 0.95
#define GUIR_NUM_RATCHETS 128
#define GUIR_GOURD_FREQ  2500.0
#define GUIR_GOURD_RESON 0.97
#define GUIR_GOURD_FREQ2  4000.0
#define GUIR_GOURD_RESON2 0.97

/****************************  GUIRO  ***********************/

void *guiro_class;

typedef struct _guiro
{
	//header
    t_pxobject x_obj;
    
    //user controlled vars	    
	    
	float guiroScrape;
	float shakeEnergy;
	float input,output[2];
	float coeffs[2];
	float input2,output2[2];
	float coeffs2[2];
	float sndLevel;
	float gain; 
	float soundDecay;
	float systemDecay;

	long  num_objects;
    float shake_damp;
    float shake_max;
	float res_freq, res_freq2;
	    
    long num_objectsSave;	//number of beans	//resonance
    float shake_dampSave; 	//damping
    float shake_maxSave;
    float res_freqSave, res_freq2Save;

	float collLikely,scrapeVel;
	float totalEnergy;
	float ratchet,ratchetDelta;
	float finalZ[3];	
    
    //signals connected? or controls...
    short num_objectsConnected;
    short res_freqConnected, res_freq2Connected;
    short shake_dampConnected;
    short shake_maxConnected;
   
    float srate, one_over_srate;
} t_guiro;

/****PROTOTYPES****/

//setup funcs
void *guiro_new(double val);
void guiro_dsp(t_guiro *x, t_signal **sp, short *count);
void guiro_float(t_guiro *x, double f);
void guiro_int(t_guiro *x, int f);
void guiro_bang(t_guiro *x);
t_int *guiro_perform(t_int *w);
void guiro_assist(t_guiro *x, void *b, long m, long a, char *s);

void guiro_setup(t_guiro *x);
float guiro_tick(t_guiro *x);

//noise maker
float noise_tick();
int my_random(int max) ;

/****FUNCTIONS****/

void guiro_setup(t_guiro *x) {

  x->num_objects = x->num_objectsSave = GUIR_NUM_RATCHETS;
  x->soundDecay = GUIR_SOUND_DECAY;
  x->coeffs[0] = -GUIR_GOURD_RESON * 2.0 * cos(GUIR_GOURD_FREQ * TWO_PI / x->srate);
  x->coeffs[1] = GUIR_GOURD_RESON*GUIR_GOURD_RESON;
  x->coeffs2[0] = -GUIR_GOURD_RESON2 * 2.0 * cos(GUIR_GOURD_FREQ2 * TWO_PI / x->srate);
  x->coeffs2[1] = GUIR_GOURD_RESON2*GUIR_GOURD_RESON2;
  x->ratchet = x->ratchetDelta = 0.;
  x->guiroScrape = 0.;
}

float guiro_tick(t_guiro *x) {
  float data;
  if (my_random(1024) < x->num_objects) {
    x->sndLevel += 512. * x->ratchet * x->totalEnergy;
  }
  x->input = x->sndLevel;
  x->input *= noise_tick() * x->ratchet;
  x->sndLevel *= x->soundDecay;
		 
  x->input2 = x->input;
  x->input -= x->output[0]*x->coeffs[0];
  x->input -= x->output[1]*x->coeffs[1];
  x->output[1] = x->output[0];
  x->output[0] = x->input;
  x->input2 -= x->output2[0]*x->coeffs2[0];
  x->input2 -= x->output2[1]*x->coeffs2[1];
  x->output2[1] = x->output2[0];
  x->output2[0] = x->input2;
     
  x->finalZ[2] = x->finalZ[1];
  x->finalZ[1] = x->finalZ[0];
  x->finalZ[0] = x->output[1] + x->output2[1];
  data = x->finalZ[0] - x->finalZ[2];
		
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
    setup((struct messlist **)&guiro_class, (method)guiro_new, (method)dsp_free, (short)sizeof(t_guiro), 0L, A_DEFFLOAT, 0);
    addmess((method)guiro_dsp, "dsp", A_CANT, 0);
    addmess((method)guiro_assist,"assist",A_CANT,0);
    addfloat((method)guiro_float);
    addint((method)guiro_int);
    addbang((method)guiro_bang);
    dsp_initclass();
    rescopy('STR#',9333);
}

void guiro_assist(t_guiro *x, void *b, long m, long a, char *s)
{
	assist_string(9333,m,a,1,6,s);
}

void guiro_float(t_guiro *x, double f)
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
		x->res_freq2 = f;
	}
}

void guiro_int(t_guiro *x, int f)
{
	guiro_float(x, (double)f);
}

void guiro_bang(t_guiro *x)
{
	int i;
	post("guiro: scraping");
	x->guiroScrape = 0.;
	/*
	for(i=0; i<2; i++) {
		x->output[i] = 0.;
		x->output1[i] = 0.;
		x->output2[i] = 0.;
		x->output3[i] = 0.;
		x->output4[i] = 0.;
	}
	x->input = 0.0;
	x->input1 = 0.0;
	x->input2 = 0.0;
	x->input3 = 0.0;
	x->input4 = 0.0;
	for(i=0; i<3; i++) {
		x->finalZ[i] = 0.;
	}
	*/
}

void *guiro_new(double initial_coeff)
{
	int i;

    t_guiro *x = (t_guiro *)newobject(guiro_class);
    //zero out the struct, to be careful (takk to jkclayton)
    if (x) { 
        for(i=sizeof(t_pxobject);i<sizeof(t_guiro);i++)  
                ((char *)x)[i]=0; 
	} 
    dsp_setup((t_pxobject *)x,5);
    outlet_new((t_object *)x, "signal");
    
    x->srate = sys_getsr();
    x->one_over_srate = 1./x->srate;
    
	x->guiroScrape = 0;
	x->shakeEnergy = 0.0;
	for(i=0; i<2; i++) {
		x->output[i] = 0.;
		x->output2[i] = 0.;
	}
	x->shake_damp = x->shake_dampSave = 0.9;
	x->shake_max = x->shake_maxSave = 0.9;
	x->totalEnergy = 0.9;
	x->input = 0.0;
	x->input2 = 0.0;
	x->sndLevel = 0.0;
	x->gain = 0.0;
	x->soundDecay = 0.0;
	x->systemDecay = 0.0;
	x->res_freq = x->res_freqSave = 2000.; 
	x->res_freq2 = x->res_freq2Save = 3500.;
 	x->collLikely = 0.;
 	x->scrapeVel = 0.00015;
 	x->ratchet=0.0;x->ratchetDelta=0.0005;
 	for(i=0; i<3; i++) {
		x->finalZ[i] = 0.;
	}
    
    guiro_setup(x);
    
    srand(0.54);
    
    return (x);
}


void guiro_dsp(t_guiro *x, t_signal **sp, short *count)
{
	x->num_objectsConnected = count[0];
	x->shake_dampConnected = count[1];
	x->shake_maxConnected = count[2];
	x->res_freqConnected = count[3];
	x->res_freq2Connected = count[4];
	x->srate = sp[0]->s_sr;
	x->one_over_srate = 1./x->srate;
	
	dsp_add(guiro_perform, 8, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, \
								sp[4]->s_vec, sp[5]->s_vec, sp[0]->s_n);	
	
}

t_int *guiro_perform(t_int *w)
{
	t_guiro *x = (t_guiro *)(w[1]);
	
	float num_objects	= x->num_objectsConnected	? 	*(float *)(w[2]) : x->num_objects;
	float shake_damp 	= x->shake_dampConnected	? 	*(float *)(w[3]) : x->shake_damp;
	float shake_max 	= x->shake_maxConnected		? 	*(float *)(w[4]) : x->shake_max;
	float res_freq 		= x->res_freqConnected		? 	*(float *)(w[5]) : x->res_freq;
	float res_freq2 	= x->res_freq2Connected		? 	*(float *)(w[6]) : x->res_freq2;
	
	float *out = (float *)(w[7]);
	long n = w[8];

	float lastOutput, temp;
	long temp2;

		if(num_objects != x->num_objectsSave) {
			if(num_objects < 1.) num_objects = 1.;
			x->num_objects = (long)num_objects;
			x->num_objectsSave = (long)num_objects;
			x->gain = log(num_objects) * 30.0 / (float) num_objects;
		}
		
		if(res_freq != x->res_freqSave) {
			x->res_freqSave = x->res_freq = res_freq;
			x->coeffs[0] = -GUIR_GOURD_RESON * 2.0 * cos(res_freq * TWO_PI / x->srate);
		}
		
		if(shake_damp != x->shake_dampSave) {
			x->shake_dampSave = x->shake_damp = shake_damp;
			//x->systemDecay = .998 + (shake_damp * .002);
			//x->ratchetDelta = shake_damp;
			x->scrapeVel = shake_damp;
		}
		
		if(shake_max != x->shake_maxSave) {
			x->shake_maxSave = x->shake_max = shake_max;
		 	//x->shakeEnergy += shake_max * MAX_SHAKE * 0.1;
	    	//if (x->shakeEnergy > MAX_SHAKE) x->shakeEnergy = MAX_SHAKE;
	    	x->guiroScrape = shake_max;
		}	

		if(res_freq2 != x->res_freq2Save) {
			x->res_freq2Save = x->res_freq2 = res_freq2;
			x->coeffs2[0] = -GUIR_GOURD_RESON2 * 2.0 * cos(res_freq2 * TWO_PI / x->srate);
		}	
		
		
		while(n--) {
		  if (x->guiroScrape < 1.0)      {
	      	x->guiroScrape += x->scrapeVel;
	      	x->totalEnergy = x->guiroScrape;
	      	x->ratchet -= (x->ratchetDelta + (0.002*x->totalEnergy));
	      	if (x->ratchet<0.0) x->ratchet = 1.0;
	      	lastOutput = guiro_tick(x);
	    	}
	      else lastOutput = 0.0;
	      *out++ = lastOutput;
		}
	return w + 9;
}	

