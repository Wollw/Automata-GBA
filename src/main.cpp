#include "bn_core.h"

#include "bn_sprite_ptr.h"
#include "bn_sprite_actions.h"
#include "bn_sprite_items_cell.h"
#include "bn_keypad.h"
#include "bn_vector.h"
#include "bn_camera_ptr.h"

#include <vector>

#define CELL_SIZE 8

#define WORLD_X 16
#define WORLD_Y 8

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
};

typedef bn::vector<bn::vector<Cell, WORLD_Y>, WORLD_X> World;

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

	// Blinker
	//world[1][0].set_state(true);
	//world[1][1].set_state(true);
	//world[1][2].set_state(true);

	// Toad
	//world[3][4].set_state(true);	
	//world[4][4].set_state(true);	
	//world[5][4].set_state(true);	
	//world[4][5].set_state(true);	
	//world[5][5].set_state(true);	
	//world[6][5].set_state(true);	

	// Beacon
	//world[8][0].set_state(true);
	//world[8][1].set_state(true);
	//world[9][0].set_state(true);
	//world[10][3].set_state(true);
	//world[11][3].set_state(true);
	//world[11][2].set_state(true);

	// Glider
	world[0][2].set_state(true);
	world[1][0].set_state(true);
	world[1][2].set_state(true);
	world[2][2].set_state(true);
	world[2][1].set_state(true);

	while(true)
	{
		if (bn::keypad::a_pressed())
			update_world(world);
		bn::core::update();
	}
}
