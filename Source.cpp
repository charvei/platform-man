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

using namespace std;

/*TO DO:
	-Refactor physics
	-Repeated/held down key press is 'interrupted' when another key is pressed
		>Causes issues with running and jumping
	-kicking up dirt effect for better feedback
	-valid move isn't using map its using player which is old implementation stuff*/

struct map_object {
	char icon;
	bool solid;
};

class Level {
	map_object** level_map;
	map_object** construct_map(int height, int width, std::map<string, map_object>* map_objects);
	int height;
	int width;
	std::map<string, map_object>* level_objects;
public:
	Level(int height, int width, std::map<string, map_object>* level_objects);
	map_object** get_map() { return level_map; }
	int get_height() { return height; }
	int get_width() { return width; }
	void move_map_object(int startx, int starty, int endx, int endy);
	void add_map_object(map_object object, int xpos, int ypos);
};

Level::Level(int height, int width, std::map<string, map_object>* level_objects) {
	this->level_objects = level_objects;
	this->level_map = construct_map(height, width, level_objects);		//Do i need to have height & width parameters?
	this->height = height;
	this->width = width;
}

map_object** Level::construct_map(int height, int width, std::map<string, map_object>* level_objects) {
	map_object** map;	//declaration
	map = new map_object*[height];	//allocate space for height number of char*'s (i.e a column).
	for (int i = 0; i < height; i++) {
		map[i] = new map_object[width]; //allocate space for width number of char's (for each row of the column)
	}
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			if (i == (height - 1)) {
				map[i][j] = level_objects->find("land")->second;
			}
			else if ((j % 42 < 4) && (i > height - 6)) {
				map[i][j] = level_objects->find("wall")->second;
			}
			else {
				map[i][j] = level_objects->find("air")->second;
			}
		}
	}
	return map;
}

void Level::add_map_object(map_object object, int xpos, int ypos) {
	level_map[xpos][ypos] = object;
}

void Level::move_map_object(int startx, int starty, int endx, int endy) {
	level_map[endx][endy] = level_map[startx][starty];
	level_map[startx][starty] = level_objects->find("air")->second;
}

class Level_Manager {
	list<Level> levels;
	std::map<string, map_object> level_objects;
public:
	Level_Manager();
	void add_level(Level level);
	Level* get_current_level() { return &(levels.front()); }
	map_object add_level_object(string name, char icon, bool solid);
	std::map<string, map_object>* get_map_objects() { return &level_objects; }
};

Level_Manager::Level_Manager() {
	add_level_object("land", '=', true);
	add_level_object("wall", '|', true);
	add_level_object("air", ' ', false);

	Level level1(23, 83, get_map_objects());
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
	this->level_objects.insert(std::pair<string, map_object>(name, temp_obj));
	return temp_obj;
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
	void run(char direction, double to_velocity);
	void jump();
	void friction(double friction_constant);
	void change_x_velocity(char direction, double to_velocity);
	void change_y_velocity(char direction, double to_velocity);
	void change_move_progress();
	void move_player_map(char direction, int steps);
	void gravity(double gravity_constant);
	double* get_velocity() { return &x_velocity; }
};

Player::Player(int position[2], char icon, Level_Manager* level_manager) {
	this->position = position;
	this->icon = icon;
	this->level_manager = level_manager;

	map_object player = level_manager->add_level_object("player", icon, true);
	level_manager->get_current_level()->add_map_object(player, position[0], position[1]);

	x_velocity = 0;
	y_velocity = 0;
}

void Player::run(char direction, double to_velocity = 1) {
	change_x_velocity(direction, 0.15);
}

void Player::jump() {
	//printw("{%c}", level_manager->get_current_level()->get_map()[position[0]+1][position[1]].icon);
	refresh();
	if (!(valid_move(position[0]+1, position[1]))) {
		//standing on something solid
		change_y_velocity('N', 2);
		refresh();
	}
	//change_y_velocity('N', 2);
}

void Player::change_x_velocity(char direction, double to_velocity) {
	if (direction == 'E') {
		this->x_velocity += to_velocity;
		if (this->x_velocity > 2) {
			this->x_velocity = 2;
		}
	}
	else if (direction == 'W') {
		this->x_velocity -= to_velocity;
		if (this->x_velocity < -2) {
			this->x_velocity = -2;
		}
	}
}

void Player::change_y_velocity(char direction, double to_velocity) {
	if (direction == 'N') {
		this->y_velocity += to_velocity;
		if (this->y_velocity > 2) {
			this->y_velocity = 2;
		}
	}
	else if (direction == 'S') {
		this->y_velocity -= to_velocity;
		if (this->y_velocity < -2) {
			this->y_velocity = -2;
		}
	}
}

void Player::friction(double friction_constant) {
	if (!(valid_move(position[0] + 1, position[1]))) {
		this->x_velocity *= friction_constant;
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
		change_y_velocity('S', 0.08);
	}
	else {
		this->y_velocity = 0;
	}
	this->y_velocity *= gravity_constant;
}

/*There is some weird ass shit going on with x, y names - need to make sure x is not called y etc.
come up with some conventions... for now its working*/
bool Player::valid_move(int ypos, int xpos) {
	Level* level = this->level_manager->get_current_level();
	if ((xpos > level->get_width()-1) || (xpos < 0) || (ypos > level->get_height()-1) || (ypos < 0)) {
		return false;
	}
	else {
		return level->get_map()[ypos][xpos].solid == true ? false : true;
		//refresh();
	}
	return false;
}

void Player::move_player_map(char direction, int steps) {
	switch (direction) {
	case 'N':
		if (valid_move(position[0] - steps, position[1])) {
			level_manager->get_current_level()->move_map_object(position[0], position[1], position[0] - steps, position[1]);
			this->set_position(position[0] - steps, position[1]);
		}
		break;
	case 'S':
		if (valid_move(position[0] + steps, position[1])) {
			level_manager->get_current_level()->move_map_object(position[0], position[1], position[0] + steps, position[1]);
			this->set_position(position[0] + steps, position[1]);
		}
		break;
	case 'E':
		if (valid_move(position[0], position[1] + steps)) {
			level_manager->get_current_level()->move_map_object(position[0], position[1], position[0], position[1] + steps);
			this->set_position(position[0], position[1] + steps);
		}
		break;
	case 'W':
		if (valid_move(position[0], position[1] - steps)) {
			level_manager->get_current_level()->move_map_object(position[0], position[1], position[0], position[1] - steps);
			this->set_position(position[0], position[1] - steps);
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
	
public:
	Screen(int height, int width);
	WINDOW* get_window() { return main_window; }
	void show_player(Player* player);
	void hide_player(Player* player);

	void show_level(Level level);

	int get_height() { return height; }
	int get_width() { return width; }
};

Screen::Screen(int height, int width) {
	this->height = height;
	this->width = width;
	this->starty = (LINES - height) / 2;
	this->startx = (COLS - width) / 2;
	initscr();
	//raw();
	cbreak();
	nodelay(stdscr, TRUE);
	keypad(stdscr, TRUE);
	noecho();
	printw("");
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
	map_object** map = level.get_map();
	for (int i = 0; i < level.get_height(); i++) {
		refresh();
		for (int j = 0; j < level.get_width(); j++) {
			mvwaddch(main_window, i+1, j+1, map[i][j].icon);
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

class Map_Object_Manager {
	//Can't declare vector of the struct for some god damn reason.
	std::map<string, map_object> map_objects;
public:
	Map_Object_Manager();
	void add_map_object(string name, char icon, bool solid);
	std::map<string, map_object>* get_map_objects() { return &map_objects; }
};

Map_Object_Manager::Map_Object_Manager() {
	add_map_object("land", '_', true);
	add_map_object("wall", '|', true);
	add_map_object("air", ' ', false);
}

void Map_Object_Manager::add_map_object(string name, char icon, bool solid) {
	map_object temp_obj = {
		icon,
		solid,
	};
	this->map_objects.insert(std::pair<string, map_object>(name, temp_obj));
}

class Game_Manager {
	Screen* screen;
	Player_Manager* player_manager;
	Level_Manager* level_manager;
	void translate_input(int input);
	void game_loop();
public:
	Game_Manager(Screen* screen, Player_Manager* player_manager, Level_Manager* level_manager, Map_Object_Manager* map_object_manager);
};

Game_Manager::Game_Manager(Screen* screen, Player_Manager* player_manager, Level_Manager* level_manager, Map_Object_Manager* map_object_manager) {
	this->screen = screen;
	this->player_manager = player_manager;
	this->level_manager = level_manager;
	screen->show_level(*(level_manager->get_current_level()));
	screen->show_player(player_manager->get_player());
	game_loop();
}

/*Simple ticking system; perhaps will 'upgrade' later*/
void Game_Manager::game_loop() {
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
			(*player_manager->get_player()).friction(0.88);
			lag -= frame_wait;
		}

		/*RENDERING*/
		screen->show_level(*(level_manager->get_current_level()));
		refresh();
	}
}

void Game_Manager::translate_input(int input) {
	switch (input) {
	case KEY_UP:
		(*player_manager->get_player()).jump();
		break;
	case KEY_DOWN:
		//(*player_manager->get_player()).run('S');
		break;
	case KEY_RIGHT:
		(*player_manager->get_player()).run('E');
		break;
	case KEY_LEFT:
		(*player_manager->get_player()).run('W');
		break;
	}
}

/*MOVE FUNCTIONS FROM PLAYER TO PLAYER_MANAGER???*/
int main() {
	int player1_pos[2] = { 4, 4 };
	Level_Manager level_manager = Level_Manager();
	Player player1 = Player(player1_pos, 'O', &level_manager);
	Screen screen = Screen(25, 85);
	Player_Manager player_manager = Player_Manager(&player1);
	Map_Object_Manager map_object_manager = Map_Object_Manager();
	Game_Manager game_manager = Game_Manager(&screen, &player_manager, &level_manager, &map_object_manager);
	endwin();
}