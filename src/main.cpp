#include "bn_core.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_actions.h"
#include "bn_vector.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_timer.h"
#include "bn_string.h"
#include "bn_sram.h"

#include "bn_log.h"

#include "bn_sprite_items_item.h"
#include "bn_sprite_items_cursor.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_bg_palette_items_palette.h"
#include "bn_regular_bg_tiles_items_cell.h"
#define CELL_SIZE 8

struct Rule {
    char live[9];
    char dead[9];
};
typedef struct Rule Rule;

#define SRAM_WORLD_OFFSET   1024
#define SRAM_RULE_OFFSET    0

Rule _rule = Rule
    { .live = {0,0,1,1,0,0,0,0,0}
    , .dead = {0,0,0,1,0,0,0,0,0}};

Rule _life = _rule;

class Item {
    int _x, _y;

    bn::regular_bg_map_cell *_tile = NULL;
    bn::sprite_ptr *_sprite = NULL;

    Item *_item_up = NULL;
    Item *_item_down = NULL;
    Item *_item_left = NULL;
    Item *_item_right = NULL;

    int _state = 0;
    int _tile_id = 0;

    char _id[8];

    static constexpr int _state_tile_map[] = {1,2,3,4,5,6,7,8};

    public:
    Item(int x = 0, int y = 0) {
        _x = x;
        _y = y;
    }

    void set_id(char *s) {
        char *id = _id;
        while (*s) {
            *id++ = *s++;
        }
    }

    void set_id(bn::string<8> s) {
        for (int i = 0; i < s.size(); i++)
            _id[i] = s.at(i);
    }

    char *get_id() {
        return _id;
    }

    Item(int x, int y, bn::sprite_ptr *s)
    {
        _sprite = s;
        _x = x;
        _y = y;
    }
    
    Item(int x, int y, bn::regular_bg_map_cell *t)
    {
        _tile = t;
        _x = x;
        _y = y;
    }

    int get_state() {
        return _state;
    }

    void set_state(int s) {
        _state = s;
        if (_tile) {
            bn::regular_bg_map_cell_info c_info(*_tile);
            c_info.set_tile_index(_state_tile_map[_state]);
            *_tile = c_info.cell();
        }
    }

    int get_x() {
        return _x; 
    }

    int get_y() {
        return _y;
    }

    void set_up(Item *i) {
        _item_up = i; 
    }
    void set_down(Item *i) {
        _item_down = i; 
    }
    void set_left(Item *i) {
        _item_left = i; 
    }
    void set_right(Item *i) {
        _item_right = i; 
    }

    typedef void (*F)(Item*, void *p);
    void on_a(F f, void *p) {
        f(this, p);
    }

    Item *get_up()    { return _item_up; }
    Item *get_down()  { return _item_down; }
    Item *get_left()  { return _item_left; }
    Item *get_right() { return _item_right; }
};


class Cursor {

    Item *_current_item;
    bn::sprite_ptr _cursor_sprite;

    int _offset_x = 0;
    int _offset_y = 0;

    public:
    Cursor(int window_width, int window_height, Item *i) :
        _cursor_sprite(bn::sprite_items::cursor.create_sprite(0, 0))
    {
        _offset_x = -CELL_SIZE*window_width/2+CELL_SIZE/2;
        _offset_y = -CELL_SIZE*window_height/2+CELL_SIZE/2;
        _current_item = i;
        _cursor_sprite.set_position(i->get_x() + _offset_x, i->get_y() + _offset_y);
        redraw();
    }

    static Cursor *change_cursor(Cursor *from, Cursor *to) {
        if (from != NULL) from->toggle_visible();
        if (to != NULL) to->toggle_visible();
        return to;
    }

    void set_offset(int x, int y) {
        _offset_x = x;
        _offset_y = y;
        redraw();
    }

    void redraw() {
        _cursor_sprite.set_position(
            _current_item->get_x() + _offset_x,
            _current_item->get_y() + _offset_y);
    }

    void up() {
        if (_current_item->get_up())
            _current_item = _current_item->get_up();
        redraw();
    }

    void down() {
        if (_current_item->get_down())
            _current_item = _current_item->get_down();
        redraw();
    }

    void left() {
        if (_current_item->get_left())
            _current_item = _current_item->get_left();
        redraw();
    }

    void right() {
        if (_current_item->get_right())
            _current_item = _current_item->get_right();
        redraw();
    }

    typedef void (*F)(Item*, void *p);
    void on_a(F f, void *p) {
        _current_item->on_a(f, p);
    }

    void toggle_visible() {
        _cursor_sprite.set_visible(!_cursor_sprite.visible());
    }

};

class Settings {
    public:
    static constexpr int bg_cols = 32;
    static constexpr int bg_rows = 32;

    static constexpr int world_cols = 20;
    static constexpr int world_rows = 16;

    static constexpr int world_offset_x = (bg_cols - world_cols) / 2;
    static constexpr int world_offset_y = (bg_rows - world_rows) / 2;

    enum MenuItem { NEW, SAVE, LOAD };
    
    private:
    alignas(int) bn::regular_bg_map_cell cells[bg_cols * bg_rows];
    bn::regular_bg_map_item map_item;
    bn::regular_bg_item bg_item;
    bn::regular_bg_ptr bg;
    bn::regular_bg_map_ptr bg_map;

    Cursor *_cursor;

    Item *_menu_items[3];

    Item *_rules_items[2][9];

    Item * _new_item(int x, int y, int id) {
            Item *i = new Item(x * 8 , y * 8, get_cell(x,y));
            i->set_state(id); 
            return i;

    }
    public:
    Settings() :
        map_item(cells[0], bn::size(
            bg_cols,
            bg_rows)),
        bg_item(bn::regular_bg_tiles_items::cell,
            bn::bg_palette_items::palette,
            map_item),
        bg(bg_item.create_bg(0,0)),
        bg_map(bg.map())
    {
        bn::memory::clear(bg_rows * bg_cols, cells[0]);

        for (int i = 0; i < world_cols; i++) {
            set_cell(i,0,4);
            for (int j = 1; j < world_rows-1; j++)
                set_cell(i,j,1);
            set_cell(i,world_rows-1,4, false, true);
        }
        for (int j = 0; j < world_rows; j++) {
            set_cell(0,j,5);
            set_cell(world_cols-1,j,5, true, false);
        }
        set_cell(0,0,3);
        set_cell(world_cols-1,0,3,true,false);
        set_cell(0,world_rows-1,3,false,true);
        set_cell(world_cols-1,world_rows-1,3,true,true);

        _menu_items[NEW] = _new_item(3,6,0);
        _menu_items[SAVE] = _new_item(3,7,0);
        _menu_items[LOAD] = _new_item(3,8,0);

        _menu_items[NEW]->set_id(bn::string<8>("NEW"));
        _menu_items[SAVE]->set_id(bn::string<8>("SAVE"));
        _menu_items[LOAD]->set_id(bn::string<8>("LOAD"));

        _menu_items[NEW]->set_down(_menu_items[SAVE]);
        _menu_items[SAVE]->set_down(_menu_items[LOAD]);
        _menu_items[LOAD]->set_down(_menu_items[NEW]);
        _menu_items[NEW]->set_up(_menu_items[LOAD]);
        _menu_items[LOAD]->set_up(_menu_items[SAVE]);
        _menu_items[SAVE]->set_up(_menu_items[NEW]);

        // NEW
        set_cell(4,6,32);
        set_cell(5,6,33);
        set_cell(6,6,34);
        set_cell(7,6,35);

        // SAVE
        set_cell(4,7,24);
        set_cell(5,7,25);
        set_cell(6,7,26);
        set_cell(7,7,27);

        // LOAD
        set_cell(4,8,28);
        set_cell(5,8,29);
        set_cell(6,8,30);
        set_cell(7,8,31);

        // Rules
        set_cell(1,2,8);
        set_cell(1,3,9);
        for (int i = 0; i < 9; i++) {
            set_cell(2+i,1,'0'+i);
            _rules_items[0][i] = _new_item(2+i,2,_rule.dead[i]);
            _rules_items[1][i] = _new_item(2+i,3,_rule.live[i]);
            _rules_items[1][i]->set_down(_menu_items[NEW]);
            
            char id[5];
            id[0] = 'R';
            id[1] = 'B';
            id[2] = '0'+i;
            id[3] = '\0';
            _rules_items[0][i]->set_id(id);
            id[1] = 'S';
            _rules_items[1][i]->set_id(id);
        }

        _menu_items[NEW]->set_up(_rules_items[1][0]);

        for (int i = 0; i < 9; i++) {
            _rules_items[0][i]->set_down(_rules_items[1][i]);
            _rules_items[1][i]->set_up(_rules_items[0][i]);
            if (i+1 < 9) {
                _rules_items[0][i]->set_right(_rules_items[0][i+1]);
                _rules_items[1][i]->set_right(_rules_items[1][i+1]);
            }
            if (i-1 >= 0) {
                _rules_items[0][i]->set_left(_rules_items[0][i-1]);
                _rules_items[1][i]->set_left(_rules_items[1][i-1]);
            }
        }

        _cursor = new Cursor(world_cols, world_rows, get_cell(NEW));

        redraw();
    }

    void set_cell(int x, int y, int id) {
        set_cell(x, y, id, false, false);
    }

    void set_cell(int x, int y, int id, bool hflip, bool vflip) {
        bn::regular_bg_map_cell *c = &cells[map_item.cell_index(x + world_offset_x, y + world_offset_y)];
        bn::regular_bg_map_cell_info c_info(*c);
        c_info.set_tile_index(id);
        c_info.set_horizontal_flip(hflip);
        c_info.set_vertical_flip(vflip);
        *c = c_info.cell();
    }

    bn::regular_bg_map_cell *get_cell(int x, int y) {
        return &cells[map_item.cell_index(x + world_offset_x, y + world_offset_y)];
    }

    Item *get_cell(MenuItem mi) {
        return _menu_items[mi];
    }

    Cursor *get_cursor() {
        return _cursor;
    }

    void set_visible(bool b) {
        bg.set_visible(b);
    }

    void redraw() {
        for (int i = 0; i < 9; i++) {
            _rules_items[0][i]->set_state(_rule.dead[i]);
            _rules_items[1][i]->set_state(_rule.live[i]);
        }
            
        bg_map.reload_cells_ref();
    }

};

class World {
    public:
    static constexpr int bg_cols = 32;
    static constexpr int bg_rows = 32;

    static constexpr int world_cols = 30;
    static constexpr int world_rows = 20;

    static constexpr int world_offset_x = (bg_cols - world_cols) / 2;
    static constexpr int world_offset_y = (bg_rows - world_rows) / 2;

    private:
    alignas(int) bn::regular_bg_map_cell cells[World::bg_cols * World::bg_rows];
    bn::regular_bg_map_item map_item;
    bn::regular_bg_item bg_item;
    bn::regular_bg_ptr bg;
    bn::regular_bg_map_ptr bg_map;

    bool _torus = true;

    Item *_world[bg_cols][bg_rows];


    Cursor *_cursor;

    enum cell_states {DEAD, LIVE};

    public:
    World() :
        map_item(cells[0], bn::size(
            bg_cols,
            bg_rows)),
        bg_item(bn::regular_bg_tiles_items::cell,
            bn::bg_palette_items::palette,
            map_item),
        bg(bg_item.create_bg(0,0)),
        bg_map(bg.map())
    {
        bn::memory::clear(bg_rows * bg_cols, cells[0]);
        for (int i = 0; i < world_cols; i++) {
        for (int j = 0; j < world_rows; j++) {
            _world[i][j] = new Item(i * 8 , j * 8,
                                &cells[map_item.cell_index(
                                    i+world_offset_x,
                                    j+world_offset_y)]);
            _world[i][j]->set_state(DEAD);
        }}

        for (int i = 0; i < world_cols; i++) {
        for (int j = 0; j < world_rows; j++) {
            if ( i - 1 >= 0) _world[i][j]->set_left(_world[i-1][j]);
            if ( j - 1 >= 0) _world[i][j]->set_up(_world[i][j-1]);
            if ( i + 1 < world_cols) _world[i][j]->set_right(_world[i+1][j]);
            if ( j + 1 < world_rows) _world[i][j]->set_down(_world[i][j+1]);
        }}
        bg_map.reload_cells_ref();
    
        _cursor = new Cursor(world_cols, world_rows, get_cell(0,0));

    }

    Item *get_cell(int x, int y) {
        return _world[x][y];
    }

    void next_generation() {
        bn::vector<bn::vector<int, world_rows>, world_cols> neighbors;
        neighbors.resize(world_cols);
        for (int i = 0; i < world_cols; i++) {
        neighbors[i].resize(world_rows);
        for (int j = 0; j < world_rows; j++) {
            neighbors[i][j] = 0;
            for (int ii = i - 1; ii <= i + 1; ii++) {
            for (int jj = j - 1; jj <= j + 1; jj++) {
                if (ii != i || jj != j) {
                    if (_torus) {
                        int x = ii < 0 ? world_cols -1 : ii;
                        int y = jj < 0 ? world_rows -1 : jj;
                        neighbors[i][j] += _world[x % world_cols][y % world_rows]->get_state();
                    } else if (ii >= 0 && jj >= 0 && ii < world_cols && jj < world_rows)
                        neighbors[i][j] += _world[ii][jj]->get_state();
                }
            }}
        }}

        for (int i = 0; i < world_cols; i++) {
        for (int j = 0; j < world_rows; j++) {
            int n = neighbors[i][j];
            Item *cell = _world[i][j];
            if (cell->get_state()) {
                    cell->set_state(_rule.live[n]);
            } else {
                    cell->set_state(_rule.dead[n]);
            }
        }}
        redraw();

    }

    void clear() {
        for (int i = 0; i < world_cols; i++)
        for (int j = 0; j < world_rows; j++)
            _world[i][j]->set_state(false);
        redraw();
    }
    
    void redraw() {
        bg_map.reload_cells_ref();
    }

    Cursor *get_cursor() {
        return _cursor;
    }

    void set_visible(bool b) {
        bg.set_visible(b);
    }

    void save() {
        for (int j = 0; j < (world_rows); j++) {
        for (int i = 0; i < (world_cols); i++) {
            bn::sram::write_offset(_world[i][j]->get_state(), SRAM_WORLD_OFFSET + (j * (world_cols + 1) + i) * sizeof(int));
        }}
    }

    void load() {
        BN_LOG("SIZE: ", sizeof(cells[0]));
        for (int j = 0; j < (world_rows); j++) {
        for (int i = 0; i < (world_cols); i++) {
            int state;
            bn::sram::read_offset(state, SRAM_WORLD_OFFSET + (j * (world_cols + 1) + i) * sizeof(int));
            _world[i][j]->set_state(state);
        }}
        redraw();
    }

};


void func (Item *i) {
    i->set_state(!i->get_state());
}


struct Data {
    Settings *s;
    World *w;
};

typedef struct Data Data;
int main()
{
    bn::core::init();
    World world;

    bn::timer t = bn::timer();

    enum Status { running = 1, paused = 2, settings = 3};
    Status status = paused;
    
    Settings settings_menu;
    settings_menu.get_cursor()->toggle_visible();
    settings_menu.set_visible(false);

    Cursor *cursor = world.get_cursor();

    Cursor *cursor_running = NULL;

    while(true)
    {
        if (status == running) {
            if (bn::keypad::b_pressed()) {
                status = paused;
                cursor = Cursor::change_cursor(cursor, world.get_cursor());
                BN_LOG("PAUSING");
            }
            if (t.elapsed_ticks() > 50000) {
                world.next_generation();
                t.restart();
            }
            if (bn::keypad::up_pressed()) {
                if (cursor != NULL) cursor->up();
            }
            if (bn::keypad::down_pressed()) {
                if (cursor != NULL) cursor->down();
            }
            if (bn::keypad::left_pressed()) {
                if (cursor != NULL) cursor->left();
            }
            if (bn::keypad::right_pressed()) {
                if (cursor != NULL) cursor->right();
            }
            if (bn::keypad::start_pressed()) {
                settings_menu.set_visible(true);
                cursor = Cursor::change_cursor(cursor, settings_menu.get_cursor());
                status = settings;
            }
        } else if (status == paused) {
            if (bn::keypad::up_pressed()) {
                cursor->up();
            }
            if (bn::keypad::down_pressed()) {
                cursor->down();
            }
            if (bn::keypad::left_pressed()) {
                cursor->left();
            }
            if (bn::keypad::right_pressed()) {
                cursor->right();
            }
            if (bn::keypad::a_pressed()) {
                cursor->on_a([](Item *i, void *p) {
                    i->set_state(!i->get_state());
                    ((World*)p)->redraw();
                }, &world);
            }
            if (bn::keypad::start_pressed()) {
                settings_menu.set_visible(true);
                cursor = Cursor::change_cursor(cursor, settings_menu.get_cursor());
                status = settings;
            }
            if (bn::keypad::b_pressed()) {
                cursor = Cursor::change_cursor(cursor, cursor_running);
                status = running;
            }
        } else if (status == settings) {
            if (bn::keypad::b_pressed()) {
                status = paused;
                cursor = Cursor::change_cursor(cursor, world.get_cursor());
                settings_menu.set_visible(false);
                world.set_visible(true);
            }
            if (bn::keypad::up_pressed()) {
                cursor->up();
            }
            if (bn::keypad::down_pressed()) {
                cursor->down();
            }
            if (bn::keypad::left_pressed()) {
                cursor->left();
            }
            if (bn::keypad::right_pressed()) {
                cursor->right();
            }
            if (bn::keypad::a_pressed()) {
                Data data;
                data.s = &settings_menu;
                data.w = &world;
                cursor->on_a([](Item *i, void *p) {
                    BN_LOG("ID: ", i->get_id());

                    Settings *s = ((Data*) p)->s;
                    World *w = ((Data*) p)->w;

                    char *id = i->get_id();
                    char *r = NULL;
                    if (id[0] == 'R') {
                        if (id[1] == 'B')
                            r = &(_rule.dead[id[2]-'0']);
                        if (id[1] == 'S')
                            r = &(_rule.live[id[2]-'0']);
                    }
                    if (r != NULL) {
                        *r = !*r;
                        i->set_state(*r);
                    }

                    // Reset Game
                    if (bn::string<8>(i->get_id()) == bn::string<8>("NEW")) {
                        _rule = _life;
                        w->clear();
                    }
                    // Save Game
                    if (bn::string<8>(i->get_id()) == bn::string<8>("SAVE")) {
                        bn::sram::write_offset(_rule, SRAM_RULE_OFFSET);
                        w->save();
                    }
                    // Load Game
                    if (bn::string<8>(i->get_id()) == bn::string<8>("LOAD")) {
                        bn::sram::read_offset(_rule, SRAM_RULE_OFFSET);
                        w->load();
                    }

                    s->redraw();
                }, &data);

            }
        }
        bn::core::update();
    }
}
