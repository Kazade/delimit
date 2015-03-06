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
#include <kazbase/logging.h>

struct Match {
    int line;
    int start_col;
    int end_col;
    unicode text;
};

struct Result {
    unicode filename;
    std::vector<Match> matches;
};

class SearchThread {
public:
    SearchThread(const std::vector<unicode>& files_to_search,
                 const unicode& search_text,
                 bool is_regex,
                 const std::string& within_directory=""):
        files_to_search_(files_to_search),
        within_directory_(within_directory),
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

            if(!within_directory_.empty() && !file.starts_with(within_directory_)) {
                // Ignore files if they aren't within the specified directory
                continue;
            }

            if(!os::path::exists(file)) {
                continue;
            }

            try {
                std::string enc;
                auto data = file_utils::read(file, &enc);
                auto matches = re.search(data);
                if(!matches.empty()) {
                    L_DEBUG(_u("Found search text in file {0}").format(file));

                    Result new_result;
                    new_result.filename = file;
                    for(auto match: matches) {

                        //TODO: Populate new_result
                        Match new_match;

                        auto prev_newline = match.start();
                        while(prev_newline--) {
                            if(data[prev_newline] == '\n') {
                                break;
                            }
                        }

                        auto next_newline = match.end();
                        while(next_newline < (signed) data.length()) {
                            if(data[next_newline] == '\n') {
                                break;
                            }
                            next_newline++;
                        }

                        new_match.start_col = match.start() - prev_newline;
                        new_match.end_col = match.end() - prev_newline;
                        new_match.line = data.slice(nullptr, match.start()).count("\n");
                        new_match.text = data.slice(prev_newline, next_newline).strip();
                        new_result.matches.push_back(new_match);
                    }
                    push_result(new_result);
                }
            } catch(...) {
                L_INFO(_u("Error searching file {0}").format(file));
                continue;
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
    std::string within_directory_;
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
