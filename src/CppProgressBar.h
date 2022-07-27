#ifndef CPP_PROGRESS_BAR_H
#define CPP_PROGRESS_BAR_H

#include <iostream>
#include <string>
#include <cmath>

class CppProgressBar {
private:
    size_t progress_loop_number_{};
    std::string progress_bar_;
    std::string empty_line_{std::string(200, ' ')+"\r"};
    double percentage_{};
    double oneHundredth_{};
    size_t one{};
    std::string progress_line_;
    size_t loop_counter_{};
public:
    inline void cntSet(size_t first) {
        loop_counter_ = first;
    }
    inline void cntIncrement() {
        loop_counter_++;
        update_variable();
    }
    CppProgressBar() noexcept = default;
    void init_variable(size_t loop_number);
    void update_variable();
//    inline void finish_progress_bar() {
//        for (size_t j = 0; j < 100 && j < progress_bar_.length() + 1; ++j) {
//            if (progress_bar_[j] != '=') {
//                progress_bar_[j] = '=';
//            }
//        }
//        std::clog << "100% [" << progress_bar_ << ']' << std::endl;
//    };
    inline void stdout_in_for_progress (std::string& e) {
        std::clog << empty_line_;
        std::cout << e;
        std::clog << progress_line_;
    }
};
#endif /* CPP_PROGRESS_BAR_H */