#include "CppProgressBar.h"

void CppProgressBar::init_variable(size_t loop_number) {
    progress_loop_number_ = loop_number;
    percentage_ = 0;
    oneHundredth_ = double(loop_number) / 100.0;
    one = loop_number / 1000;
    if (!one) one = 1;
    progress_bar_.resize(100, '.');
}

void CppProgressBar::update_variable() {
    if (loop_counter_ % one == 0) {
        double percentage_now = double(loop_counter_) / oneHundredth_;
        if (percentage_ == percentage_now) {
            return;
        }
        percentage_ = percentage_now;
        size_t j;
        for (j = 0; j < size_t(std::floor(percentage_)); ++j) {
            if (progress_bar_[j] != '=') {
                progress_bar_[j] = '=';
            }
        }
        progress_bar_[j] = '>';
        if (percentage_ < 10)
            progress_line_ = "  " + std::to_string(percentage_) + "% [" + progress_bar_ + "]                     " +
                             std::to_string(loop_counter_) + "/" + std::to_string(progress_loop_number_) + "\r";
        else if (percentage_ == 100)
            progress_line_ = std::to_string(percentage_) + "% [" + progress_bar_ + "]                     " +
                             std::to_string(loop_counter_) + "/" + std::to_string(progress_loop_number_) + "\r";
        else
            progress_line_ = ' ' + std::to_string(percentage_) + "% [" + progress_bar_ + "]                     " +
                             std::to_string(loop_counter_) + "/" + std::to_string(progress_loop_number_) + "\r";

        std::clog << empty_line_;
        std::clog << progress_line_;
    }
}
