//#define BMPHEADER_SIZE 54

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include <SDL2/SDL.h>

// there will be a low level I/O function from the operating system
extern long write(int, const char *, unsigned long);


float zoom      = 1.5;
float quadLimit = 3.0;
char colorLimit = 40;

long double min = -2.84;
long double max = 1;
int MAX_ITERATIONS = 120;
long double factor = 1;

typedef enum { FALSE, TRUE } boolean;

typedef struct Complex_s {
	float re;
	float im;
} Complex;

typedef struct calculateData {
	char ** lines;
	int yStart;
	int yEnd;
	int width;
	int height;
	float imageRelation;
	int threadNum;
} calculateData;

long double map(long double value, long double in_min, long double in_max, long double out_min, long double out_max)
{
    return(value-in_min)*(out_max-out_min)/(in_max-in_min)+out_min;
}

void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = (b[i] >> j) & 1;
           // fprintf(stderr, "%u", byte);
        }
    }
    //puts("");
}

// bad, but fast !!!
int intFloor(double x) {
	return (int)(x+100000) - 100000;
}

// count chars until \0 or space or "to long"
int len(char * str) {
	int ddorf=0;
	while (str[ddorf] != '\0' && str[ddorf] != ' ' && ddorf != 40225) ++ddorf;
	return ddorf;
}

// read a positive number from a char array
int str2num(char * str) {
	int result = 0;
	int b = 1;
	int l = len(str);
	int i;
	for(i=1; i<l; ++i) b *= 10;
	for(i=0; i<l; ++i) {
		result += b * (int)(str[i] - '0');
		b /= 10;
	}
	return result;
}

void toRGB(int id, char * blueGreenRed) {
	blueGreenRed[0] = 0;
	blueGreenRed[1] = 0;
	blueGreenRed[2] = 0;
	if ( id == colorLimit ) return;
	
	float hi,q,t,coeff;

	coeff = 7.0 * (id/(float)colorLimit);
	hi = intFloor(coeff);
	t = coeff - hi;
	q = 1 - t;
	if (hi == 0.0) {
		blueGreenRed[2] = 0;
		blueGreenRed[1] = t*255; //immer mehr green und blau -> dunkelblau zu cyan
		blueGreenRed[0] = t*127 + 128;
	} else if (hi == 1.0) {
		blueGreenRed[2] = t*255; //immer mehr rot -> cyan zu weiss
		blueGreenRed[1] = 255;
		blueGreenRed[0] = 255;
	} else if (hi == 2.0) {
		blueGreenRed[2] = 255;
		blueGreenRed[1] = 255;
		blueGreenRed[0] = q*255; // immer weniger blau -> weiss zu gelb
	} else if (hi == 3.0) {
		blueGreenRed[2] = 255;
		blueGreenRed[1] = q*127 + 128; // immer weniger green -> gelb zu orange
		blueGreenRed[0] = 0;
	} else if (hi == 4.0) {
		blueGreenRed[2] = q*127 + 128; // orange wird dunkler -> orange zu braun
		blueGreenRed[1] = q*63 + 64;
		blueGreenRed[0] = 0;
	} else if (hi == 5.0) {
		blueGreenRed[2] = 128;
		blueGreenRed[1] = 64;
		blueGreenRed[0] = t*128; // mehr blau -> braun zu violett
	} else if (hi == 6.0) {
		blueGreenRed[2] = q*128; // weniger rot und green -> violett wird dunkelblau
		blueGreenRed[1] = q*64;
		blueGreenRed[0] = 128;
	}
}

void* calculate (void* arg){
	calculateData* cd = (calculateData*)arg;
//	fprintf(stderr, "Thread %i: Starting calculation of %i lines from %i to %i!\n", cd->threadNum, cd->yEnd - cd->yStart + 1, cd->yStart, cd->yEnd);
	char blueGreenRed[3];
	int x, y;
	for (y = cd->yStart; y <= cd->yEnd; y++) {
		// Allocate Memory for this line
		cd->lines[y - 1] = malloc(cd->width * sizeof(char) * 3);
			
		char iterate=0;
		Complex c    = {0,0};
		Complex newz = {0,0};
		
		for (x=1; x <= cd->width; ++x) {
			Complex z = {0,0};
			float quad=0;
			
			//c.re = zoom * (-1.0 + cd->imageRelation * ( (x-1.0) / (cd->width-1.0)) );
            //c.im = zoom * ( 0.5 - (y-1.0) / (cd->height-1.0) );
            
            c.re = map((x-1), 0, (cd->width-1), min, max);
            c.im = map((y-1), 0, (cd->height-1), min, max);
			
			// iterate
			for ( iterate=1; iterate < colorLimit && quad < quadLimit; ++iterate ) {
				quad = z.re * z.re + z.im * z.im;
				
				newz.re = (z.re * z.re) - (z.im * z.im) + c.re;
				newz.im =  z.re * z.im * 2.0            + c.im;
				
				z = newz;
			}
			toRGB(iterate, blueGreenRed);
			//Has to be x-1 and y-1 because x and y start at 1
			cd->lines[y-1][(x-1)*3] = blueGreenRed[0];
			cd->lines[y-1][(x-1)*3 + 1] = blueGreenRed[1];
			cd->lines[y-1][(x-1)*3 + 2] = blueGreenRed[2];
		}
		//fprintf(stderr, "Line %i done!\n", y);
	}
//	fprintf(stderr, "Thread %i finished!\n", cd->threadNum);
}

int main(int argc, char ** argv, char ** envp) {
	int width  = str2num(argv[1]);
	int height = str2num(argv[2]);
	int numThreads = str2num(argv[3]);
	int maxSimultThreads = str2num(argv[4]);
	
	int i;
	
	float imageRelation = (float)width/(float)height;
	
	//Allocate Memory for line pointers
	char** lines = malloc(height * sizeof(char*));
	
	
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event;

	SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
	SDL_RenderSetLogicalSize(renderer,width,height);
	
	int count = 0;
	
	boolean quit = FALSE;
	
	while (!quit)
    {
       		while( SDL_PollEvent(&event) ) { // poll the event queue for quit events
			if( event.type == SDL_QUIT ) {
				quit = TRUE;
			}
			else if( event.type == SDL_KEYDOWN ) {
				if( event.key.keysym.scancode == SDL_SCANCODE_ESCAPE ) { quit = TRUE; }
				
			}
		}
		//Allocating memory for data structs
		calculateData* cds = malloc(numThreads * sizeof(calculateData));
		
		//Allocating memory for threads
		pthread_t* threads = malloc(numThreads * sizeof(pthread_t));
		
		int linesPerThread = height/numThreads;
//		fprintf(stderr, "Number of Threads: %i, Lines per Thread: %i, Max. simultaneous Threads: %i\n", numThreads, linesPerThread, maxSimultThreads);
		
		//Initialising data structs and starting calculation
		for (int i = 0; i < numThreads; i++){
			cds[i].threadNum = i;
			cds[i].lines = lines;
			cds[i].imageRelation = imageRelation;
			cds[i].width = width;
			cds[i].height = height;
			cds[i].yStart = i * linesPerThread + 1;
			if (i == numThreads - 1){ //Last Thread
				cds[i].yEnd = height;
			} else {
				cds[i].yEnd = (i+1) * linesPerThread;
			}
//			fprintf(stderr, "Starting Thread %i at address %p: yStart = %i, yEnd = %i\n", i, &(threads[i]), cds[i].yStart, cds[i].yEnd);
			
			if (i > maxSimultThreads){
//				fprintf(stderr, "Waiting for Thread %i to terminate in order to start Thread %i...\n", i - maxSimultThreads, i);
				pthread_join(threads[i-maxSimultThreads],NULL);
			}
			
			pthread_create(&(threads[i]), NULL, calculate, &(cds[i]));
			
			//calculate(cds[i]);
		}

		//Waiting for threads to terminate
//		fprintf(stderr, "Main is waiting for threads to terminate...\n");
		for (int i = 0; i < numThreads; i++){
			pthread_join(threads[i], NULL);
		}

		// Writing output
//		fprintf(stderr, "Writing to file...\n");
		
		SDL_RenderPresent(renderer);
		
		for(int y=0; y<height; y++){
			for(int x=0;x<width; x++){
				
				//int blue=(int)lines[y][x*3];
                //int green=(int)lines[y][x*3+1];
                //int red=(int)lines[y][x*3+2];

				SDL_SetRenderDrawColor(renderer,lines[y][x*3+2],lines[y][x*3+1],lines[y][x*3],255);
				//SDL_SetRenderDrawColor(renderer,red,green,blue,255);
                SDL_RenderDrawPoint(renderer,x,y);
			}
		}
		// free memory
		for (i = 0; i < height; i++){
			free(lines[i]);
		}
		count++;
        //zoom=zoom-0.01;
        max -= 0.1 * factor;
        min += 0.15 * factor;
        factor *=0.9349;

        if((count > 5) && colorLimit < MAX_ITERATIONS){
            colorLimit *=1.05;
        }
	}
	free(lines);
	SDL_Quit();
	return 0;
}
