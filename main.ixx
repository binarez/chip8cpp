// Stephane Duguay et Jean-Sebastien Ross

// Tutorial: https://austinmorlan.com/posts/chip8_emulator/
// Test ROMs: https://github.com/dmatlack/chip8/tree/master/roms/games

import <iostream>;
import <string>;
import <clocale>;
import <cassert>;
import <chrono>;
import <unordered_map>;

#pragma warning( push )
#pragma warning( disable : 26819 )
#include <SDL.h>
#pragma warning( pop )

import VideoBuffer;
import Processeur;
import Memoire;

using namespace std;

//------------------------------------------------------------
// chip8 spec
static const int ECRAN_LARGEUR = 64;
static const int ECRAN_HAUTEUR = 32;
static const int MEMOIRE_TAILLE_OCTETS = 4096;

//------------------------------------------------------------
static const int SDL_RESOLUTION_LARGEUR = 1024;
static const int SDL_RESOLUTION_HAUTEUR = 512;
static const std::unordered_map<SDL_Keycode, octet> SDL_CLES
{
	{ SDLK_1, 0x0 },
	{ SDLK_2, 0x1 },
	{ SDLK_3, 0x2 },
	{ SDLK_4, 0x3 },
	{ SDLK_q, 0x4 },
	{ SDLK_w, 0x5 },
	{ SDLK_e, 0x6 },
	{ SDLK_r, 0x7 },
	{ SDLK_a, 0x8 },
	{ SDLK_s, 0x9 },
	{ SDLK_d, 0xA },
	{ SDLK_f, 0xB },
	{ SDLK_z, 0xC },
	{ SDLK_x, 0xD },
	{ SDLK_c, 0xE },
	{ SDLK_v, 0xF },
};

//------------------------------------------------------------
struct SDLEnv
{
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	bool ok() const { return window != nullptr && renderer != nullptr; }
};

//------------------------------------------------------------
SDLEnv init()
{
	SDLEnv sdlEnv;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		cout << "Erreur d'initialisation de la librairie SDL: " << SDL_GetError() << endl;
		return {};
	}

	sdlEnv.window = SDL_CreateWindow(	"chip8",
										SDL_WINDOWPOS_CENTERED,
										SDL_WINDOWPOS_CENTERED,
										SDL_RESOLUTION_LARGEUR,
										SDL_RESOLUTION_HAUTEUR, 0);
	if (sdlEnv.window)
	{
		sdlEnv.renderer = SDL_CreateRenderer(sdlEnv.window, -1, SDL_RENDERER_ACCELERATED);
	}

	return sdlEnv;
}

//------------------------------------------------------------
void render(SDL_Renderer& renderer, IVideoBuffer& video)
{
	SDL_SetRenderDrawColor(&renderer, 0, 0, 0, 255);	// Fond noir
	SDL_RenderClear(&renderer);
	SDL_SetRenderDrawColor(&renderer, 255, 255, 255, 255);	// Pixel blanc

	const float pixelRatioX{ static_cast<float>(SDL_RESOLUTION_LARGEUR) / static_cast<float>(ECRAN_LARGEUR) };
	const float pixelRatioY{ static_cast<float>(SDL_RESOLUTION_HAUTEUR) / static_cast<float>(ECRAN_HAUTEUR) };

	for (int y = 0; y < ECRAN_HAUTEUR; ++y)
	{
		for (int x = 0; x < ECRAN_LARGEUR; ++x)
		{
			if (video.pixel(x, y))
			{
				SDL_Rect rect{ static_cast<int>(x * pixelRatioX), static_cast<int>(y * pixelRatioY), pixelRatioX, pixelRatioY };
				SDL_RenderFillRect(&renderer, &rect);
			}
		}
	}

	SDL_RenderPresent(&renderer);
}

//------------------------------------------------------------
void go(SDLEnv& env, IProcesseur & processeur, IVideoBuffer& video)
{
	assert(env.ok());

	const float frequenceMAJhz{ 120.0f };
	const float tempsMAJms{ 1000.0f / frequenceMAJhz };
	auto dernierTick{ std::chrono::high_resolution_clock::now() };

	while (true)
	{
		const auto maintenant{ std::chrono::high_resolution_clock::now() };
		const float delta{ std::chrono::duration<float, std::chrono::milliseconds::period>(maintenant - dernierTick).count() };
		if( delta < tempsMAJms)	// 60 Hz
			continue;

		SDL_Event event;
		if (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				break;
			}
			else if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					break;
				}
				else
				{
					const auto itCle{ SDL_CLES.find(event.key.keysym.sym) };
					if (itCle != SDL_CLES.end())
					{
						processeur.appuyerToucheClavier(itCle->second);
					}
				}
			}
			else if (event.type == SDL_KEYUP)
			{
				const auto itCle{ SDL_CLES.find(event.key.keysym.sym) };
				if (itCle != SDL_CLES.end())
				{
					processeur.relacherToucheClavier(itCle->second);
				}
			}
		}

		processeur.tick();
		render(*(env.renderer), video);
		dernierTick = maintenant;
	}
}

//------------------------------------------------------------
void bye(SDLEnv& env)
{
	if (env.renderer)
		SDL_DestroyRenderer(env.renderer);
	if (env.window)
		SDL_DestroyWindow(env.window);
	SDL_Quit();
	env = {};
}

//------------------------------------------------------------
int main(int argc, char* argv[])
{
	setlocale(0, "");	// 0 = LC_ALL

	if (argc != 2)
	{
		cout << "chip8cpp v0.99" << endl;
		cout << "SVP spécifier le fichier de ROM à lancer." << endl;
		cout << argv[0] << " FICHIER_ROM" << endl;
		return -1;
	}

	SDLEnv env = init();
	if (!env.ok())
	{
		return -2;
	}

	VideoBuffer video(ECRAN_LARGEUR, ECRAN_HAUTEUR);
	Memoire memoire(MEMOIRE_TAILLE_OCTETS);
	Processeur processeur(memoire, video);
	if (!processeur.charger(argv[1]))
	{
		cout << "Erreur lors de l'exécution de la ROM. Fichier invalide." << endl;
		return -2;
	}

	go(env, processeur, video);
	cout << "ROM exécutée avec succès." << endl;

	bye(env);
	return 0;
}
