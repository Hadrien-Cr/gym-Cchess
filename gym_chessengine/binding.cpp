#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "engine.cpp"

namespace py = pybind11;

class PyChessBoard {
public:
    PyChessBoard() : board(start_position) {}

    void reset(std::string fen) {
        char* fen_char = new char[fen.length() + 1];
        strcpy(fen_char, fen.c_str());
        board.parse_fen(fen_char);
        delete[] fen_char;
    } 
    
    void init_engine() {
        board.init_nnue("gym_chessengine/nn-eba324f53044.nnue");
    }

    std::tuple<py::array_t<double>, double, bool> step(int a) {
        UndoInfo undo;
        move_list move_list[1];
        board.generate_moves(move_list);
        int move;

        for (int i = 0; i < move_list->move_count; i++) {
            int64_t _move = move_list->moves[i];
            if (decode_move_source(_move) * 64 + decode_move_target(_move) == a) {
                move = _move;
                break;
            }
        }
        if (board.make_move(move) == 0) {
            return std::make_tuple(get_observation(), -1.0, true);
        }
    }

    std::string current_side(){
        if (board.side_to_move == 0) {
            return "white";
        }
        else {
            return "black";
        }
    }

    int environment_move(int depth) {
        board.search_position(depth);
        int move = board.pv_table[0][0];
        int start_square = decode_move_source(move);
        int end_square = decode_move_target(move);
        return start_square * 64 + end_square;
    }

    py::array_t<double> get_observation() {
        auto obs = py::array_t<double>({12, 8, 8});
        py::buffer_info buf = obs.request();
        double *ptr = static_cast<double *>(buf.ptr);

        for (int i = 0; i < 12 * 8 * 8; ++i) {
            ptr[i] = 0;
        }

        for (int piece = 0; piece < 12; ++piece) {
            for (int rank = 0; rank < 8; ++rank) {
                for (int file = 0; file < 8; ++file) {
                    if (get_bit(board.piece_bitboards[piece], rank * 8 + file)) {
                        ptr[piece * 64 + rank * 8 + file] = 1.0;
                    }
                }
            }
        }
        return obs;
    }

    void print_board() {
        board.print_board();
    }

private:
    ChessBoard board;
};

PYBIND11_MODULE(binding, m) {
    py::class_<PyChessBoard>(m, "PyChessBoard")
        .def(py::init<>())
        .def("reset", &PyChessBoard::reset)
        .def("init_engine", &PyChessBoard::init_engine)
        .def("step", &PyChessBoard::step)
        .def("environment_move", &PyChessBoard::environment_move)
        .def("current_side", &PyChessBoard::current_side)
        .def("get_observation", &PyChessBoard::get_observation)
        .def("print_board", &PyChessBoard::print_board);
}