#include "bn_core.h"

#include "bn_bg_tiles.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_actions.h"
#include "bn_sprite_items_cursor.h"
#include "bn_keypad.h"
#include "bn_vector.h"
#include "bn_camera_ptr.h"
#include "bn_timer.h"
#include "bn_memory.h"
#include "bn_sram.h"

#include "bn_regular_bg_tiles_items_cell.h"
#include "bn_bg_palette_items_palette.h"

#define CELL_SIZE 8

struct Rule {
	char live[8];
	char dead[8];
};
typedef struct Rule Rule;

class Menu {

	private:
	static constexpr int offset_x = 2;
	static constexpr int offset_y = 7;

	static constexpr int bg_cols = 32;
	static constexpr int bg_rows = 32;

	static constexpr int width = 8;
	static constexpr int height = 8;

	Rule *rule;

	alignas(int) bn::regular_bg_map_cell cells[Menu::bg_cols * Menu::bg_rows];
	bn::regular_bg_map_item map_item;
	bn::regular_bg_item bg_item;
	bn::regular_bg_ptr bg;	
	bn::regular_bg_map_ptr bg_map;

	public:
	
	Menu(Rule *r) :
		rule(r),
		map_item(cells[0], bn::size(
			Menu::bg_cols,
			Menu::bg_rows)),
		bg_item(bn::regular_bg_tiles_items::cell,
			bn::bg_palette_items::palette,
			map_item),
		bg(bg_item.create_bg(0,0)),
		bg_map(bg.map())
	{
		bn::memory::clear(bg_rows * bg_cols, cells[0]);

		set_cell(-1,-1,2);
		set_cell(-1,0,4);
		set_cell(-1,1,4);
		set_cell(-1,2,5);
		for (int i = 0; i < 8; i++) {
			set_cell(i,-1,3);
			set_cell(i,0, rule->dead[i]);
			set_cell(i,1, rule->live[i]);
			set_cell(i,2, 6);
		}
		set_cell_hflip(8,-1,2);
		set_cell_hflip(8,0,4);
		set_cell_hflip(8,1,4);
		set_cell_hflip(8,2,5);

		update();
	}

	void redraw_rules() {
		for (int i = 0; i < 8; i++) {
			set_cell(i,0, rule->dead[i]);
			set_cell(i,1, rule->live[i]);
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

	void toggle_rule(int i, int j) {
		if (j == 0) {
			rule->dead[i] = !rule->dead[i];
		} else {
			rule->live[i] = !rule->live[i];
		}
		redraw_rules();
	}

	void update() {
		bg_map.reload_cells_ref();
	}

	void toggle_visible() {
		bool v = bn::regular_bg_visible_manager::get(bg);
		bn::regular_bg_visible_manager::set(!v, bg);
	}

};

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
	Rule *rule;
	public:
	Automaton(Rule *r) : rule(r) {
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
					state[i][j] = rule->live[n];
			} else {
					state[i][j] = rule->dead[n];
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

	void save() {
		for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			bn::sram::write_offset((state[i][j]), 512 + j * (width + 1) + i);
		}}
	}

	void load() {
		for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			bool s;
			bn::sram::read_offset(s, 512 + j * (width + 1) + i);
			set_cell(i,j,s);
		}}
		
	}

};

class Cursor {
	int _x, _y;
	bn::sprite_ptr _sprite;
	bool _visible;

	int _min_x = 0;
	int _max_x = World::world_x - 1;
	int _min_y = 0;
	int _max_y = World::world_y - 1;
	
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

	void set_constraints(int min_x, int max_x, int min_y, int max_y) {
		_min_x = min_x;
		_max_x = max_x;
		_min_y = min_y;
		_max_y = max_y;
	}

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
		if (x < _min_x || x > _max_x || y < _min_y || y > _max_y)
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

	void save() {
		bn::sram::write_offset(_x, 128);
		bn::sram::write_offset(_y, 256);
	}

	void load() {
		bn::sram::read_offset(_x, 128);
		bn::sram::read_offset(_y, 256);
		_sprite.set_position(
			CELL_SIZE * -World::world_x/2 + CELL_SIZE/2 + _x * CELL_SIZE,
			CELL_SIZE * -World::world_y/2 + CELL_SIZE/2 + _y * CELL_SIZE);
	}
};

int main() {
	bn::core::init();

	Rule r = Rule
		{ .live = {0,0,1,1,0,0,0,0}
		, .dead = {0,0,0,1,0,0,0,0}};

	Automaton a(&r);
	Menu menu(&r);
	menu.toggle_visible();

	Cursor c = Cursor(0, 0);
	Cursor menu_c = Cursor(0, 0);
	menu_c.toggle_visible();
	menu_c.set_constraints(0,7,0,1);

	bool running = false;
	bool menuing = false;

	bn::timer t = bn::timer();

	while(true)
	{
		if (bn::keypad::l_pressed()) {
			a.load();
			c.load();
			bn::sram::read(r);
			menu.redraw_rules();
		}
		if (bn::keypad::r_pressed()) {
			c.save();
			a.save();	
			bn::sram::write(r);
		}
			
		if (menuing) {

			if (bn::keypad::up_pressed())
				menu_c.move(0,-1);
			if (bn::keypad::down_pressed())
				menu_c.move(0,1);
			if (bn::keypad::left_pressed())
				menu_c.move(-1,0);
			if (bn::keypad::right_pressed())
				menu_c.move(1,0);

			if (bn::keypad::a_pressed()) {
				int x = menu_c.get_x();
				int y = menu_c.get_y();
				menu.toggle_rule(x,y);
			}


			if (bn::keypad::b_pressed() || bn::keypad::select_pressed()) {
				menuing = false;
				menu.toggle_visible();
				c.toggle_visible();
				menu_c.toggle_visible();
			}
		} else if (running) {
			if (t.elapsed_ticks() > 50000) {
				a.update();
				t.restart();
			}
			if (bn::keypad::start_pressed()) {
				running = false;
				c.toggle_visible();
			}
			if (bn::keypad::select_pressed()) {
				running = false;
				c.toggle_visible();
				menuing = true;
				menu.toggle_visible();
				c.toggle_visible();
				menu_c.toggle_visible();
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

			if (bn::keypad::select_pressed()) {
				menuing = true;
				menu.toggle_visible();
				c.toggle_visible();
				menu_c.toggle_visible();
			}
			
		}
		bn::core::update();
	}
}
