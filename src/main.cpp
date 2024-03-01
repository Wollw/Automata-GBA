#include "bn_core.h"

#include "bn_sprite_ptr.h"
#include "bn_sprite_actions.h"
#include "bn_sprite_items_cell.h"
#include "bn_sprite_items_cursor.h"
#include "bn_keypad.h"
#include "bn_vector.h"
#include "bn_camera_ptr.h"
#include "bn_timer.h"

#define CELL_SIZE 8

#define WORLD_X 10
#define WORLD_Y 10

#define CAMERA_X 60
#define CAMERA_Y 30

class Cell {

	bn::sprite_ptr _sprite;
	bool _state;

	public:
	Cell(bn::sprite_ptr&& sprite, bool state) : _sprite(bn::move(sprite)), _state(state)
	{
		_sprite.set_visible(_state);
	};
	bn::sprite_ptr get_sprite() { return _sprite; };
	bool get_state() { return _state; };
	void set_state(bool s) {
		_state = s;
		_sprite.set_visible(s);
	};
	
	void toggle_state() {
		set_state(!_state);
	}
};

typedef bn::vector<bn::vector<Cell, WORLD_Y>, WORLD_X> World;

class Cursor {
	int _x, _y;
	bn::sprite_ptr _sprite;
	bool _visible;
	
	public:
	Cursor(bn::sprite_ptr &&sprite, int x, int y) :  _sprite(bn::move(sprite))
	{
		_visible = true;
		_x = x;
		_y = y;
	};

	void set_pos(int x, int y) {
		_x = x;
		_y = y;
		_sprite.set_position(_x, _y);
	};

	bn::sprite_ptr get_sprite() {
		return _sprite;
	};

	void move(int x, int y) {
		x += _x;
		y += _y;
		if (x < 0 || x >= WORLD_X || y < 0 || y >= WORLD_Y)
			return;
		_x = x;
		_y = y;
		_sprite.set_position(
			_x * CELL_SIZE,
			_y * CELL_SIZE);
	};

	void toggle_visible() {
		_visible = !_visible;
		_sprite.set_visible(_visible);
	}

	int get_x() {
		return _x;
	};

	int get_y() {
		return _y;
	};
};

void update_world(World &w) {
	// Get number of living neighbors
	bn::vector<bn::vector<int, WORLD_Y>, WORLD_X> neighbors;
	for (int i = 0; i < WORLD_X; i++) {
		neighbors.resize(WORLD_X);
		for (int j = 0; j < WORLD_Y; j++) {
			int n = 0;
			for (int ii = i - 1; ii <= i + 1; ii++) {
			for (int jj = j - 1; jj <= j + 1; jj++) {
				if (ii != i || jj != j)
				if (ii >= 0 && jj >= 0 && ii < WORLD_X && jj < WORLD_Y)
					n += w[ii][jj].get_state() ? 1 : 0;
			}}
			neighbors[i].push_back(n);
	}}


	for (int i = 0; i < WORLD_X; i++) {
	for (int j = 0; j < WORLD_Y; j++) {
		int n = neighbors[i][j];
		// Apply Conway
		if (w[i][j].get_state()) {
			if (n < 2 || n > 3)
				w[i][j].set_state(false);
		} else if (n == 3) {
				w[i][j].set_state(true);
		}
	}}
}



int main() {
	bn::core::init();
	bn::camera_ptr camera = bn::camera_ptr::create(CAMERA_X,CAMERA_Y);

	World world;
	for (int i = 0; i < WORLD_X; i++) {
		world.resize(WORLD_X);
		for (int j = 0; j < WORLD_Y; j++) {
			Cell c = Cell(bn::sprite_items::cell.create_sprite(
					CELL_SIZE * i,
					CELL_SIZE * j),
				false);
			c.get_sprite().set_camera(camera);
			world[i].push_back(c);
	}}

	Cursor c = Cursor(bn::sprite_items::cursor.create_sprite(0,0), 0, 0);
	c.get_sprite().set_camera(camera);

	bn::timer t = bn::timer();

	bool running = false;

	while(true)
	{

		if (bn::keypad::b_pressed()) {
			running = !running;
			c.toggle_visible();
		}

		if (!running) {
			if (bn::keypad::down_pressed())
				c.move(0,1);
			if (bn::keypad::up_pressed())
				c.move(0,-1);
			if (bn::keypad::left_pressed())
				c.move(-1,0);
			if (bn::keypad::right_pressed())
				c.move(1,0);
			if (bn::keypad::a_pressed()) {
				int x = c.get_x();
				int y = c.get_y();
				world[x][y].toggle_state();
			}
		} else if (running && t.elapsed_ticks() > 100000) {
			update_world(world);
			t.restart();
		}

		bn::core::update();
	}
}
