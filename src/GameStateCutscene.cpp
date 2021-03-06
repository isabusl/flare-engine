/*
Copyright © 2012-2013 Henrik Andersson

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

#include "CommonIncludes.h"
#include "GameStateCutscene.h"
#include "GameStatePlay.h"
#include "FileParser.h"
#include "WidgetScrollBox.h"

using namespace std;

Scene::Scene() : frame_counter(0)
	, pause_frames(0)
	, caption("")
	, caption_size(0,0)
	, art(NULL)
	, sid(-1)
	, caption_box(NULL)
	, done(false) {
}

Scene::~Scene() {

	SDL_FreeSurface(art);
	delete caption_box;

	while(!components.empty()) {
		if (components.front().i != NULL) SDL_FreeSurface(components.front().i);
		components.pop();
	}
}

bool Scene::logic() {
	if (done) return false;

	bool skip = false;
	if (inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
		inpt->lock[MAIN1] = true;
		skip = true;
	}
	if (inpt->pressing[ACCEPT] && !inpt->lock[ACCEPT]) {
		inpt->lock[ACCEPT] = true;
		skip = true;
	}
	if (inpt->pressing[CANCEL] && !inpt->lock[CANCEL]) {
		inpt->lock[CANCEL] = true;
		done = true;
	}

	/* Pause until specified frame */
	if (!skip && pause_frames != 0 && frame_counter < pause_frames) {
		++frame_counter;
		return true;
	}

	/* parse scene components until next pause */
	while (!components.empty() && components.front().type != "pause") {

		if (components.front().type == "caption") {

			font->setFont("font_captions");
			caption = components.front().s;
			caption_size = font->calc_size(caption, (int)(VIEW_W * 0.8f));

			delete caption_box;
			caption_box = new WidgetScrollBox(screen->w,caption_size.y);
			caption_box->pos.x = 0;
			caption_box->pos.y = screen->h - caption_size.y;
			font->renderShadowed(caption, screen->w / 2, 0,
								 JUSTIFY_CENTER,
								 caption_box->contents,
								 (int)(VIEW_W * 0.8f),
								 FONT_WHITE);

		}
		else if (components.front().type == "image") {

			if (art)
				SDL_FreeSurface(art);

			art = components.front().i;

			art_dest.x = (VIEW_W/2) - (art->w/2);
			art_dest.y = (VIEW_H/2) - (art->h/2);
			art_dest.w = art->w;
			art_dest.h = art->h;

		}
		else if (components.front().type == "soundfx") {
			if (sid != 0)
				snd->unload(sid);

			sid = snd->load(components.front().s, "Cutscenes");
			snd->play(sid);
		}

		components.pop();
	}

	/* check if current scene has reached the end */
	if (components.empty())
		return false;

	/* setup frame pausing */
	frame_counter = 0;
	pause_frames = components.front().x;
	components.pop();

	return true;
}

void Scene::render() {
	SDL_Rect r = art_dest;
	if (art != NULL)
		SDL_BlitSurface(art, NULL, screen, &r);

	if (caption != "") {
		caption_box->render();
	}
}

GameStateCutscene::GameStateCutscene(GameState *game_state)
	: previous_gamestate(game_state)
	, game_slot(-1) {
	scale_graphics = false;
}

GameStateCutscene::~GameStateCutscene() {
}

void GameStateCutscene::logic() {

	if (scenes.empty()) {
		if (game_slot != -1) {
			GameStatePlay *gsp = new GameStatePlay();
			gsp->resetGame();
			gsp->game_slot = game_slot;
			gsp->loadGame();

			previous_gamestate = gsp;
		}

		/* return to previous gamestate */
		delete requestedGameState;
		requestedGameState = previous_gamestate;
		return;
	}

	while (!scenes.empty() && !scenes.front().logic())
		scenes.pop();
}

void GameStateCutscene::render() {
	if (!scenes.empty())
		scenes.front().render();
}

bool GameStateCutscene::load(std::string filename) {
	FileParser infile;

	// @CLASS Cutscene|Description of cutscenes in cutscenes/
	if (!infile.open("cutscenes/" + filename, true, false))
		return false;

	// parse the cutscene file
	while (infile.next()) {

		if (infile.new_section) {
			if (infile.section == "scene")
				scenes.push(Scene());
		}

		if (infile.section.empty()) {
			// allow having an empty section (globals such as scale_gfx might be set here
		}
		else if (infile.section == "scene") {
			SceneComponent sc = SceneComponent();

			if (infile.key == "caption") {
				// @ATTR scene.caption|string|A caption that will be shown.
				sc.type = infile.key;
				sc.s = msg->get(infile.val);
			}
			else if (infile.key == "image") {
				// @ATTR scene.image|string|An image that will be shown.
				sc.type = infile.key;
				sc.i = loadImage(infile.val);
				if (sc.i == NULL)
					sc.type = "";
			}
			else if (infile.key == "pause") {
				// @ATTR scene.pause|integer|Pause before next component
				sc.type = infile.key;
				sc.x = toInt(infile.val);
			}
			else if (infile.key == "soundfx") {
				// @ATTR scene.soundfx|string|A sound that will be played
				sc.type = infile.key;
				sc.s = infile.val;
			}

			if (sc.type != "")
				scenes.back().components.push(sc);

		}
		else {
			fprintf(stderr, "unknown section %s in file %s\n", infile.section.c_str(), infile.getFileName().c_str());
		}

		if (infile.key == "scale_gfx") {
			// @ATTR scale_gfx|bool|The graphics will be scaled to fit screen width
			scale_graphics = toBool(infile.val);
		}
	}

	if (scenes.empty()) {
		cerr << "No scenes defined in cutscene file " << filename << endl;
		return false;
	}

	return true;
}

SDL_Surface *GameStateCutscene::loadImage(std::string filename) {

	std::string image_file = (mods->locate("images/"+ filename));
	SDL_Surface *image = IMG_Load(image_file.c_str());
	if (!image) {
		std::cerr << "Missing cutscene art reference: " << image_file << std::endl;
		return NULL;
	}

	/* scale image to fit height */
	if (scale_graphics) {
		float ratio = image->h/(float)image->w;
		SDL_Surface *art = scaleSurface(image, VIEW_W, (int)(VIEW_W*ratio));
		if (art == NULL)
			return image;

		SDL_FreeSurface(image);
		image = art;
	}

	return image;
}

