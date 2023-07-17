#include <iostream>
#include <vector>
#include <deque>
#include <raylib.h>
#include <cstdlib>
#include <ctime>
#include <map>

typedef std::pair<int,int> pii_t;

// Constants
const int screen_width = 1600;
const int screen_height = 900;
const int fps = 120;
const int cell_size = 20;
const int wall_size = 4;

const int maze_height = screen_height - 200;

const int columns = screen_width / cell_size;
const int rows = maze_height / cell_size;

std::vector<pii_t> adj_cells[columns][rows]; // c x r array of adj matrices of adjacent cells
std::vector<pii_t> open_adj_cells[columns][rows]; // c x r array of adj matrices of adjacent cells without a wall
pii_t cell_locations[columns][rows];

std::vector<pii_t> visit_stack; // stack of cells passed, top is to visit
bool visited[columns][rows] = {0};

bool maze_complete = false;

int state = 0; // 0 is building maze, 1 is traversing


// Displaying text

const int button_height = 100;
const int button_font_size = 80;

class Button {
    public:
    char* text;
    float x, y;
    int button_width;

    Button(float x_, float y_, char* text_) {
        text = text_;
        button_width = MeasureText(text_, button_font_size);
        x = x_ - button_width /2;
        y = y_ - button_height/2;
    }

    void displayButton() {
        DrawRectangle(x-10, y, button_width+20, button_height, LIGHTGRAY);
        DrawText(text, x, y+10, button_font_size, BLACK);
    }
};

void display_background() {
    DrawRectangle(0, 700, 1600, 200, BLACK);
}

// Initialization
void initialize_cell_locations() {
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < rows; j++) {
            cell_locations[i][j] = std::make_pair(i * cell_size, j * cell_size);
        }
    }
}

/**
 * Initializes the adjacents cells matrix 
*/
void initialize_adjacent_cells() {
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < rows; j++) {
            if (i != 0) {adj_cells[i][j].push_back(std::make_pair(i-1, j));}
            if (i != columns-1) {adj_cells[i][j].push_back(std::make_pair(i+1, j));}
            if (j != 0) {adj_cells[i][j].push_back(std::make_pair(i, j-1));}
            if (j != rows-1) {adj_cells[i][j].push_back(std::make_pair(i, j+1));}
        }
    }
}

void initialize_stack() {
    visit_stack.push_back(std::make_pair(0,0));
}

void draw_boxes() {
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < rows; j++) {
            DrawRectangle(cell_locations[i][j].first + wall_size/2, cell_locations[i][j].second + wall_size/2, cell_size - wall_size, cell_size - wall_size, WHITE);
        }
    }
}

/**
 * Visually deletes the wall between the two cells in the display
*/
void clear_wall_visual(pii_t cell1, pii_t cell2) {
    if (cell1.first == cell2.first + 1) {
        DrawRectangle(cell_locations[cell2.first][cell2.second].first + cell_size - wall_size/2, cell_locations[cell2.first][cell2.second].second + wall_size/2, wall_size, cell_size - wall_size, WHITE);
    }

    else if (cell2.first == cell1.first + 1) {
        DrawRectangle(cell_locations[cell1.first][cell1.second].first + cell_size - wall_size/2, cell_locations[cell1.first][cell1.second].second + wall_size/2, wall_size, cell_size - wall_size, WHITE);
    }

    else if (cell1.second == cell2.second + 1) {
        DrawRectangle(cell_locations[cell2.first][cell2.second].first + wall_size/2, cell_locations[cell2.first][cell2.second].second + cell_size - wall_size/2, cell_size - wall_size, wall_size,  WHITE);
    }
    
    else if (cell2.second == cell1.second + 1) {
        DrawRectangle(cell_locations[cell1.first][cell1.second].first + wall_size/2, cell_locations[cell1.first][cell1.second].second + cell_size - wall_size/2, cell_size - wall_size, wall_size,  WHITE);
    }
}

/**
 * Deletes the wall between two cells by adding to adjacency matrices
*/
void clear_wall(pii_t cell1, pii_t cell2) {   
    open_adj_cells[cell1.first][cell1.second].push_back(std::make_pair(cell2.first, cell2.second));
    open_adj_cells[cell2.first][cell2.second].push_back(std::make_pair(cell1.first, cell1.second));

    clear_wall_visual(cell1, cell2);
}

void draw_maze() {
    draw_boxes();
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < rows; j++) {
            for (pii_t cell : open_adj_cells[i][j]) {
                clear_wall_visual(std::make_pair(i, j), cell);
            }
        }
    }
}

void highlight_cell(pii_t cell, Color c) {
    DrawRectangle(cell_locations[cell.first][cell.second].first + wall_size/2, cell_locations[cell.first][cell.second].second + wall_size/2, cell_size-wall_size, cell_size-wall_size, c);
}

void highlight_start_end() {
    DrawRectangle(wall_size/2, wall_size/2, cell_size-wall_size, cell_size-wall_size, GREEN);

    if (visited[columns-1][rows-1]) {
        DrawRectangle(cell_locations[columns-1][rows-1].first+wall_size/2, cell_locations[columns-1][rows-1].second+wall_size/2, cell_size-wall_size, cell_size-wall_size, YELLOW);
    }
   
} 

void next_step_maze_generation() {
    pii_t cell = visit_stack.back();
    visited[cell.first][cell.second] = 1;

    highlight_cell(cell, MAGENTA);

    // Removing visited cells from adjacent cells matrix
    for (int i = adj_cells[cell.first][cell.second].size()-1; i >= 0; i--) {
        pii_t c = adj_cells[cell.first][cell.second][i];

        if (visited[c.first][c.second]) {
            adj_cells[cell.first][cell.second].erase(adj_cells[cell.first][cell.second].begin() + i);
        }
    }

    // Dead end, so backtrack
    if (adj_cells[cell.first][cell.second].empty()) {
        visit_stack.pop_back();
        return;
    }

    // Random unvisited adj cell
    pii_t to_travel = adj_cells[cell.first][cell.second][(rand() % adj_cells[cell.first][cell.second].size())];
    clear_wall(cell, to_travel);
    visit_stack.push_back(to_travel);
}

// Traversing
std::map<pii_t, pii_t> dfs_path;
std::map<pii_t, pii_t> bfs_path;

bool trav_visited[columns][rows] = {0};

void highlight_traversal() {
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < rows; j++) {
            if (trav_visited[i][j]) {
                DrawRectangle(cell_locations[i][j].first + wall_size/2, cell_locations[i][j].second + wall_size/2, cell_size - wall_size, cell_size-wall_size, BLUE);
            }
        }
    }
}

// DFS
std::vector<pii_t> dfs_stack;

void initialize_dfs() {
    dfs_stack.push_back(std::make_pair(0,0));
}

void dfs_step() {
    pii_t cell = dfs_stack.back();
    dfs_stack.pop_back();

    highlight_cell(cell, MAGENTA);

    if (trav_visited[cell.first][cell.second]) {return;}
    
    trav_visited[cell.first][cell.second] = 1;

    for (pii_t adj_cell : open_adj_cells[cell.first][cell.second]) {
        dfs_stack.push_back(std::make_pair(adj_cell.first, adj_cell.second));

        if (!dfs_path.count(adj_cell)) {
            dfs_path[adj_cell] = cell;
        }
    }
}


// BFS
std::deque<pii_t> bfs_queue;

void initialize_bfs() {
    bfs_queue.push_back(std::make_pair(0,0));
}

void bfs_step() {
    pii_t cell = bfs_queue.front();
    bfs_queue.pop_front();

    highlight_cell(cell, MAGENTA);

    if (trav_visited[cell.first][cell.second]) {return;}

    trav_visited[cell.first][cell.second] = 1;

    for (pii_t adj_cell : open_adj_cells[cell.first][cell.second]) {
        bfs_queue.push_back(std::make_pair(adj_cell.first, adj_cell.second));

        if (!bfs_path.count(adj_cell)) {
            bfs_path[adj_cell] = cell;
        }
    }
}

void display_path(std::map<pii_t,pii_t> path, Color c) {
    if (!path.count(std::make_pair(columns-1,rows-1))) {return;}

    pii_t cell = std::make_pair(columns-1,rows-1);
    while (1) {
        highlight_cell(cell, c);

        if (cell == std::make_pair(0,0)) {
            break;
        }

        cell = path[cell];
    }
}

void reset_trav() {
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < rows; j++) {
            trav_visited[i][j] = 0;
        }
    }
}

int main () {
    // Getting random seed for rand
    srand(time(NULL));

    InitWindow(screen_width, screen_height, "Maze Generation");
    SetTargetFPS(fps); // Without this line, fps will be uncapped

    initialize_cell_locations();
    initialize_adjacent_cells();
    initialize_stack();

    // Initialize traversals
    initialize_bfs();
    initialize_dfs();
    draw_boxes();


    Button test = Button(800, 800, "Generating maze...");

    /**
     * Drawing things
     * -------------------
     * void DrawRectangle(int posX, int posY, int width, int height, Color color)
     * void DrawCircle(int centerX, int centerY, float radius, Color color)
     * void DrawLine(int startPosX, int startPostY, int endPosX, int endPosY, Color color)
     * void DrawPoly(Vector2 center, int sides, float radius, float rotation, Color color)
    */

    // Game loop
    while (WindowShouldClose() == false) {
        BeginDrawing(); // Creates blank canvas

        ClearBackground(BLACK);
        display_background();
        draw_maze();

        display_path(dfs_path, GREEN);
        display_path(bfs_path, GREEN);

        if (state == 0) {
            if (!maze_complete) {
                next_step_maze_generation();
            }
            
            if (visit_stack.empty()) {
                maze_complete = 1;
                state = 1;
                test = Button(800, 800, "Depth First Search in progress");
            }
        }

        else if (state == 1) {
            highlight_traversal();
            dfs_step();

            if (trav_visited[columns-1][rows-1]) {
                state = 2;
                reset_trav();
                test = Button(800, 800, "Breadth First Search in progress");
            }
        }
        
        else if (state == 2) {
            highlight_traversal();
            bfs_step();

            if (trav_visited[columns-1][rows-1]) {
                state = 3;
                reset_trav();
                test = Button(800, 800, "Traversal complete");
            }
        }
  
        highlight_start_end();

        test.displayButton();
        
        EndDrawing(); // Ends canvas drawing
    }

    CloseWindow();
    return 0;
}