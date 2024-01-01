#include <math.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <limits.h>
#include <complex.h> 
#include <fftw3.h>

#define  DEBUG  1

int terror(int f, int l, char * file);
#define TER( X )    terror(X, __LINE__,__FILE__)


#define PLAY        0
#define STOP        1

#define REAL        0
#define IMG         1
#define M_2PI       ((double) 6.2831853071795864769252867665590057683943387987502116419498891846) 

typedef struct {
    SDL_AudioDeviceID device;
    SDL_AudioSpec   Obtained;
	double          step;
	uint32_t        samplePos;
	int             FreqSample;
    int             Play;	
    int             *Wave;
    int             Max;
    int             Min;
    int             high;
    int             low;
    int             middle;
    float           Average;
    int             filled;
} Sound;

typedef struct {
    SDL_Renderer *Render;
    SDL_Window   *Window;
} Window;

typedef union {
    uint32_t U32;
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    }U8;
} Color;

typedef struct {
    int     x,y,h,w;
    Color    *lines;
    int     max, min;
    double  scale;
    double  offset;
    int     page;

    // Usar linked lists para cada linha e fazer a interpolação e resize no render.
    // Ao destruir uma linha pode-se reaproveitar a memória.
    struct data {
        Color *lines;
        int lenght;
        struct data *next;
    } *start, *last;
    int lsize;

    SDL_Renderer * Render;
} Waterfall;

Sound sound;
int WaterfallInit(Waterfall *W);
void WaterfallAdd(Waterfall *W, int * data, int lenght);
void WaterfallRender(Waterfall *W);

Window * WindowCreate();
void DrawWave(SDL_Renderer * Render, int x, int y, int width, int length, int *data, int buffersize , float xscale, int max);

void SDLAudioCallback(void *data, Uint8 *buffer, int length) {
    Sound *sound = (Sound*) data;

    int len = length >> 1;          // length/2 Porque usei AUDIO_S16SYS como áudio formato
    int16_t *BufferOut = (int16_t *) buffer;
    sound->high              = INT16_MIN;
    sound->low              = INT16_MAX;
    sound->Average          = 0;
    for(int i = 0; i < len; i++)
    {
        //BufferOut[i] = (int16_t) (( 2500.0f * sin(sound->samplePos * sound->step) ) + ((double) rand() / RAND_MAX) * 100.0f);
        //if (BufferOut[i] > sound->high) sound->high = BufferOut[i];
        //if (BufferOut[i] < sound->low) sound->low = BufferOut[i];
        sound->Wave[i] = BufferOut[i];
        sound->samplePos++;
        sound->Average = (sound->Average + BufferOut[i]) /2 ;
    }	    
/*
    float adjust = 1;
    if (sound->high != 0 && sound->low != 0) {
        if (sound->high > abs(sound->low)) adjust = (float) sound->Max / sound->high; else adjust = (float) sound->Max / abs(sound->low);
    }
*/
    
    //for (int i = 0; i < len; i++) sound->Wave[i] = sound->Wave[i] * adjust/2;


    //sound->Average = sound->Average / (sound->Obtained.samples + 1);
    sound->filled = 1;

    //printf ("Min: %d Max: %d Average: %f Adjust: %f\n",sound->low,sound->high,sound->Average, adjust);

}
 
void sound_init(Sound *sound) {

    sound->FreqSample 		= 44100;
	sound->step 	        = (double) ((M_PI * 2) / sound->FreqSample );
    sound->samplePos        = 0;
    sound->Max              = INT16_MAX;
    sound->Min              = INT16_MIN;
    sound->middle           = (INT16_MAX - INT16_MIN) / 2;
    sound->filled           = 0;
    sound->high             = sound->Min;
    sound->low              = sound->Max;
	printf ("Step: %f middle: %d \n", sound->step, sound->middle);

    // https://wiki.libsdl.org/SDL_AudioSpec
	SDL_AudioSpec desired;
	SDL_zero(desired);
    desired.freq = sound->FreqSample;
    desired.format = AUDIO_S16SYS; 
    desired.channels = 1;
    desired.samples = 1024 << 1;
    desired.callback = SDLAudioCallback;
    desired.userdata = sound;

    sound->device = SDL_OpenAudioDevice(NULL, 1, &desired, &sound->Obtained, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (sound->device == 0 || desired.format != sound->Obtained.format) { 
        printf ("OpenAudioDevice failed");
        exit(-1);
    }
    SDL_PauseAudioDevice(sound->device, STOP);
    
    
    sound->Wave              = malloc(sizeof(int) * sound->Obtained.samples);
    if (sound->Wave == NULL) {
        printf ("sound->Wave allocation failed.");
        exit(-1);
    }
    printf("Samples: %d , Format: %d , Buffer Wave size: %ld \n",sound->Obtained.samples, sound->Obtained.format,sizeof(Uint16) * sound->Obtained.samples);
    
	
}

float dabs (const float complex* in, float* out, float max, const int length)
{
    for (int i = 0; i < length; i += 8)
    {
        const float re[8] __attribute__((aligned (32))) = {creal(in[i]), creal(in[i+1]), creal(in[i+2]), creal(in[i+3]),creal(in[i+4]),creal(in[i+5]),creal(in[i+6]),creal(in[i+7])};
        const float im[8] __attribute__((aligned (32))) = {cimag(in[i]),cimag(in[i+1]),cimag(in[i+2]),cimag(in[i+3]),cimag(in[i+4]),cimag(in[i+5]),cimag(in[i+6]),cimag(in[i+7])};
        __m256 x4 = _mm256_load_ps (re);
        __m256 y4 = _mm256_load_ps (im);
        __m256 b4 = _mm256_sqrt_ps (_mm256_add_ps (_mm256_mul_ps (x4,x4), _mm256_mul_ps (y4,y4)));
        _mm256_storeu_ps (out + i, b4);
    }
    
    return max;
}


int main () {

    int quit = 0;
    SDL_Event event;

    TER(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO));
    Window *W = WindowCreate();
    atexit(SDL_Quit);   
	 
	sound_init(&sound);
    sound.Play              = PLAY;
    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );

	
    float *in;// , *out; 
    fftwf_complex *out;
    fftwf_plan p;
    in = /*(double*)*/ fftwf_malloc(sizeof(fftwf_complex) * sound.Obtained.samples + 1);
    out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * sound.Obtained.samples);
    //out = (double*) fftw_malloc(sizeof(double) * 2048 + 1);
    int *mag = malloc(sizeof(int) * sound.Obtained.samples);
    int *magi = _mm_malloc(sizeof(int) * sound.Obtained.samples,sizeof(float));
    int *overlapin = malloc(sizeof(int) * sound.Obtained.samples);
    p = fftwf_plan_dft_r2c_1d(sound.Obtained.samples, in, out, FFTW_ESTIMATE);
    int max = INT_MIN;

    SDL_PauseAudioDevice(sound.device, PLAY);


    Waterfall Water;
    Water.Render = W->Render;
    Water.x = 0;
    Water.y = 0;
    Water.w = 750;
    Water.h = 85;
    Water.max = sound.Max / 2;
    Water.min = sound.Min / 2;  

    WaterfallInit(&Water);


    while( quit == 0 ){

        SDL_SetRenderDrawColor(W->Render, 0, 0, 0, 255);
        SDL_RenderClear(W->Render);        
        int w,h;
        SDL_GetWindowSize(W->Window,&w,&h);
        DrawWave(W->Render,1,86,w,h/3,sound.Wave,sound.Obtained.samples,(float) sound.Obtained.samples / h ,sound.Max);
        
        if (sound.filled == 1) {
            //printf ("Min: %d Max: %d Average: %f\n",sound.low,sound.high,sound.Average);
            
            for (int i = 0; i < (sound.Obtained.samples >> 1); i++) {
                in[i] = overlapin[i] * (0.53836 - 0.46164*cos((M_2PI * i)/(sound.Obtained.samples-1)));
                overlapin[i] = sound.Wave[i + (sound.Obtained.samples >> 1)];
            }
            for (int i = (sound.Obtained.samples >> 1) + 1; i < sound.Obtained.samples; i++) {
                in[i] = sound.Wave[i - (sound.Obtained.samples >> 1)] * (0.53836 - 0.46164*cos((M_2PI * i)/(sound.Obtained.samples-1)));
            }
            
            fftwf_execute(p);
/*            
            for (int i = 0; i < sound.Obtained.samples >> 1; i++ ) {
                mag[i] = log10(cabs(out[i])) * cabs(out[i]);
                if (mag[i] > max) max = mag[i];
            }
*/
            max = dabs (out, mag, max , sound.Obtained.samples >> 1);
            for (int i = 0; i < sound.Obtained.samples >> 1; i++ ) {
                magi[i] = (int) floor(mag[i]);   
                if (magi[i] > max) max = magi[i];
            }
            
            //printf ("-> %d ", max );
            sound.filled = 0;
            DrawWave(W->Render,1,h-h/6,w,h/3,mag,sound.Obtained.samples >> 1, 1 , max/3);

            WaterfallAdd(&Water,mag,sound.Obtained.samples >> 1);
            WaterfallRender(&Water);

            SDL_RenderPresent(W->Render);
        }


        while ( SDL_PollEvent( &event ) ) {
                switch( event.type ){
                    case SDL_WINDOWEVENT: {
                        switch (event.window.event) {
                            case SDL_WINDOWEVENT_EXPOSED:
                            {
                                            

                            } break;
                        }

                    } break;
                    case SDL_KEYDOWN:                        
                        switch (event.key.keysym.sym)
                        {
                            case SDLK_ESCAPE:  
                            case SDLK_q:
                                quit = 1; 
                            break;
                            case SDLK_SPACE:
                                sound.Play ^= 1; 
                                SDL_PauseAudioDevice(sound.device, sound.Play);
                            break;
                        }
                    break;
                    case SDL_QUIT:
                        quit = 1;
                    break;
                    default:
                    break;
                }

            }
        SDL_Delay(30);
    }	

	SDL_CloseAudioDevice(sound.device);
    SDL_Quit();
    exit(0);
}


Window * WindowCreate() {
    SDL_Window* window = SDL_CreateWindow(__FILE__,
										  SDL_WINDOWPOS_UNDEFINED,
										  SDL_WINDOWPOS_UNDEFINED,
										  512,
										  256,
 										  SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        printf ("\e[1;31m[ Error creating window ]\e[0m\n");
    }

    Window *W = malloc(sizeof(Window));
    W->Render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    W->Window = window;

    if (W->Render == NULL) {
        printf ("\e[1;31m[ Error creating renderer ]\e[0m\n");
        printf("\e[1;31m[ SDL_Init failed: %s ]\e[0m\n", SDL_GetError());
    }

    return W;
}

int WaterfallInit(Waterfall *W){

    W->lines = calloc(1,sizeof(Color) * W->w * W->h);
    if (W->lines == NULL) return -1;
    int Range = abs(W->max - W->min);
    W->scale =  1;//(double) UINT32_MAX / Range;
    W->offset = 0;
    //if (W->min < 0) W->offset = abs(W->min);
    //W->offset += 255;

    W->lsize = 0;
    W->start = 0;
    W->last = NULL;

    W->page = 0;
    return 0;
}

void WaterfallAdd(Waterfall *W, int * data, int length){
    int i = 0;

    


    Color *pline = &W->lines[W->page*W->w];    
    if (length > W->w) {
        float a = 0;
        float ainc        = (float) length / (W->w);   
        
        for (i = 0; i < W->w; i++) {
            pline[i].U32 = 0;
            int astop = a+ainc;
            for (; a < astop && a < length; a++) {
                pline[i].U32 +=  W->offset + (data[(int) floor(a)]) ; 
            }
            if (i > 1) pline[i].U32 = (pline[i].U32 + pline[i-1].U32)/(2+ainc);
        }  

        //exit(-1);
    } else {
        if (length == W->w) {
            for (i = 0; i < length; i++) {
                pline[i].U32 = W->offset + data[i] * W->scale;
                //if (i > 1) pline[i].U32 = (pline[i].U32 + pline[i-1].U32)/2;
            }
        } else {
            for (i = 0; i < length; i++) {
                pline[i].U32 = W->offset + data[i] * W->scale;
            }
            for (;i < W->w; i++) {
                pline[i].U32 = 0;
            }
        }
    }

    W->page = (W->page + 1) % W->h;
}

void WaterfallRender(Waterfall *W) {
    int page;
    Color *pline;
    for (int h = 0; h < W->h; h++) {
            page = (h + W->page) % W->h; 
            pline = &W->lines[page*W->w];
        for (int w = 0; w < W->w; w++) {
            SDL_SetRenderDrawColor(W->Render, pline[w].U8.r, pline[w].U8.g, pline[w].U8.b, pline[w].U8.a);
            SDL_RenderDrawPoint(W->Render,W->x + w,W->y + h);
        }
        
    }

}

void DrawWave(SDL_Renderer * Render, int x, int y, int width, int height, int *data, int buffersize , float xscale, int max) {

        int xi = 0,yi = 0,xold = 0,yold = 0; 
        if (max == 0) max = 1;
        float scale = (float) (height >> 1) / max;
        int   xscaleint = 1.0f / xscale;
        if (xscaleint < 1) xscaleint = 1;

        //float x_increment = (float) height / buffersize;
  
        // Faz o preenchimento da onda
        for (int i = 0 ; i < buffersize; i += xscaleint ) {
            int c = abs(data[i] * (255.0f / max)) / 25;
            if (data[i] >= 0)
            SDL_SetRenderDrawColor(Render, c, c + 10, c, 100);
            else 
            SDL_SetRenderDrawColor(Render, c+10, c, c, 100);
            xi = xi + 1;
            SDL_RenderDrawLine(Render,xi + x,(height>> 1) + y,xi + x,y + (height >> 1) - data[i] * scale);
        }
        
        SDL_SetRenderDrawColor(Render, 88, 195, 74, 255);
        xold = x;
        yold =  y + (height >> 1) - data[0] * scale;
        xi = xold + 1;
        yi = y + (height >> 1) - data[1] * scale;
        for (int i = 2 ; i < buffersize; i += xscaleint ) {
            SDL_RenderDrawLine(Render, xold,yold,xi,yi);
            xold = xi;
            yold = yi;
            xi += 1;
            yi = y + (height>> 1) - data[i] * scale;
        }

}

int terror(int f, int l, char * file) {
    if (f != 0) {
        printf ("\e[1;31m[ Error %d at %d in file %s ]\e[0m\n",f,l,file);
        exit(-1);
    }
    #if DEBUG == 1
        printf ("[ R %d at %d in file %s ]\n",f,l,file);
    #endif
   return f; 
}