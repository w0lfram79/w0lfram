#include <SDL2/SDL.h>
#include <string>

int WIDTH = 800;
int HEIGHT = 800;


long double min = -2.84;
long double max = 1;
int MAX_ITERATIONS = 200;
long double factor = 1;


long double map(long double value, long double in_min, long double in_max, long double out_min, long double out_max)
{
    return(value-in_min)*(out_max-out_min)/(in_max-in_min)+out_min;
}

int main(int argc, char* argv[] )
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event;
	//const Uint8 *keys = SDL_GetKeyboardState(NULL);

	SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);
	//SDL_CreateWindowAndRenderer(1920, 1080, 0, &window, &renderer);
	SDL_RenderSetLogicalSize(renderer,WIDTH,HEIGHT);

	int count = 0;

    bool quit{ false };
    
    while (!quit)
    {
       		while( SDL_PollEvent(&event) ) { // poll the event queue for quit events
			if( event.type == SDL_QUIT ) {
				quit = true;
			}
			else if( event.type == SDL_KEYDOWN ) {
				if( event.key.keysym.scancode == SDL_SCANCODE_ESCAPE ) { quit = true; }
				set_keys(event);
			}
		}
        max -= 0.1 * factor;
        min += 0.15 * factor;
        factor *=0.9349;
        MAX_ITERATIONS +=5;

        if(count > 30){
            MAX_ITERATIONS *=1.02;
        }

        SDL_RenderPresent(renderer);

        for(int x=0;x<WIDTH; x++){
            for(int y=0; y<HEIGHT; y++){
                long double a = map(x, 0, WIDTH, min, max);
                long double b = map(y, 0, HEIGHT, min, max);

                long double ai = a;
                long double bi = b;

                int n = 0;

                for(int i=0;i<MAX_ITERATIONS;i++){
                    long double a1 = a * a - b * b;
                    long double b1 = 2 * a * b;

                    a = a1+ ai;
                    b = b1+bi;

                    if((a + b) > 2){
                        break;
                    }
                    n++;
                }

                int bright = map(n, 0, MAX_ITERATIONS, 0, 255);

                if((n==MAX_ITERATIONS) || bright < 20) {
                        bright = 0;
                }


                //int red=map(bright*bright, 0, 6502,0,255);
                int red=map(bright*bright, 0, 659,0,255);
                int green=bright;
                int blue=map(SDL_sqrt(bright), 0, SDL_sqrt(255),0, 255);

                SDL_SetRenderDrawColor(renderer,red,green,blue,255);
                SDL_RenderDrawPoint(renderer,x,y);
            }
        }

        //SDL_Surface *sshot = SDL_GetWindowSurface(window);
        //SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ABGR8888, sshot->pixels, sshot->pitch);
        //std::string file = std::to_string(count) + ".bmp";
        ////SDL_SaveBMP(sshot, file.c_str());
        //SDL_FreeSurface(sshot);

        count++;
    }
    SDL_Quit();
	return 0;
}
