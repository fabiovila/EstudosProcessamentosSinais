#include <SDL2/SDL_mutex.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <assert.h>



#define ON                  0
#define OFF                 1
#define BASE_C_NOTE          261.6256 /2//523.2511	//261.6256
#define NOISE               20
#define M_2PI               2 * M_PI
#define MAX_TONES_BY_NOTE   12
#define DEFAULT_GAIN        10
#define MAX_FREQ            15000
           

double  ADSR [100];
enum NOTES { C, Cs, D, Ds, E, F, Fs, G, Gs, A, As, B };
enum EVENTS {PRESSED,SUSTAIN,RELEASE,SILENCE};
char NOTES_NAME [12][3] = { {"C"}, {"C#"}, {"D"}, {"D#"}, {"E"}, {"F"}, {"F#"}, {"G"}, {"G#"}, {"A"}, {"A#"}, {"B"} };
#define FILTER      5000
int filter[FILTER];

const double tremolo_freq = 7;
const double vibrato_freq = 8;

typedef struct stGenerator{
    char name[12];
    double  Alpha;
    double  y0,y1,y2;
    double  ADSR;
    int     Gain;
    int (*RunCallback) (struct stGenerator *, double, enum EVENTS);
} Generator;

typedef struct stVector {
    int *value;
    int reserved;
    int size;
} Vector;

typedef struct stSynth{
    SDL_AudioDeviceID   device;
    SDL_AudioSpec       Obtained;
	double              step;
	uint32_t            samplePos;
    enum EVENTS         Event[12];	
    int                 *Wave;
    int                 Max;
    int                 GeneratorsReserved;
    Generator           ***Generators;
    int                 Phase;
} Synth;

struct {
    int Wsize, Hsize;
} Window;

Generator *GetGenerator (Synth *, int n, int g);
SDL_Renderer * WindowCreate();
void DrawWave(SDL_Renderer * Render, int x, int y, int width, int length, int *data, int buffersize , float xscale, int max);

double map(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

SDL_AudioDeviceID device;
SDL_mutex *Mutex;

int RunGenerators(Synth *s, enum NOTES n, double Vibrato, enum EVENTS e) {

    int a,i, out = 0;
    static int delay;

       for (a = 0; a < MAX_TONES_BY_NOTE; a++) {
            Generator **G = s->Generators[n];
            if (G[a] != 0) {

            switch (e) {
                case SILENCE:
                    
                    break;
                case PRESSED:
                    G[a]->ADSR = 0;
                    s->Event[n] = SUSTAIN;
                    break;
                case SUSTAIN:
                    if (G[a]->ADSR < 75) G[a]->ADSR += 0.1f;
                    break;
                case RELEASE:
                    G[a]->ADSR += 0.01f;
                    if (G[a]->ADSR >= 100) { G[a]->ADSR = 0; s->Event[n] = SILENCE; }
                    break;
            }
                assert (G[a]->ADSR < 100);
                delay = 0;
                out +=  G[a]->RunCallback(G[a],Vibrato,e) * ADSR[(int) G[a]->ADSR];
                filter[FILTER-1] = out;
            }
        }

return out;

}

Generator *Tremolo;

static void SDLAudioCallback(void *data, Uint8 *buffer, int length) {
    SDL_LockMutex(Mutex);
    Synth *synth = (Synth*) data;
    int16_t *b = (int16_t *) buffer;
    int len = length / sizeof(*b);
    int i,n,g;
    Generator   *G;

    double tremolo;
    int y[10];

    for(i = 0; i < len; ++i)
    {

        b[i] = 0;
        tremolo = Tremolo->RunCallback(Tremolo,0,SUSTAIN);
        //printf(" %f ", tremolo);
        ///b[i] += (synth->Wave[i]/(i+1))*0.9f;

        for (n = 0; n < 12; n++) {               
                b[i] += 8 * RunGenerators(synth,n, tremolo/40000.0f,synth->Event[n]);            
        }


        //b[i] += (0.5*(b[i]-y[2]));
        //b[i] *= (1.0 + (tremolo / 10.0f));
        if (b[i] != 0 ) b[i] += (NOISE >> 1) - rand()%(NOISE+1);
        //b[i] = pow(b[i]/300.0f,3);

        y[2] = y[1];
        y[1] = y[0];
        y[0] = b[i];
        
        synth->Wave[i] = b[i];
        synth->Phase++;        
    }	
    SDL_UnlockMutex(Mutex);
}



void GeneratorON (Synth *s, enum NOTES N) {
    //printf(" %d  ",s->Event[N]);
    if (s->Event[N] == RELEASE || s->Event[N] == SILENCE) s->Event[N] = PRESSED;
    
}

void GeneratorOFF (Synth *s, enum NOTES N) {
    s->Event[N] = RELEASE;    
}



void AddGenerator (Synth *s, enum NOTES n, Generator *g) {

    int a;

        for (a = 0; a < MAX_TONES_BY_NOTE; a++) {
            Generator **G = s->Generators[n];
            if (G[a] == 0) {
                G[a] = g;
                assert(G[a] == g);
                return;
            }
        }

}


int  RunSquare(Generator *this, double Vibrato, enum EVENTS e) {
    this->y0++;
    double Alpha = Vibrato + this->Alpha;
    #define SMOOTH   0.9f
    if (this->y0 >= Alpha) this->y0 = 0;

    if (this->y0 < (Alpha / 3)) {
        this->y1 =  SMOOTH*this->Gain + (1.0-SMOOTH)*this->y1;
    } else {
        this->y1 =  (1.0-SMOOTH)*this->y1;
    }
    return this->y1;
}

Generator *NewSquare(double Freq, double FreqSample, int Gain){
    Generator *sin = malloc(sizeof(Generator));
    assert(sin != NULL);
        if (Freq < MAX_FREQ) sin->Alpha = (FreqSample/Freq); else sin->Alpha = 0;
        printf (" Square: Freq %f Alpha %f\n",Freq, sin->Alpha);
        strcpy(sin->name,"SIN");
        sin->y0 = 0;
        sin->y1 = 0;
        sin->y2 = 0;
        sin->RunCallback = RunSquare;
        sin->Gain = Gain;
    return sin;
}

int  RunTriangle(Generator *this, double Vibrato, enum EVENTS e) {
    this->y0 = this->Alpha + this->y0;
    if (this->y0 > this->Gain) this->Alpha = -1.0f * this->Alpha;
    if (this->y0 <= 0) this->Alpha = -1.0f * this->Alpha;

    #define SMOOTH   0.1f
    this->y1 =  SMOOTH*this->y0 + (1.0-SMOOTH)*this->y1;

    return  this->y1 - (this->Gain / 2.0f);
}

Generator *NewTriangle(double Freq, double FreqSample, int Gain){
    Generator *sin = malloc(sizeof(Generator));
    assert(sin != NULL);
        double tg = Gain / ((FreqSample/Freq) / 2);
        if (Freq < MAX_FREQ) sin->Alpha = tg; else sin->Alpha = 0;
        printf (" Triangle: Freq %f Alpha %f\n",Freq, sin->Alpha);
        strcpy(sin->name,"SIN");
        sin->y0 = 0;
        sin->y1 = 0;
        sin->y2 = 0;
        sin->RunCallback = RunTriangle;
        sin->Gain = Gain;
    return sin;
}


int  RunSin(Generator *this, double Vibrato, enum EVENTS e) {
    if (e == SILENCE) {
        this->y1 = this->Alpha;
        this->y2 = this->Alpha;
        this->y0 = 0;
        return 0;
    }
    this->y2 = this->y1;
    this->y1 = this->y0;
    this->y0 = (this->Alpha + Vibrato) * this->y1 - this->y2;

    return  this->y0 * this->Gain;
}

Generator *NewSin(double Freq, double FreqSample, int Gain){
    Generator *sin = malloc(sizeof(Generator));
    assert(sin != NULL);
        if (Freq < MAX_FREQ) sin->Alpha = 2*cos((M_2PI*Freq)/FreqSample); else sin->Alpha = 0;
        printf (" Sinusoide: Freq %f Alpha %f\n",Freq, sin->Alpha);
        strcpy(sin->name,"SIN");
        sin->y1 = sin->Alpha;
        sin->y2 = sin->Alpha;
        sin->RunCallback = RunSin;
        sin->Gain = Gain;
    return sin;
}

void synth_init(Synth *synth) {

    int i;
    
    synth->samplePos        = 0;
    synth->Phase            = 0;



    // https://wiki.libsdl.org/SDL_AudioSpec
	SDL_AudioSpec desired;
	SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS; 
    desired.channels = 1;
    desired.samples = 2048;
    desired.callback = SDLAudioCallback;
    desired.userdata = synth;

    synth->device = SDL_OpenAudioDevice(NULL, 0, &desired, &synth->Obtained, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (synth->device == 0 || desired.format != synth->Obtained.format) { 
        printf ("OpenAudioDevice failed");
        exit(-1);
    }
    
    synth->Wave              = malloc(sizeof(int) * synth->Obtained.samples);
    synth->GeneratorsReserved = MAX_TONES_BY_NOTE;
    synth->Generators =  calloc(12,sizeof(Generator **));
    assert(synth->Generators != NULL);

    for (i = 0; i < 12; i++) {
        synth->Generators[i] = calloc(MAX_TONES_BY_NOTE,sizeof(Generator *));
        assert(synth->Generators[i] != NULL);
    }

    if (synth->Wave == NULL) {
        printf ("synth->Wave allocation failed.");
        exit(-1);
    }
    
    

    printf("Samples: %d , Format: %d , Buffer Wave size: %ld \n",synth->Obtained.samples, synth->Obtained.format,sizeof(Uint16) * synth->Obtained.samples);
    
	
}


int main () {

    int quit = 0;
    int i,a;
    SDL_Event event;
    Synth synth;

    Mutex = SDL_CreateMutex();
    SDL_LockMutex(Mutex);
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);
    SDL_Renderer * Render = WindowCreate();

    atexit(SDL_Quit);

	synth_init(&synth);

    double note;
    Generator *Ge;
    puts("");
    for (i = 0; i < 100; i++) {
        if (i < 25) { // PRESS
            ADSR[i] = 0.001 + pow(i*0.04,1/3.0);
        } else  // PRESSS -> SUSTAIN
        if (i >= 25 && i < 50) {
            ADSR[i] = exp(-(i-25)*0.04);
        } else // SUSTAIN
        if (i >= 50 && i < 75) {
            ADSR[i] = ADSR[49];
        } else { // RELEASE
            ADSR[i] = ADSR[74] - (i-75)*0.015;
        }  
        printf ("%d:%f ",i, ADSR[i]);
    }
    puts("");

    Tremolo = NewTriangle(tremolo_freq ,synth.Obtained.freq,DEFAULT_GAIN);
    for (i = 0; i < 12; i++) {

        printf ("SampleRate %d Note %s\n",synth.Obtained.freq, NOTES_NAME[i]);
        synth.Event[i] = SILENCE;
          
            // 16'
            note = BASE_C_NOTE * pow(pow(2,1.0/12),i-12);
            Ge = NewSin(note,synth.Obtained.freq, DEFAULT_GAIN);   
           AddGenerator(&synth,i,Ge);

            // 5 1/3'
            note = BASE_C_NOTE * pow(pow(2,1.0/12),i+7);
            Ge = NewSin(note,synth.Obtained.freq, DEFAULT_GAIN);   
            AddGenerator(&synth,i,Ge);

            // 8'
            note = BASE_C_NOTE * pow(pow(2,1.0/12),i);
            Ge = NewSin(note,synth.Obtained.freq, DEFAULT_GAIN);   
            AddGenerator(&synth,i,Ge);

            // 4'
            note = BASE_C_NOTE * pow(pow(2,1.0/12),i+12);
            Ge = NewSin(note,synth.Obtained.freq, DEFAULT_GAIN);   
            AddGenerator(&synth,i,Ge);

            // 2 2/3'
            note = BASE_C_NOTE * pow(pow(2,1.0/12),i+19);
            Ge = NewSin(note,synth.Obtained.freq, DEFAULT_GAIN);   
            AddGenerator(&synth,i,Ge);

            // 2'
            note = BASE_C_NOTE * pow(pow(2,1.0/12),i+24);
            Ge = NewSin(note,synth.Obtained.freq, DEFAULT_GAIN/2);   
            AddGenerator(&synth,i,Ge);

            // 1 3/5'
            note = BASE_C_NOTE * pow(pow(2,1.0/12),i+28);
            Ge = NewSin(note,synth.Obtained.freq, DEFAULT_GAIN/2);   
            AddGenerator(&synth,i,Ge);

            // 1 1/3
            note = BASE_C_NOTE * pow(pow(2,1.0/12),i+31);
            Ge = NewSin(note,synth.Obtained.freq, DEFAULT_GAIN/2);   
            AddGenerator(&synth,i,Ge);

            // 1'
            note = BASE_C_NOTE * pow(pow(2,1.0/12),i+36);
            Ge = NewSin(note,synth.Obtained.freq, DEFAULT_GAIN/2);   
            AddGenerator(&synth,i,Ge);
            
            

}
	


    SDL_UnlockMutex(Mutex);
    SDL_PauseAudioDevice(synth.device, ON);    
    
    while( quit == 0 ){

        SDL_SetRenderDrawColor(Render, 0, 0, 0, 255);
        SDL_RenderClear(Render);
        
        SDL_LockMutex(Mutex);
        DrawWave(Render,1,1,Window.Wsize,Window.Hsize,synth.Wave,2048, 0.30,65535 >> 1);

        SDL_RenderPresent(Render);


            while( SDL_PollEvent( &event ) ){
                switch( event.type ){

                    case SDL_WINDOWEVENT:

                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        Window.Hsize = event.window.data2;
                        Window.Wsize = event.window.data1;
                        printf ("Resized H %d W %d", Window.Hsize, Window.Wsize);
                    }
                    break;
                    case SDL_KEYDOWN:                        
                        switch (event.key.keysym.sym)
                        {
                            case SDLK_ESCAPE:  
                            //case SDLK_q:
                                quit = 1; 
                            break;
                            case SDLK_a:  GeneratorON(&synth,C); break;
                            case SDLK_w:  GeneratorON(&synth,Cs); break;
                            case SDLK_s:  GeneratorON(&synth,D); break;
                            case SDLK_e:  GeneratorON(&synth,Ds); break;
                            case SDLK_d:  GeneratorON(&synth,E); break;
                            case SDLK_f:  GeneratorON(&synth,F); break;
                            case SDLK_t:  GeneratorON(&synth,Fs); break;
                            case SDLK_g:  GeneratorON(&synth,G); break;
                            case SDLK_y:  GeneratorON(&synth,Gs); break;
                            case SDLK_h:  GeneratorON(&synth,A); break;
                            case SDLK_u:  GeneratorON(&synth,As); break;
                            case SDLK_j:  GeneratorON(&synth,B); break;

                            case SDLK_SPACE:
                                //synth.Play ^= 1; 
                                //SDL_PauseAudioDevice(device, synth.Play);
                            break;
                        }
                    break;
                    case SDL_KEYUP:                        
                        switch (event.key.keysym.sym)
                        {
                            case SDLK_a:  GeneratorOFF(&synth,C); break;
                            case SDLK_w:  GeneratorOFF(&synth,Cs); break;
                            case SDLK_s:  GeneratorOFF(&synth,D); break;
                            case SDLK_e:  GeneratorOFF(&synth,Ds); break;
                            case SDLK_d:  GeneratorOFF(&synth,E); break;
                            case SDLK_f:  GeneratorOFF(&synth,F); break;
                            case SDLK_t:  GeneratorOFF(&synth,Fs); break;
                            case SDLK_g:  GeneratorOFF(&synth,G); break;
                            case SDLK_y:  GeneratorOFF(&synth,Gs); break;
                            case SDLK_h:  GeneratorOFF(&synth,A); break;
                            case SDLK_u:  GeneratorOFF(&synth,As); break;
                            case SDLK_j:  GeneratorOFF(&synth,B); break;
                        }
                    break;





                    case SDL_QUIT:
                        quit = 1;
                    break;
                    default:
                    break;
                }

            }
            SDL_UnlockMutex(Mutex);
            fflush(stdout);
            SDL_Delay(30);
    }	

	SDL_CloseAudioDevice(device);
    SDL_Quit();
    exit(0);
}


SDL_Renderer * WindowCreate() {
    SDL_Window* window = SDL_CreateWindow("",
										  SDL_WINDOWPOS_UNDEFINED,
										  SDL_WINDOWPOS_UNDEFINED,
										  512,
										  256,
 										  SDL_WINDOW_RESIZABLE);
    Window.Hsize = 256;
    Window.Wsize = 512;
    return SDL_CreateRenderer(window, -1, 0);
}

void DrawWave(SDL_Renderer * Render, int x, int y, int width, int length, int *data, int buffersize , float xscale, int max) {

        int xi = 0,yi = 0,xold = 0,yold = 0; 
        if (max == 0) max = 1;
        float scale = (float) (length >> 1) / max;
        int   xscaleint = 1.0f / xscale;
        
        for (int i = 0 ; i < buffersize; i += xscaleint ) {
            int c = abs(data[i] * (255 / max)) / 25;
            SDL_SetRenderDrawColor(Render, c, c + 10, c, 100);
            xi = xi + 1;
            SDL_RenderDrawLine(Render,xi + x,(length >> 1) + y,xi + x,y + (length >> 1) - data[i] * scale);
        }
        
        SDL_SetRenderDrawColor(Render, 88, 195, 74, 255);
        xold = x;
        yold =  y + (length >> 1) - data[0] * scale;
        xi = xold + 1;
        yi = y + (length >> 1) - data[1] * scale;
        for (int i = 2 ; i < buffersize; i += xscaleint ) {
            SDL_RenderDrawLine(Render, xold,yold,xi,yi);
            xold = xi;
            yold = yi;
            xi += 1;
            yi = y + (length >> 1) - data[i] * scale;
        }

}

/*

```python
# Definindo parâmetros
num_registers = 12
num_bits = 16  # Número de bits em cada registrador
clock_frequency = 1000000  # Frequência do clock em Hz
base_frequencies = [440.0, 466.16, 493.88, 523.25, 554.37, 587.33, 622.25, 659.26, 698.46, 739.99, 783.99, 830.61]  # Exemplo de frequências para 12 pitches

# Inicializando registros com zeros
registers = [0] * num_registers

# Loop principal
while True:
    for i in range(num_registers):
        # Adiciona constante à cada registrador
        registers[i] += int((base_frequencies[i] * 2**num_bits) / clock_frequency)

        # Checa overflow e emite sinal
        if registers[i] & (1 << (num_bits - 1)):
            output_signal = 1
        else:
            output_signal = 0

        # Adiciona ou subtrai 1 de forma não periódica para introduzir jitter
        registers[i] += random.choice([-1, 1])

        # Restringe o registrador ao número de bits especificado
        registers[i] &= (2**num_bits - 1)

```
*/