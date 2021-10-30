#include "CppProgressBar.h"

void CppProgressBar::init_variable(size_t loop_number) {
    progress_loop_number_ = loop_number;
    percentage_ = 0;
    oneHundledth_ = loop_number / 100.0;
    preWinSize_ = nowWinSize_ = 108;
    progress_bar_.resize(nowWinSize_ - 8, '.');
    empty_line_.resize(nowWinSize_ - 1, ' ');
    empty_line_ += '\r';
//    if (nowWinSize_ < 107) {
//        progress_bar_.resize(nowWinSize_ - 8, '.');
//        empty_line_.resize(nowWinSize_ - 1, ' ');
//        empty_line_ += '\r';
//    } else {
//        progress_bar_.resize(100, '.');
//        empty_line_.resize(107, ' ');
//        empty_line_ += '\r';
//    }
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
        progress_line_ = "  " + std::to_string(percentage_) + "% \[" + progress_bar_ + "] "+std::to_string(loop_counter_)+"/"+std::to_string(progress_loop_number_)+"\r";
    else if (percentage_ == 100)
        progress_line_ = std::to_string(percentage_) + "% \[" + progress_bar_ + "] "+std::to_string(loop_counter_)+"/"+std::to_string(progress_loop_number_)+"\r";
    else
        progress_line_ = ' ' + std::to_string(percentage_) + "% \[" + progress_bar_ + "] "+std::to_string(loop_counter_)+"/"+std::to_string(progress_loop_number_)+"\r";
}

//template <typename T> inline void CppProgressBar::stdout_in_for_progress  (T& e) {
//inline void CppProgressBar::stdout_in_for_progress  (std::string& e) {
//    std::clog << empty_line_;
//    std::cout << e << std::endl;
//    std::clog << progress_line_;
//}

//inline void CppProgressBar::display_progress_bar() {
//    std::clog << "\r" << progress_line_;
//}

//void CppProgressBar::stdout_in_for_progress(std::string &e) {
//    std::clog << empty_line_;
//    std::cout << e;
//    std::clog << progress_line_;
//}

//inline void CppProgressBar::finish_progress_bar()

//template <typename Func>
//void for_progress (size_t loop, Func process) {
//    CppProgressBar cpb;
//    cpb.init_variable(loop);
//    for(cpb.cntSet(0); cpb.cntGet()<loop; cpb.cntIncrement()) {
//        cpb.update_variable();
//        std::string output_string;
//        process(output_string);
//        if(output_string != "") {
//            cpb.stdout_in_for_progress(output_string);
//        }
//        cpb.display_progress_bar();
//    }
//    cpb.finish_progress_bar();
//}
