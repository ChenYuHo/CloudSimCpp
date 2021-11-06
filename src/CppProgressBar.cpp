#include "CppProgressBar.h"

void CppProgressBar::init_variable(size_t loop_number) {
    progress_loop_number_ = loop_number;
    percentage_ = 0;
    oneHundledth_ = loop_number / 100.0;
    progress_bar_.resize(100, '.');
}

void CppProgressBar::update_variable() {
    percentage_ = loop_counter_ / oneHundledth_;
    size_t k = 0;
    for (size_t j = 0; j < percentage_; ++j) {
        k = std::round(j / 100.0 * progress_bar_.length());
        if (progress_bar_[k] != '=') {
            progress_bar_[k] = '=';
        }
    }
    progress_bar_[k] = '>';
    if (percentage_ < 10)
        progress_line_ = "  " + std::to_string(percentage_) + "% [" + progress_bar_ + "]                     "+std::to_string(loop_counter_)+"/"+std::to_string(progress_loop_number_)+"\r";
    else if (percentage_ == 100)
        progress_line_ = std::to_string(percentage_) + "% [" + progress_bar_ + "]                     "+std::to_string(loop_counter_)+"/"+std::to_string(progress_loop_number_)+"\r";
    else
        progress_line_ = ' ' + std::to_string(percentage_) + "% [" + progress_bar_ + "]                     "+std::to_string(loop_counter_)+"/"+std::to_string(progress_loop_number_)+"\r";
}
