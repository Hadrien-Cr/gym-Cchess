#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "engine.cpp"

namespace py = pybind11;

class MyEnv {
public:
    MyEnv() : board(start_position) {}

    void reset(std::string fen) {
        char* fen_char = new char[fen.length() + 1];
        strcpy(fen_char, fen.c_str());
        board.parse_fen(fen_char);
        delete[] fen_char;
    }

    std::tuple<py::array_t<double>, double, bool> step(int move) {
        if (board.apply_move(move, 0) == 0) {
            // Invalid move
            return std::make_tuple(get_observation(), -1.0, true);
        }

        move_list move_list[1];
        board.generate_moves(move_list);
        if (move_list->move_count == 0) {
            // Game over
            return std::make_tuple(get_observation(), 0.0, true);
        }

        return std::make_tuple(get_observation(), 0.0, false);
    }

    int environment_move() {
        board.search_position(1); // Search for 1 second
        int best_move = board.pv_table[0][0];
        board.apply_move(best_move, 0);
        return best_move;
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

PYBIND11_MODULE(myenv_cpp, m) {
    py::class_<MyEnv>(m, "MyEnv")
        .def(py::init<>())
        .def("reset", &MyEnv::reset)
        .def("step", &MyEnv::step)
        .def("environment_move", &MyEnv::environment_move)
        .def("get_observation", &MyEnv::get_observation)
        .def("print_board", &MyEnv::print_board);
}