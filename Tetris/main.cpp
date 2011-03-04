#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_mixer.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <vector>


//Global variables
enum screen {SCREEN_WIDTH = 640, SCREEN_HEIGHT = 480, SCREEN_BPP = 32, MAX_FPS = 60};
SDL_Surface * screen = NULL;
Uint32 speed = 1000;
std::ofstream debug_out;
TTF_Font * font = NULL;
Uint16 currentFPS = NULL;
Uint16 Grid[20][10] = 
{{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0},
};
Uint16 Current[4][4] = {{0}};
Uint16 Next[4][4] = {{0}};
SDL_Rect grid;
SDL_Rect fps;
SDL_Rect dst;
SDL_Color textColor = {0,0, 0};
Uint32 colors[10];
Sint16 xpos = 4, ypos = 0;
Uint32 score = 0;
Uint16 level = 1;
Uint16 eliminated = 0;
//Sound files
Mix_Music *music = NULL;
Mix_Chunk * drop = NULL;
Mix_Chunk * rotate = NULL;
Mix_Chunk * rowsound = NULL;
Mix_Chunk * tetra = NULL;
Mix_Chunk * levelup = NULL;
Mix_Chunk * tryagain = NULL;
//

class Timer
{
private:
	Uint32 start;
	Uint32 last;
	Uint32 delta;
	bool running;
public:
	Timer() { start = 0; last = 0; delta = 0; running = false;}
	~Timer(){};
	void Start() {start = SDL_GetTicks(); running = true;}
	//void Stop() {last = start;}
	Uint32 GetTime() {if (running == false) return 0; last = SDL_GetTicks(); return(last - start);}
	Uint32 DeltaTime() {if (running == false) return 0; last = start; start = SDL_GetTicks(); delta = start - last; return delta;}
	Uint32 LastTime() {if (running == false) return 0; return last;}
	//bool DeltaTime(Uint32 time) {if (running == false) return 0; delta = SDL_GetTicks() - start;  if (delta > time) {start-=time; return true;} return false;}
} inputTimer;

class Explode
{
private:
	bool sound;
	Timer time;
	Uint32 interval;
	//Uint32 last;
	Uint32 max;
	Sint16 row;
public:
	Explode(Uint16 r);
	~Explode(){};
	bool Update();
	void Adjust(Uint16 r);
	const Uint16 GetRow() {return row;}

};
std::vector <Explode> completeRows;

Explode::Explode(Uint16 r) : interval(10), max(1000), row(r), sound(false)
{
	time.Start();
}
bool Explode::Update()
{
	if (sound == false)
	{
		Mix_PlayChannel(-1, rowsound, 0);
		sound = true;
	}
	if (time.GetTime() > max)
	{
		for (Uint16 r = row; r > 0; --r)
		{
			for (Uint16 j = 0; j < 10; ++j)
				//Grid[row][j] = Grid[row][j];
				Grid[r][j] = Grid[r-1][j];
		}
		row = 0;
		return true;
	}
	if (time.LastTime() > interval)
	{
		//last = time.GetTime();
		for (Uint16 c = 0; c < 10; ++c)
		{
			if (Grid[row][c] != 9)
				Grid[row][c] = 9;
			else
				Grid[row][c] = 8;
		}
	}
	return false;
}

void Explode::Adjust(Uint16 r)
{
	if(row < r)
		--row;
}

//Function prototypes
Sint16 Initialize();
bool HandleInput();
void DrawScreen();
void RandomDrop();
void ApplyBlock(Uint16 &type, Uint16 block[][4]);
std::ostream & time(std::ostream & out);
void Exit();
bool Down();
bool Left();
bool Right();
void Rotate();
void Dropped();
void GameOver();
void CheckRows();
//

int main( int argc, char* args[] ) 
{ 
	//Close game if unable to initialize
	if(Initialize() < 0)
		return 1;

	bool gameRunning = true;
	Uint32 frame = 0;

	Timer FPS;
	FPS.Start();
	Timer DropTimer;
	DropTimer.Start();

	while (gameRunning)
	{
		SDL_Event event;

		if (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				gameRunning = false;
				break;
			}
		}
		gameRunning = HandleInput();
		DrawScreen();
		SDL_Flip(screen);
		frame++;
		if (FPS.GetTime() > 1000)
		{
			currentFPS = (int)(frame/(FPS.GetTime()/1000.f));
			frame = 0;
			FPS.Start();
			//debug_out << "GetTime/1000 = " << (FPS.GetTime()/1000.f) << "frame = " << frame << "fps = " << currentFPS << "\n";
		}
		if (DropTimer.GetTime() > 1000)
		{
			DropTimer.Start();
			Uint8 * keys = SDL_GetKeyState(NULL);
			if (!keys[SDLK_DOWN])
				Down();
		}
		Uint16 amount = 0;
		for (Uint16 i = 0; i < completeRows.size(); ++i)
		{
			if (completeRows[i].Update())
			{
				score += completeRows.size() * (level) * 10;
				eliminated++;
				amount++;
			}
		}
		if (amount >3)
			Mix_PlayChannel( -1, tetra, 0 );

		for (Uint16 i = 0; i < completeRows.size(); ++i)
		{
			if (completeRows[i].GetRow() == 0)
			{
				completeRows.erase(completeRows.begin() + i);
				amount++;
			}
		}

		if (eliminated > 10)
		{
			eliminated = 0;
			level++;
			Mix_PlayChannel( -1, levelup, 0 );
		}

	}


	return 0;
} 

Sint16 Initialize()
{
	srand((unsigned)time(0));
	Sint16 Status = 0;
	//Debug file output
	const char * debug_file = "debug.txt";
	debug_out.open(debug_file);
	if (!debug_out.is_open())
	{
		std::cerr << time << "Unable to open " << debug_file << " for debug output.  Now closing.\n";
		Status = -1;
		return(Status);
	}

	//Start SDL 
	if (Status = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO ) == -1)
	{
		debug_out << time << "Failed to initialize SDL: " << SDL_GetError();
	}

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE|SDL_DOUBLEBUF);
	if (screen == NULL)
	{
		debug_out << time << "Failed to create " << SCREEN_WIDTH << "x" <<SCREEN_HEIGHT << "x" << SCREEN_BPP << " screen: " << SDL_GetError();
		Status = -1;

	}

	if (Status = TTF_Init() == -1 )
	{ 
		debug_out << time << "Failed to initialize TTF.\n";
	} 

	font = TTF_OpenFont( "MacType.ttf", (int)(SCREEN_WIDTH/22)); 

	if (font == NULL)
	{
		debug_out << time << "Failed to load TTF font.\n";
		Status = -1;
	}

	//Initialize SDL_mixer
	
	if( Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096 ) == -1 )
	{
		Status = -1;
	}
	else
	{
		music = Mix_LoadMUS("audio/music.ogg");
		if (music == NULL)
			debug_out << time << "Failed to load music.  audio/music.ogg\n";
		else
			Mix_PlayMusic( music, -1 );
	}

	drop = Mix_LoadWAV("audio/drop.wav");
	if (drop == NULL)
		debug_out << time << "Failed to load audio/drop.wav\n";
	
	rotate = Mix_LoadWAV("audio/rotation.wav");
	if (rotate == NULL)
		debug_out << time << "Failed to load audio/rotation.wavn\n";

	rowsound = Mix_LoadWAV("audio/row.wav");
	if (rowsound == NULL)
		debug_out << time << "Failed to load audio/row.wav\n";

	tetra = Mix_LoadWAV("audio/tetra.wav");
	if (tetra == NULL)
		debug_out << time << "Failed to load audio/tetra.wav\n";

	levelup = Mix_LoadWAV("audio/levelup.wav");
	if (levelup == NULL)
		debug_out << time << "Failed to load audio/levelup.wav\n";

	tryagain = Mix_LoadWAV("audio/tryagain.wav");
	if (tryagain == NULL)
		debug_out << time << "Failed to load audio/tryagain.wav\n";


	SDL_WM_SetCaption("Tetris", NULL);
	debug_out << time << "Tetris has initialized successfully.\n";
	atexit(Exit);



	//Generate dimensions
	grid.h = SCREEN_HEIGHT / 24;
	grid.w = SCREEN_WIDTH / 32;
	grid.y = grid.h * 2;
	grid.x = grid.w * 4;

	//dest for FPS
	SDL_Rect dst;
	dst.x = SCREEN_WIDTH - 10 * grid.w;
	dst.y = SCREEN_WIDTH - 10 * grid.h;
	dst.w = 0;
	dst.h = 0;

	//Generate colors
	colors[0] = SDL_MapRGB(screen->format, 0,0,0);
	for(Uint16 i = 1; i < 10; ++i)
	{
		colors[i] = SDL_MapRGB(screen->format, rand()%225 + 30, rand()%225 +30, rand()%225 +30);
	}

	//Draw background and static elements
		//Fill entire background
	Uint32 backgroundColor = SDL_MapRGB(screen->format, 140,209,242);
	SDL_FillRect(screen, NULL, backgroundColor);
		//"FPS:" text
	SDL_Surface * temp;
	temp = TTF_RenderText_Solid(font, "FPS: ", textColor);
	SDL_BlitSurface(temp, NULL, screen, &dst); 
	SDL_FreeSurface(temp);
		//"Next" text
	dst.x = grid.w * 16;
	dst.y = grid.h * 2;
	temp = TTF_RenderText_Solid(font, "Next", textColor);
	SDL_BlitSurface(temp, NULL, screen, &dst);
	SDL_FreeSurface(temp);
		//"score"
	dst.x = grid.w * 16;
	dst.y = grid.h * 9;
	temp = TTF_RenderText_Solid(font, "Score:", textColor);
	SDL_BlitSurface(temp, NULL, screen, &dst);
	SDL_FreeSurface(temp);
		//"level"
	dst.x = grid.w * 16;
	dst.y = grid.h * 13;
	temp = TTF_RenderText_Solid(font, "Level:", textColor);
	SDL_BlitSurface(temp, NULL, screen, &dst);
	SDL_FreeSurface(temp);
		//Locate FPS rect for future rendering
	fps.x = SCREEN_WIDTH - 5 * grid.w;
	fps.y = SCREEN_WIDTH - 10 * grid.h;
	fps.w = 0;
	fps.h = 0;
		//Generate first "next" block then perform standard RandomDrop() to move next to now and make a new next
	Uint16 type = (rand()%7); 
	ApplyBlock(type, Next);
	RandomDrop();
		//Start inputTimer for input speed
	inputTimer.Start();

	return(Status);

}

bool HandleInput()
{
	static Uint8 * keys;

	keys = SDL_GetKeyState(NULL);
	if (keys[SDLK_ESCAPE])
		return false;
	
	if (inputTimer.GetTime() > 80)
	{
		//if (keys[SDLK_SPACE])
		//{
		//	RandomDrop();
		//}
		if (keys[SDLK_DOWN])
		{
			Down();
		}
		if (keys[SDLK_LEFT])
		{
			Left();
		}
		if (keys[SDLK_RIGHT])
		{
			Right();
		}
		if (keys[SDLK_UP])
		{
			Rotate();
			keys[SDLK_UP] = false;
		}
		if (keys[SDLK_SPACE])
		{
			Rotate();
			keys[SDLK_SPACE] = false;
		}
		if (keys[SDLK_r])
		{
			Rotate();
			keys[SDLK_r] = false;
		}
		if (keys[SDLK_d])
			Dropped();
		inputTimer.Start();
	}
	return true;
}

void DrawScreen()
{
	dst = grid;
	for (Uint16 r = 0; r < 20; ++r)
	{
		for (Uint16 c = 0; c < 10; ++c)
		{
			SDL_FillRect(screen, &dst, colors[Grid[r][c]]);
			dst.x += dst.w;
		}
		dst.x = grid.x;
		dst.y += dst.h;
	}

	//dst.w = grid.w*10;
	//dst.h = 1;
	//dst.x = grid.x;
	//for (Uint16 r = 0; r < 20; ++r)
	//{
	//	dst.y = r * grid.h + grid.y;
	//	SDL_FillRect(screen, &dst, SDL_MapRGB(screen->format, 255,255,255));
	//}


	static SDL_Surface * temp;
	//A string for 4 digits conversion from i to string in itoa to render FPS on screen
	char buf[10];
	_itoa_s(currentFPS, buf, 5, 10);
	temp = TTF_RenderText_Solid(font, buf, textColor);
	dst = fps;
	dst.w = temp->w+20;
	dst.h = temp->h;
	Uint32 color = SDL_MapRGB(screen->format, 140,209,242);
	SDL_FillRect(screen, &dst, color);
	SDL_BlitSurface(temp, NULL, screen, &fps);
	SDL_FreeSurface(temp);
	//score
	_itoa_s(score, buf, 9, 10);
	temp = TTF_RenderText_Solid(font, buf, textColor);
	dst.x = 16 * grid.w;
	dst.y = 11 * grid.h;
	dst.w = 100;
	dst.h = temp->h;
	SDL_FillRect(screen, &dst, color);
	SDL_BlitSurface(temp, NULL, screen, &dst);
	SDL_FreeSurface(temp);
	//level
	_itoa_s(level, buf, 9, 10);
	temp = TTF_RenderText_Solid(font, buf, textColor);
	dst.x = grid.w * 16;
	dst.y = grid.h * 15;
	dst.w = 50;
	dst.h = temp->h;
	SDL_FillRect(screen, &dst, color);
	SDL_BlitSurface(temp, NULL, screen, &dst);
	SDL_FreeSurface(temp);


	dst = grid;
	dst.y += ypos * dst.h;
	dst.x += xpos * dst.w;
	for (Uint16 r = 0; r < 4; ++r)
	{
		for (Uint16 c = 0; c < 4; ++c)
		{
			if (Current[r][c] > 0)
				SDL_FillRect(screen, &dst, colors[Current[r][c]]);
			dst.x += dst.w;
		}
		dst.x = grid.x + xpos * dst.w;
		dst.y += dst.h;
	}



}

void RandomDrop()
{

	static Uint16 type;
	//static Uint16 rotation;

	type = (rand()%7); 
	for (Uint16 r = 0; r < 4; ++r)
	{
		for (Uint16 c = 0; c < 4; ++c)
		{
			Current[r][c] = Next[r][c];
		}
	}
	ApplyBlock(type, Next);


}

void ApplyBlock(Uint16 &type, Uint16 block[][4])
{
	for (Uint16 r = 0; r < 4; ++r)
	{
		for (Uint16 c = 0; c < 4; ++c)
		{
			block[r][c] = 0;
		}
	}
	switch (type)
	{
	case 0: 
		//I
		block[1][0] = 1;
		block[1][1] = 1;
		block[1][2] = 1;
		block[1][3] = 1;
		break;
	case 1:
		//L
		block[1][1] = 2;
		block[1][2] = 2;
		block[2][2] = 2;
		block[3][2] = 2;
		break;
	case 2:
		//Back L
		block[1][1] = 3;
		block[1][2] = 3;
		block[2][1] = 3;
		block[3][1] = 3;
		break;
	case 3:
		//Z
		block[0][1] = 4;
		block[1][1] = 4;
		block[1][2] = 4;
		block[2][2] = 4;
		break;
	case 4:
		//Back Z;
		block[0][2] = 5;
		block[1][1] = 5;
		block[1][2] = 5;
		block[2][1] = 5;
		break;
	case 5:
		//[]
		block[1][1] = 6;
		block[1][2] = 6;
		block[2][1] = 6;
		block[2][2] = 6;
		break;
	case 6:
		//W
		block[2][0] = 7;
		block[1][1] = 7;
		block[2][1] = 7;
		block[2][2] = 7;
		break;
	default:
		debug_out << time << "Random number out of bounds error. (Next block generation)\n";
		break;
	}

	dst.w = grid.w;
	dst.h = grid.h;
	dst.x = grid.w * 16;
	dst.y = grid.h*4;
	for (Uint16 r = 0; r < 4; ++r)
	{
		for (Uint16 c = 0; c < 4; ++c)
		{
			SDL_FillRect(screen, &dst, colors[block[r][c]]);
			dst.x += dst.w;
		}
		dst.x = grid.w * 16;
		dst.y += dst.h;
	}

}

std::ostream & time(std::ostream & out)
{
	time_t rawtime;
	struct tm * timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	//out << asctime (timeinfo);
	char * x = asctime (timeinfo);
	x[strlen(x)-1] = '\0';
	out << "<" << x << "> ";
	return out;
}
void Exit()
{
	TTF_CloseFont(font);
	TTF_Quit(); 
	if (music)
		Mix_FreeMusic(music);
	if (drop)
		Mix_FreeChunk(drop);
	if (rotate)
		Mix_FreeChunk(rotate);
	if (rowsound)
		Mix_FreeChunk(rowsound);
	if (tetra)
		Mix_FreeChunk(tetra);
	if (levelup)
		Mix_FreeChunk(levelup);
	if (tryagain)
		Mix_FreeChunk(tryagain);
	Mix_CloseAudio();
	SDL_Quit();
	debug_out << time << "Tetris has closed successfully.\n";
	debug_out.close();
}

bool Down()
{
	for (Uint16 r = 3; r > 0; --r)
	{
		for (Uint16 c = 0; c < 4; ++c)
		{
			if ((Current[r][c] > 0 && Grid[r+1+ypos][c+xpos] > 0) || (Current[r][c] > 0 && ypos+1+r >= 20))
			{
				Dropped();
				return false;
			}
		}
	}
	ypos++;
	return true;
}

bool Right()
{
	for (Uint16 r = 0; r < 4; ++r)
	{
		for (Uint16 c = 3; c > 0; --c)
		{
			if ((Current[r][c] > 0 && xpos+c+1 >= 10) || (Current[r][c] > 0 && Grid[r+ypos][c+xpos+1] > 0))
				return false;
		}
	}
	xpos++;
	return true;
}

bool Left()
{
	for (Uint16 r = 0; r < 4; ++r)
	{
		for (Uint16 c = 0; c < 4; ++c)
		{
			if ((Current[r][c] > 0 && xpos+c-1 < 0) || (Current[r][c] > 0 && Grid[ypos+r][xpos-1+c] > 0))
				return false;
		}
	}
	xpos--;
	return true;
}

void Rotate()
{
	Uint16 temp[4][4];

	for (Uint16 r = 0; r < 4; ++r)
	{
		for (Uint16 c = 0; c < 4; ++c)
		{
			temp[r][c] = Current[3-c][r];
			if ((temp[r][c] > 0 && Grid[r+ypos][c+xpos] >0) || (temp[r][c] > 0 && xpos+c >= 10)
					|| (temp[r][c] > 0 && xpos+c < 0) ||(temp[r][c] > 0 && ypos+r >= 20))
					return;
		}
	}

	Mix_PlayChannel( -1, rotate, 0 );

	

	for (Uint16 r = 0; r < 4; ++r)
	{
		for (Uint16 c = 0 ;c < 4; ++c)
		{
			Current[r][c] = temp[r][c];
		}
	}

}

void Dropped()
{
	Mix_PlayChannel(-1, drop, 0 );
	bool over = false;
	for (Uint16 r = 0; r < 4; ++r)
	{
		for (Uint16 c = 0; c < 4; ++c)
		{
			if (Current[r][c])
				if (Grid[r+ypos][c+xpos])
				{
					over = true;
				}
				else
					Grid[r+ypos][c+xpos] = Current[r][c];

		}
	}
	xpos = 4;
	ypos = 0;

	RandomDrop();
	CheckRows();
	if (over)
		GameOver();
}

void GameOver()
{
	Mix_HaltMusic();

	debug_out << time << "Game over.\n";
	Timer GreyScreen;
	GreyScreen.Start();
	Uint32 i = 0, r = 0, c = 0;

	dst = grid;

	Mix_PlayChannel( -1, tryagain, 0 );
	//Mix_PlayChannel(-1, rowsound, 0);
	while (i < 200)
	{
		if (GreyScreen.GetTime() > 10)
		{
			++i;
			dst.x = grid.x + c * grid.w;
			dst.y = grid.y + r * grid.h;
			SDL_FillRect(screen, &dst, SDL_MapRGB(screen->format, 150,150,150));
			++c;
			if (c >= 10)
			{
				c = 0;
				++r;
			}
			GreyScreen.Start();
			SDL_Flip(screen);
		}
	}



	for (Uint16 r = 0; r < 20; ++r)
		for (Uint16 c = 0; c < 10; ++c)
			Grid[r][c] = 0;

	score = 0;
	level = 1;
	eliminated = 0;

	Mix_PlayMusic(music, -1);

}

void CheckRows()
{
	Uint16 r;
	bool wholeRow = true;
	for (r = 0; r < 20; ++r)
	{
		for (Uint16 c = 0; c < 10; ++c)
		{
			if (wholeRow == false)
				break;
			if (!(Grid[r][c]))
				wholeRow = false;				
		}
		if (wholeRow == true)
		{
			completeRows.push_back(Explode(r));
			debug_out << time << "whole row!\n";
		}
		wholeRow = true;
	}
}