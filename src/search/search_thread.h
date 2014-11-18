#ifndef SEARCH_THREAD_H
#define SEARCH_THREAD_H

#include <vector>
#include <queue>
#include <mutex>
#include <kazbase/unicode.h>
#include <thread>
#include <kazbase/file_utils.h>
#include <kazbase/regex.h>
#include <kazbase/os.h>

struct Match {
    int line;
    int start_col;
    int end_col;
};

struct Result {
    unicode filename;
    std::vector<Match> matches;
};

class SearchThread {
public:
    SearchThread(const std::vector<unicode>& files_to_search, const unicode& search_text, bool is_regex):
        files_to_search_(files_to_search),
        search_text_(search_text),
        is_regex_(is_regex) {

        is_running_ = true;
        thread_ = std::thread(std::bind(&SearchThread::run, this));
    }

    bool running() const { return is_running_; }
    void stop() { is_running_ = false; }
    void join() { thread_.join(); }

    void run() {
        /*
         *  Runs the main search loop. Goes through the specified files, and generates
         *  matches. These are added to a queue which can be accessed from the main thread
         *  through pop_result
         */
        unicode search = search_text_;

        if(is_regex_) {
            search = regex::escape(search);
        }

        regex::Regex re(search);

        while(!files_to_search_.empty()) {
            if(!is_running_) {
                break;
            }

            unicode file = files_to_search_.back();
            files_to_search_.pop_back();

            if(!os::path::exists(file)) {
                continue;
            }

            auto data = file_utils::read(file);
            auto matches = re.search(data);

            for(auto match: matches) {
                Result new_result;

                //TOOD: Populate new_result
                new_result.filename = file;

                push_result(new_result);
            }
        }

        is_running_ = false;
    }

    std::shared_ptr<Result> pop_result() {
        std::lock_guard<std::mutex> lock(lock_);

        if(results_.empty()) {
            return std::shared_ptr<Result>();
        }

        auto res = std::make_shared<Result>(results_.front());
        results_.pop();
        return res;
    }

private:
    std::mutex lock_;

    std::vector<unicode> files_to_search_;
    unicode search_text_;
    bool is_regex_;
    bool is_running_;

    std::queue<Result> results_;

    void push_result(Result new_result) {
        std::lock_guard<std::mutex> lock(lock_);
        results_.push(new_result);
    }

    std::thread thread_;

};

#endif // SEARCH_THREAD_H
