#pragma once

#include <string>
#include <chrono>
#include <cstdio>

using namespace std::chrono;


namespace xwb
{

class TimeCost
{
public:
    TimeCost(const std::string& tag)
        :_tag(tag)
    {
        //构造函数 TimeCost(const std::string& tag): 接受一个标记 tag，用于标识计时器，同时记录当前时间点为代码块开始时间。
        start_tp = high_resolution_clock::now();
    }
    //析构函数 ~TimeCost(): 在代码块执行结束时被调用，获取当前时间点，计算时间差，并打印出代码块执行时间和标记。
    ~TimeCost()
    {
        high_resolution_clock::time_point end_tp = high_resolution_clock::now();
        duration<double> time_span = duration_cast<duration<double>>(end_tp - start_tp);
        double cost = time_span.count();
        printf("[TIME] %.8f: %s\n", cost, _tag.c_str());
    }


private:
    high_resolution_clock::time_point start_tp;
    std::string _tag;
};

void setDebugFilename(const std::string& filename);
std::string getDebugFilename();
std::string getSaveFilename(const std::string& ext);

} // namespace xwb


#define TIME_COST_FUNCTION xwb::TimeCost a(__FUNCTION__);
#define TIME_COST_FUNCTION_LINE xwb::TimeCost a(std::string(__FUNCTION__) + "_" + std::to_string(__LINE__));