#include "executor.h"
#include "log.h"
#include "lazy.h"
#include "awaiter.h"
#include <iostream>
#include <fstream>
#include <utility>
#include <sstream>


unsigned int currentTid() 
{
    std::thread::id tid = std::this_thread::get_id();
    std::stringstream ss;
    ss << tid;
    unsigned int id;
    ss >> id;
    return id;
}

std::string readSync(const std::string& file)
{
    info("read file: %s in thread %ud\n", file.c_str(), currentTid());
    std::string content;
    std::ifstream reader(file);
    while (!reader.eof()) {
        std::string tmp;
        reader >> tmp;
        content += tmp;
    }
    return content;
};

std::string reverseSync(const std::string& content)
{
    info("reverse string in thread %ud\n", currentTid());
    std::string reversed;
    for (auto iter = content.rbegin(); iter != content.rend(); ++iter)
    {
        reversed.push_back(*iter);
    }
    return reversed;
};

void toLowerSync(std::string& content) {
    for (auto& c : content)
    {
        c = std::tolower(c);
    }
}

bool writeSync(const std::string& file, const std::string& content)
{
    info("write file: %s, in thread %ud\n", file.c_str(), currentTid());
    std::ofstream ofs(file, std::ios_base::trunc);
    if (!ofs.good())
    {
        return false;
    }
    ofs.write(content.c_str(), content.size());
    return true;
};

Lazy<> coReverse(const std::string& in, const std::string& out)
{
    std::string content = co_await CommonAwaiter(readSync, in);
    info("file content: %s, current thread %ud\n\n", content.c_str(), currentTid());
    
    std::string reversed = co_await CommonAwaiter(reverseSync, content);
    info("reversed content: %s, current thread %ud\n\n", reversed.c_str(), currentTid());

    co_await CommonAwaiter(toLowerSync, std::ref(reversed));
    info("to lower: %s, current thread %ud\n", reversed.c_str(), currentTid());

    bool suc = co_await CommonAwaiter(writeSync, out, reversed);
    info("write result: %s, current thread %ud\n\n", suc ? "success" : "failed", currentTid());
    
    Executor::instance().postMainTask([]()->void {Executor::instance().quit(); });
}

int main(int argc, char* argv[])
{
    std::string in("I:\\VCProjects\\coroutine\\test.txt"), out("I:\\VCProjects\\coroutine\\reverse.txt");
    Lazy<> future = coReverse(in, out);
    info("running in %ud\n", currentTid());
    Executor::instance().run();

    system("pause");
}