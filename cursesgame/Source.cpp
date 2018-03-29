#include <curses.h>
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <map>
#include <winapifamily.h>
#include <chrono>
#include <ctime>
#include <list>
#include <cmath>
#include <queue>
#include <iostream>
#include <fstream>

using namespace std;

/*TO DO:
	-Refactor physics
	-Repeated/held down key press is 'interrupted' when another key is pressed
		>Causes issues with running and jumping
	-hold UP to 'glide' (reduce air friction) + graphics to represent this
	-kicking up dirt effect for better feedback
	-visual overhaul
	-lives
	-enemies (involves extracting some elements from player and generalising)
	-intro logo 'sierra screen'.
	-music
	-menus
	-controls feel improvements
	IN PROGRESS
	-level editor (text file)
		>fix bugs with reading + outputting levels
			>filling space with air if level is smaller than screen

	*/

struct map_object {
	char icon;
	bool solid;
	//bool harmful_direction[4];	//N, S, E, W
	vector<bool> harmful_dir;
	//bool harmful_n, harmful_s, harmful_e, harmful_w;
};



class Level {
	//map_object** level_map;
	vector<vector<map_object*>> map;
	vector<vector<map_object*>> construct_map(int height, int width, std::map<string, map_object>* map_objects);
	int height;
	int width;
	std::map<string, map_object>* level_objects;
public:
	Level(int height, int width, std::map<string, map_object>* level_objects);
	Level(vector<vector<map_object*>> map, std::map<string, map_object>* level_objects);
	vector<vector<map_object*>>* get_map() { return &map; }
	int get_height() { return height; }
	int get_width() { return width; }
	void move_map_object(int startx, int starty, int endx, int endy);
	void add_map_object(map_object* object, int xpos, int ypos);
};

Level::Level(int height, int width, std::map<string, map_object>* level_objects) {
	this->level_objects = level_objects;
	this->map = construct_map(height, width, level_objects);
	this->height = height;
	this->width = width;
}

Level::Level(vector<vector<map_object*>> map, std::map<string, map_object>* level_objects) {
	this->map = map;
	this->level_objects = level_objects;
	this->height = map.size();
	this->width = map.at(0).size();
}

vector<vector<map_object*>> Level::construct_map(int height, int width, std::map<string, map_object>* level_objects) {
	vector<vector<map_object*>> map;
	for (int i = 0; i < height; i++) {
		vector<map_object*> temp_row;
		for (int j = 0; j < width; j++) {
			if (i == (height - 1)) {
				temp_row.push_back(&(level_objects->find("land")->second));
			}
			else if ((j % 42 < 4) && (i > height - 6)) {
				temp_row.push_back(&(level_objects->find("wall")->second));
			}
			else {
				temp_row.push_back(&(level_objects->find("air")->second));
			}
		}
		map.push_back(temp_row);
	}
	return map;
}

void Level::add_map_object(map_object* object, int xpos, int ypos) {
	map.at(xpos).at(ypos) = object;
}

void Level::move_map_object(int startx, int starty, int endx, int endy) {
	map.at(endx).at(endy) = map.at(startx).at(starty);
	map.at(startx).at(starty) = &(level_objects->find("air")->second);
}

class Level_Manager {
	list<Level> levels;
	int screen_height;
	std::map<string, map_object> map_objects;
public:
	Level_Manager(int screen_height);
	void add_level(Level level);
	Level* get_current_level() { return &(levels.front()); }
	map_object add_level_object(string name, char icon, bool solid);
	map_object add_level_object(string name, char icon, bool solid, bool harmful_n, bool harmful_s, bool harmful_e, bool harmful_w);
	std::map<string, map_object>* get_map_objects() { return &map_objects; }
	vector<vector<map_object*>> read_level_file(string file, int screen_height);
};

Level_Manager::Level_Manager(int screen_height) {
	this->screen_height = screen_height;
	add_level_object("land", '=', true);
	add_level_object("land2", '-', true);
	add_level_object("wall", '|', true);
	add_level_object("air", ' ', false);
	add_level_object("spike-up", '^', true, true, false, false, false);
	add_level_object("spike-down", 'v', true);

	//Level level1(23, 500, get_map_objects());		//Generated placeholder level
	Level level1(read_level_file("level_1.txt", screen_height), &map_objects);

	add_level(level1);
}

void Level_Manager::add_level(Level level) {
	levels.push_back(level);
}

map_object Level_Manager::add_level_object(string name, char icon, bool solid) {
	map_object temp_obj = {
		icon,
		solid,
	};
	this->map_objects.insert(std::pair<string, map_object>(name, temp_obj));
	return temp_obj;
}

map_object Level_Manager::add_level_object(string name, char icon, bool solid, bool harmful_n, bool harmful_s, bool harmful_e, bool harmful_w) {
	map_object temp_obj = {
		icon,
		solid,
		//harmful_n,
		//harmful_s,
		//harmful_e,
		//harmful_w,
	};
	this->map_objects.insert(std::pair<string, map_object>(name, temp_obj));
	return temp_obj;
}

vector<vector<map_object*>> Level_Manager::read_level_file(string file, int screen_height) {
	vector<vector<map_object*>> map;
	std::map<string, map_object> *level_objects = (get_map_objects());
	std::map<string, map_object>::iterator it;
	string line;
	bool map_start_flag = false;
	int width;
	try {
		ifstream level_file("../levels/" + file, ios::in);
		if (!level_file.is_open()) {
			throw 1;
		}
		while (getline(level_file, line)) {
			//cout << line + '\n';
			if (!map_start_flag && (line.length() > 0)) {
				//Check for things
				if (line.at(0) == '$') {
					map_start_flag = true;
					width = line.length();
				}
			}
			else {
				//Read map
				vector<map_object*> temp_vector;
				if ((line.length() > 0) && line.at(0) == '$') {
					map_start_flag = false;
					return map;
				}
				else if (line.length() > 0) {
					for (int i = 0; i < line.length(); i++) {
						for (it = level_objects->begin(); it != level_objects->end(); it++) {
							if (it->second.icon == line.at(i)) {
								temp_vector.push_back(&it->second);
							}
						}
					}
					if (line.length() < width) {
						for (int i = 0; i < width - line.length(); i++) {
							temp_vector.push_back(&(level_objects->find("air")->second));
						}
					}
				}
				else {
					for (int i = 0; i < width; i++) {
						temp_vector.push_back(&(level_objects->find("air")->second));
					}
				}
				map.push_back(temp_vector);
			}
		}
		//Map should be populated now - check to see if smaller than screen, if so add some "air" at top of map
		if (map.size() < screen_height) {
			for (int i = 0; i < (screen_height - map.size()); i++) {
				vector<map_object*> temp_vector;
				for (int j = 0; j < width; j++) {
					temp_vector.push_back(&level_objects->find("air")->second);
				}
				map.insert(map.begin(), temp_vector);
				//map.push_back(temp_vector);
			}
		}
	}
	catch (int err) {
		printw("ERROR READING FILE");
		return map;
	}
	return map;
	//REMEMBER TO CLOSE LEVEL FILE
}

/*A lot of these functions would fit well in an entity manager of sorts*/
class Player {
	char icon;
	int* position;
	Level_Manager* level_manager;
	//velocity player_velocity;
	double x_velocity;
	double x_move_progress;
	double y_velocity;
	double y_move_progress;
public:
	Player(int position[2], char icon, Level_Manager* level_manager);
	int* get_position() { return position; }
	void set_position(int xpos, int ypos) { this->position[0] = xpos; this->position[1] = ypos; }
	void move_player();
	char get_icon() { return icon; }
	bool valid_move(int xpos, int ypos);
	bool valid_move(int y_dest, int x_dest, int x_diff, int y_diff);
	void run(char direction, double to_velocity);
	void jump();
	void friction(double friction_constant);
	void change_x_velocity(char direction, double to_velocity);
	void change_y_velocity(char direction, double to_velocity);
	void change_move_progress();
	void move_player_map(char direction, int steps);
	void gravity(double gravity_constant);
	double* get_x_velocity() { return &x_velocity; }
};

Player::Player(int position[2], char icon, Level_Manager* level_manager) {
	this->position = position;
	this->icon = icon;
	this->level_manager = level_manager;

	map_object player = level_manager->add_level_object("player", icon, true);
	level_manager->get_current_level()->add_map_object(&player, position[0], position[1]);

	x_velocity = 0;
	y_velocity = 0;
}

void Player::run(char direction, double to_velocity = 1) {
	change_x_velocity(direction, 0.25);
}

void Player::jump() {
	//printw("{%c}", level_manager->get_current_level()->get_map()[position[0]+1][position[1]].icon);
	refresh();
	if (!(valid_move(position[0]+1, position[1])) && (y_velocity < 1)) {
		//standing on something solid
		change_y_velocity('N', 2);
	}
	//change_y_velocity('N', 2);
}

void Player::change_x_velocity(char direction, double to_velocity) {
	if (direction == 'E') {
		this->x_velocity += to_velocity;
		if (this->x_velocity > 1.5) {
			this->x_velocity = 1.5;
		}
	}
	else if (direction == 'W') {
		this->x_velocity -= to_velocity;
		if (this->x_velocity < -1.5) {
			this->x_velocity = -1.5;
		}
	}
}

void Player::change_y_velocity(char direction, double to_velocity) {
	if (direction == 'N') {
		this->y_velocity += to_velocity;
		if (this->y_velocity > 1.5) {
			this->y_velocity = 1.5;
		}
	}
	else if (direction == 'S') {
		this->y_velocity -= to_velocity;
		if (this->y_velocity < -1.5) {
			this->y_velocity = -1.5;
		}
	}
}

void Player::friction(double friction_constant) {
	if (!(valid_move(position[0] + 1, position[1]))) {
		this->x_velocity *= friction_constant;
	}
	else {
		//Different level of friction when in the air
		this->x_velocity *= (friction_constant+((1 - friction_constant)/1.5));
	}
}

/*whenever move progress clocks over an integer*/
void Player::change_move_progress() {
	this->x_move_progress += x_velocity;
	this->y_move_progress += y_velocity;
}


void Player::gravity(double gravity_constant) {
	if (valid_move(position[0] - 1, position[1])) {	//don't like this check here
		//there is a non-solid object below player
		change_y_velocity('S', 0.06);
	}
	else {
		this->y_velocity = 0;
	}
	this->y_velocity *= gravity_constant;
}

/*There is some weird stuff going on with x, y names - need to make sure x is not called y etc.
come up with some conventions... for now its working*/
bool Player::valid_move(int ypos, int xpos) {
	Level* level = this->level_manager->get_current_level();
	vector<vector<map_object*>> map = *(level->get_map());
	if ((xpos > level->get_width()-1) || (xpos < 0) || (ypos > level->get_height()-1) || (ypos < 0)) {
		return false;
	}
	else {
		return (*map.at(ypos).at(xpos)).solid == true ? false : true;
		//refresh();
	}
	return false;
}

bool Player::valid_move(int y_dest, int x_dest, int x_diff, int y_diff) {
	Level* level = this->level_manager->get_current_level();
	vector<vector<map_object*>> map = *(level->get_map());
	for (int i = 0; i <= x_diff; i++) {
		for (int j = 0; j <= y_diff; j++) {
			if ((x_dest+i > level->get_width()-1) || (x_dest+i < 0) || (y_dest+j > level->get_height()-1) || (y_dest+j < 0)) {
				return false;
			}
			else {
				if ((*map.at(y_dest + j).at(x_dest + i)).solid == true) {
					return false;
				}
				//return level->get_map().at(y_dest+j).at(x_dest+i).solid == true ? false : true;
			}
		}
	}
	return true;
}

void Player::move_player_map(char direction, int steps) {
	int valid_at = 0;
	switch (direction) {
	case 'N':
		for (int i = 1; i <= steps; i++) {
			if (!valid_move(position[0] - i, position[1])) {
				break;
			}
			else {
				valid_at = i;
			}
		}
		if (valid_at) {
			level_manager->get_current_level()->move_map_object(position[0], position[1], position[0] - valid_at, position[1]);
			this->set_position(position[0] - valid_at, position[1]);
		}
		break;
	case 'S':
		for (int i = 1; i <= steps; i++) {
			if (!valid_move(position[0] + i, position[1])) {
				break;
			}
			else {
				valid_at = i;
			}
		}
		if (valid_at) {
			level_manager->get_current_level()->move_map_object(position[0], position[1], position[0] + valid_at, position[1]);
			this->set_position(position[0] + valid_at, position[1]);
		}
		break;
	case 'E':
		for (int i = 1; i <= steps; i++) {
			if (!(valid_move(position[0], position[1]+i))) {
				break;
			}
			else {
				valid_at = i;
			}
		}
		if (valid_at) {
			level_manager->get_current_level()->move_map_object(position[0], position[1], position[0], position[1] + valid_at);
			this->set_position(position[0], position[1] + valid_at);
		}
		else {
			//Hit a solid object when trying to go EAST -> set velocity and move_progress to 0
			this->x_velocity = 0;
			this->x_move_progress = 0;
		}
		break;
	case 'W':
		for (int i = 1; i <= steps; i++) {
			if (!(valid_move(position[0], position[1] - i))) {
				break;
			}
			else {
				valid_at = i;
			}
		}
		if (valid_at) {
			level_manager->get_current_level()->move_map_object(position[0], position[1], position[0], position[1] - valid_at);
			this->set_position(position[0], position[1] - valid_at);
		}
		else {
			//Hit a solid object when trying to go EAST -> set velocity and move_progress to 0
			this->x_velocity = 0;
			this->x_move_progress = 0;
		}
		break;
	}
}

void Player::move_player() {
	if (x_move_progress > 1) {
		move_player_map('E', (std::abs((int)x_move_progress)));
		this->x_move_progress -= (int)x_move_progress;
	}
	else if (x_move_progress < -1) {
		move_player_map('W', (std::abs((int)x_move_progress)));
		this->x_move_progress -= (int)x_move_progress;
	}
	if (y_move_progress > 1) {
		move_player_map('N', (std::abs((int)y_move_progress)));
		this->y_move_progress -= (int)y_move_progress;
	}
	else if (y_move_progress < -1) {
		move_player_map('S', (std::abs((int)y_move_progress)));
		this->y_move_progress -= (int)y_move_progress;
	}
	
}

class Screen {
	int height, width, starty, startx;
	WINDOW* main_window;
	WINDOW* create_new_window(int height, int width, int starty, int startx);
	int cam_x_pos = 0;
	int player_screen_pos = 0;
public:
	Screen(int height, int width);
	WINDOW* get_window() { return main_window; }
	void show_player(Player* player);
	void hide_player(Player* player);

	int* get_cam_x_pos() { return &cam_x_pos; }
	int* get_player_screen_pos() { return &player_screen_pos; }

	void show_level(Level level);
	void scroll_level(Level* level, Player* player, int* cam_x_pos, int* player_screen_pos);

	int get_height() { return height; }
	int get_width() { return width; }
};

Screen::Screen(int height, int width) {
	this->height = height;
	this->width = width;
	this->starty = (LINES - height) / 2;
	this->startx = (COLS - width) / 2;
	initscr();
	cbreak();
	nodelay(stdscr, TRUE);
	keypad(stdscr, TRUE);
	noecho();
	refresh();
	this->main_window = create_new_window(this->height, this->width, this->starty, this->startx);
}

WINDOW* Screen::create_new_window(int height, int width, int starty, int startx) {
	WINDOW* local_win;
	local_win = newwin(height, width, 2, 2);
	box(local_win, 0, 0);
	wprintw(local_win, "main_window");
	wrefresh(local_win);
	return local_win;
}

void Screen::show_player(Player* player) {
	mvwaddch(main_window, (*player).get_position()[0], (*player).get_position()[1], (*player).get_icon());
	wrefresh(main_window);
}

void Screen::hide_player(Player* player) {
	mvwaddch(main_window, (*player).get_position()[0], (*player).get_position()[1], ' ');
	wrefresh(main_window);
}

void Screen::show_level(Level level) {
	//map_object** map = level.get_map();
	vector<vector<map_object*>> map = *(level.get_map());
	//vector<vector<map_object>> map = level.get_map();

	for (int i = 0; i < map.size(); i++) {
		for (int j = 0; j < this->width-2; j++) {
			//printw("{%d=screen, %d=level}", this->height, map.size());
			mvwaddch(main_window, i+1, j+1, (*map.at(i).at(j)).icon);
		}
	}
	wrefresh(main_window);
}

/*Approach: camera is at a good starting point but will be good to make a bit responsive
	>e.g. more willing to move to follow player when at max velocity but less willing during
	small movements (more mario like!)*/
void Screen::scroll_level(Level* level, Player* player, int* cam_x_pos, int* player_screen_pos) {
	vector<vector<map_object*>> map = *(level->get_map());
	int start_y = 0;
	*player_screen_pos = player->get_position()[1] - *cam_x_pos;

	if (player->get_position()[1] < ((this->width / 2) - 4)) {
		/*Player is at start of level*/
		//printw("a");
		*cam_x_pos = 0;
	}
	else if (player->get_position()[1] > (level->get_width() + 4 - ((this->width) - this->width / 2))) {
		/*Player is at end of level*/
		*cam_x_pos = (level->get_width() - this->width + 2);
	}
	else if (*player_screen_pos > ((this->width / 2) + 4)) {
		//Player is a bit in the right side of screen
		if ((*player->get_x_velocity() > 0)) {
			*cam_x_pos = player->get_position()[1] - ((this->width / 2) + 4);
		}
	} 
	else if (*player_screen_pos < ((this->width / 2) - 4)) {
		if ((*player->get_x_velocity() < 0)) {
			*cam_x_pos = player->get_position()[1] - ((this->width / 2) - 4);
		}
	}
	//map_object** map = level->get_map();
	//vector<vector<map_object>> map = level->get_map();
	for (int i = 0; i < map.size(); i++) {
		for (int j = 0; j < this->width - 2; j++) {
			mvwaddch(main_window, i + 1, j + 1, (*map.at(i).at(j+*cam_x_pos)).icon);
		}
	}
	wrefresh(main_window);
}

class Player_Manager {
	Player* player;
	
public:
	Player_Manager(Player* player);
	Player* get_player() { return player; }
};

Player_Manager::Player_Manager(Player* player) {
	this->player = player;
}

class Game_Manager {
	Screen* screen;
	Player_Manager* player_manager;
	Level_Manager* level_manager;
	void translate_input(int input);
	void game_loop();
public:
	Game_Manager(Screen* screen, Player_Manager* player_manager, Level_Manager* level_manager);
};

Game_Manager::Game_Manager(Screen* screen, Player_Manager* player_manager, Level_Manager* level_manager) {
	this->screen = screen;
	this->player_manager = player_manager;
	this->level_manager = level_manager;
	screen->show_level(*(level_manager->get_current_level()));
	screen->show_player(player_manager->get_player());
	game_loop();
}

/*Simple ticking system; perhaps will 'upgrade' later*/
void Game_Manager::game_loop() {
	//screen->show_level(*(level_manager->get_current_level()));
	auto previous = std::chrono::system_clock::now();
	auto previous_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(previous);
	double elapsed;
	double lag = 0;
	int input;
	double frame_wait = 32;
	queue<int> pressed_keys;
	while (1) {
		auto current = std::chrono::system_clock::now();
		auto current_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(current);
		elapsed = std::chrono::milliseconds(current_ms - previous_ms).count();
		previous_ms = current_ms;
		lag += elapsed;
		int i = 0;

		/*PROCESSING*/
		while ((input = getch()) != ERR) {
			pressed_keys.push(input);
		}
		while (!pressed_keys.empty()) {
			translate_input(pressed_keys.front());
			pressed_keys.pop();
		}
		

		/*UPDATING GAMESTATE*/
		while (lag >= frame_wait) {
			(*player_manager->get_player()).change_move_progress();
			(*player_manager->get_player()).move_player();
			(*player_manager->get_player()).gravity(0.9);
			(*player_manager->get_player()).friction(0.8);
			lag -= frame_wait;
		}

		/*RENDERING*/
		
		screen->scroll_level(level_manager->get_current_level(), player_manager->get_player(), screen->get_cam_x_pos(), screen->get_player_screen_pos());
		refresh();
	}
}

void Game_Manager::translate_input(int input) {
	if (GetAsyncKeyState(' ') & (1 << 15)) {
		(*player_manager->get_player()).jump();
	}
	if ((GetAsyncKeyState(VK_LEFT) & (1 << 15))) {
		(*player_manager->get_player()).run('W');
	}
	if ((GetAsyncKeyState(VK_RIGHT) & (1 << 15))) {
		(*player_manager->get_player()).run('E');
	}
}

/*MOVE FUNCTIONS FROM PLAYER TO PLAYER_MANAGER???*/
int main() {
	int player1_pos[2] = { 9, 1 };
	int screen_height = 25;
	int screen_width = 85;
	Level_Manager level_manager = Level_Manager(screen_height);
	Player player1 = Player(player1_pos, 'O', &level_manager);
	Screen screen = Screen(screen_height, screen_width);
	Player_Manager player_manager = Player_Manager(&player1);
	Game_Manager game_manager = Game_Manager(&screen, &player_manager, &level_manager);
	endwin();
}