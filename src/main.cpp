#include "bn_core.h"

#include "bn_bg_tiles.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_sprite_items_cursor.h"
#include "bn_keypad.h"
#include "bn_vector.h"
#include "bn_camera_ptr.h"
#include "bn_timer.h"
#include "bn_memory.h"

#include "bn_regular_bg_tiles_items_cell.h"
#include "bn_bg_palette_items_palette.h"

#define CELL_SIZE 8

class World {

	public:
	static constexpr int world_x = 28;
	static constexpr int world_y = 18;

	private:
	static constexpr int offset_x = 2;
	static constexpr int offset_y = 7;

	static constexpr int bg_cols = 32;
	static constexpr int bg_rows = 32;

	static constexpr int width = 8;
	static constexpr int height = 8;

	alignas(int) bn::regular_bg_map_cell cells[World::bg_cols * World::bg_rows];
	bn::regular_bg_map_item map_item;
	bn::regular_bg_item bg_item;
	bn::regular_bg_ptr bg;	
	bn::regular_bg_map_ptr bg_map;

	public:
	
	World() :
		map_item(cells[0], bn::size(
			World::bg_cols,
			World::bg_rows)),
		bg_item(bn::regular_bg_tiles_items::cell,
			bn::bg_palette_items::palette,
			map_item),
		bg(bg_item.create_bg(0,0)),
		bg_map(bg.map())
	{
		bn::memory::clear(bg_rows * bg_cols, cells[0]);

		for (int i = 0; i < bg_rows; i++) {
		for (int j = 0; j < bg_cols; j++) {
			_set_cell(i,j,0,false,false);
		}}

		set_cell(-1,-1,2);
		set_cell(-1,18,5);
		set_cell_hflip(28,-1,2);
		set_cell_hflip(28,18,5);
		for (int i = 0; i < 28; i++) {
			set_cell(i,-1,3);
			set_cell(i,18,6);
		}
		for (int j = 0; j < 18; j++) {
			set_cell(-1, j, 4);
			set_cell_hflip(28, j, 4);
		}

		update();
	}

	void _set_cell(int x, int y, int id, bool hflip, bool vflip) {
		bn::regular_bg_map_cell& c = cells[map_item.cell_index(x,y)];
		bn::regular_bg_map_cell_info c_info(c);
		c_info.set_tile_index(id);
		c_info.set_palette_id(0);
		c_info.set_horizontal_flip(hflip);
		c_info.set_vertical_flip(vflip);
		c = c_info.cell();
	}

	void set_cell(int x, int y, int id) {
		_set_cell(x + offset_x, y + offset_y, id, false, false);
	}

	void set_cell_hflip(int x, int y, int id) {
		_set_cell(x + offset_x, y + offset_y, id, true, false);
	}

	void set_cell_vflip(int x, int y, int id) {
		_set_cell(x + offset_x, y + offset_y, id, false, true);
	}

	void set_cell_hvflip(int x, int y, int id) {
		_set_cell(x + offset_x, y + offset_y, id, true, true);
	}

	void update() {
		bg_map.reload_cells_ref();
	}
	
};

typedef bn::vector<bn::vector<bool, World::world_y>,World::world_x> State;

class Automaton {
	World world;
	State state;
	int width = World::world_x;
	int height = World::world_y;
	public:
	Automaton() {
		state.resize(width);
		for (int i = 0; i < state.size(); i++)
			state[i].resize(height);
		update();
	}

	void update() {

		bn::vector<bn::vector<int, World::world_y>, World::world_x> neighbors;
		neighbors.resize(width);
		for (int i = 0; i < width; i++) {
		neighbors[i].resize(height);
		for (int j = 0; j < height; j++) {
			neighbors[i][j] = 0;
			for (int ii = i - 1; ii <= i + 1; ii++) {
			for (int jj = j - 1; jj <= j + 1; jj++) {
				if (ii != i || jj != j)
				if (ii >= 0 && jj >= 0 && ii < width && jj < height)
					neighbors[i][j] += state[ii][jj];
			}}
		}}
		
		for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			int n = neighbors[i][j];
			if (state[i][j]) {
				if (n < 2 || n > 3)
					state[i][j] = false;
			} else if (n == 3) {
					state[i][j] = true;
			}
			world.set_cell(i,j,state[i][j]);
		} }
		
		world.update();
	}

	void set_cell(int i, int j, bool s) {
		state[i][j] = s;
		world.set_cell(i,j,state[i][j]);
		world.update();
	}

	bool get_cell(int i, int j) {
		return state[i][j];
	}

	void toggle_cell(int i, int j) {
		set_cell(i, j, !state[i][j]);
	}
};

class Cursor {
	int _x, _y;
	bn::sprite_ptr _sprite;
	bool _visible;
	
	public:
	Cursor(int x, int y) :
		_sprite(bn::sprite_items::cursor.create_sprite(
			CELL_SIZE * -World::world_x/2 + CELL_SIZE/2,
			CELL_SIZE * -World::world_y/2 + CELL_SIZE/2))
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
		if (x < 0 || x >= World::world_x || y < 0 || y >= World::world_y)
			return;
		_x = x;
		_y = y;
		_sprite.set_position(
			CELL_SIZE * -World::world_x/2 + CELL_SIZE/2 + _x * CELL_SIZE,
			CELL_SIZE * -World::world_y/2 + CELL_SIZE/2 + _y * CELL_SIZE);
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


int main() {
	bn::core::init();

	Automaton a;

	Cursor c = Cursor(0, 0);

	bool running = false;
	bn::timer t = bn::timer();


	while(true)
	{
		if (running) {
			if (t.elapsed_ticks() > 100000) {
				a.update();
				t.restart();
			}
			if (bn::keypad::start_pressed()) {
				running = false;
				c.toggle_visible();
			}
		} else {
			if (bn::keypad::up_pressed())
				c.move(0,-1);
			if (bn::keypad::down_pressed())
				c.move(0,1);
			if (bn::keypad::left_pressed())
				c.move(-1,0);
			if (bn::keypad::right_pressed())
				c.move(1,0);

			if (bn::keypad::a_pressed()) {
				int x = c.get_x();
				int y = c.get_y();
				a.toggle_cell(x,y);
			}

			if (bn::keypad::start_pressed()) {
				running = true;
				c.toggle_visible();
			}
			
		}
		bn::core::update();
	}
}
