#include <ncurses/ncurses.h>
#include <cstdlib>
#include <unordered_set>
#include <utility>
#include <functional>
#include <iostream>

struct PairHash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

struct Position{
    int x;
    int y;
    Position(): x(0), y(0){};
};

class Board{
private:
    int row;
    int column;
    int *num_board;
    bool *revealed_board;
    std::unordered_set<std::pair<int, int>, PairHash> mine_positions; // The positions are stored in (width, height) format
    Position cursor_position;
    const std::pair<int, int> cell_directions[8] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    
    void force_print_position(int c, int r){ // x, y
        int x, y; 
        x = c * 4 + 2;
        y = r * 2 + 1;
        wattron(this->board, COLOR_PAIR(num_board[this->column*r + c]));
        mvwprintw(this->board, y, x, "%d", num_board[this->column*r + c]);
        wattroff(this->board, COLOR_PAIR(num_board[this->column*r + c]));
        wrefresh(this->board);
    }

    bool valid_position(int nx, int ny){ // x, y
        return (nx >= 0 && nx < this->column && ny >= 0 && ny < this->row);
    }

    bool is_mine(int c, int r){
        return num_board[this->column*r + c] == -1;
    }
public:
    WINDOW * board;
    int board_height;
    int board_width;

    int num_mines;
    Board(int r, int c, int m): row(r), column(c), num_mines(m){
        int Y_SIZE, X_SIZE;
        getmaxyx(stdscr, Y_SIZE, X_SIZE);
        this->board_height = 2*row+1;
        this->board_width = 4*column+1;
        // I might put a check before creating the board
        this->board = newwin(board_height, board_width, (Y_SIZE/2)-(board_height/2), (X_SIZE/2)-(board_width/2));
        refresh();
        box(board, 0, 0);

        num_board = new int[row*column];
        revealed_board = new bool [row*column];
        for (int i = 0; i < row*column; i++){
            revealed_board[i] = false;
            num_board[i] = 0;
        }
        srand(time(NULL));
        while (mine_positions.size() != num_mines){
            int mine_x = rand() % (column);
            int mine_y = rand() % (row);
            std::pair<int, int> mp = std::make_pair(mine_x, mine_y);
            auto res = this->mine_positions.insert(mp);
            if (res.second){
                num_board[mine_y*this->column+mine_x] = -1;
                for (std::pair<int, int> d: cell_directions){
                    int nx = mine_x + d.first;
                    int ny = mine_y + d.second;
                    if (valid_position(nx, ny)) {
                        int index = ny * column + nx;
                        if (num_board[index] != -1) {
                            num_board[index]++;
                        }
                    }
                }
                mvprintw(mine_positions.size(), 0, "Mine position: (%d, %d)", mine_x, mine_y);
            }

        }

        for (int i = 1; i < row; ++i) mvwhline(board, 2*i, 1, ACS_HLINE, board_width-2);
        for (int j = 1; j < column; ++j) mvwvline(board, 1, 4*j, ACS_VLINE, board_height-2);
        for (int i = 1; i < row; ++i){
            for (int j = 1; j < column; ++j) mvwaddch(board, 2*i, 4*j, ACS_PLUS);
        }
        wrefresh(board);
        print_position();
    }    

    void clear_position(){ // To delete the standout effect on the cell 
        int x, y, r, c;
        r = cursor_position.y;
        c = cursor_position.x;
        x = c * 4 + 2;
        y = r * 2 + 1;
        if (revealed_board[this->column*r + c]) {
            wattron(this->board, COLOR_PAIR(num_board[this->column*r + c]));
            mvwprintw(this->board, y, x, "%d", num_board[this->column*r + c]);
            wattroff(this->board, COLOR_PAIR(num_board[this->column*r + c]));
        }
        else mvwaddch(this->board, y, x, ' ');
        wrefresh(this->board);
    }

    void print_position(){
        int x, y, r, c;
        r = cursor_position.y;
        c = cursor_position.x;
        x = c * 4 + 2;
        y = r * 2 + 1;
        wattron(this->board, A_STANDOUT);
        if (revealed_board[this->column*r + c]) mvwprintw(this->board, y, x, "%d", num_board[this->column*r + c]);
        else mvwaddch(this->board, y, x, ' ');
        wattroff(this->board, A_STANDOUT);
        wrefresh(this->board);
    };

    void click(int x, int y){
        if (revealed_board[column*y+x]) return;
        revealed_board[column*y+x] = true;
        if (num_board[column*y+x] == 0){
            for (std::pair<int, int> d: cell_directions){
                int nx = x + d.first;
                int ny = y + d.second;
                if (valid_position(nx, ny)){
                    click(nx, ny);
                    force_print_position(nx, ny);
                }
            }
        }
    }

    void handle_trigger(int ch){
        clear_position();
        switch(ch){
            case KEY_UP:
                if (this->cursor_position.y > 0) this->cursor_position.y--;
                break; 
            case KEY_DOWN:
                if (this->cursor_position.y < this->row - 1) this->cursor_position.y++;
                break;
            case KEY_LEFT:
                if (this->cursor_position.x > 0) this->cursor_position.x--;
                break;
            case KEY_RIGHT:
                if (this->cursor_position.x < this->column - 1) this->cursor_position.x++;
                break;
            case KEY_ENTER: case 10 : case 13: 
                click(this->cursor_position.x, this->cursor_position.y);
                break;
        }
        mvprintw(0, 0, "Position: (%d, %d)   ", this->cursor_position.x, this->cursor_position.y);
        print_position();
    }
};

int main(int argc, char ** argv){
    initscr();
    if (!has_colors()) {
        puts("Your terminal does not support colors mm..");
        getch();
        return -1;
    }
    noecho();
    cbreak();
    curs_set(1);
    keypad(stdscr, TRUE);
    start_color();

    init_pair(0, COLOR_WHITE, COLOR_BLACK);
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(8, COLOR_BLACK, COLOR_WHITE);

    int Y_SIZE, X_SIZE;
    int BOARD_HEIGHT, BOARD_WIDTH;
    BOARD_HEIGHT = 10;
    BOARD_WIDTH = 10;
    getmaxyx(stdscr, Y_SIZE, X_SIZE);

    Board board(BOARD_HEIGHT, BOARD_WIDTH, 15);

    int ch;
    while(ch = getch(), ch != 'x'){
        board.handle_trigger(ch);
    }
    wattron(stdscr, A_STANDOUT);
    mvprintw(Y_SIZE-1, 0, "Press any key to exit...");
    wattroff(stdscr, A_STANDOUT);
    getch();
    endwin();
}