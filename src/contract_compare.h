#pragma once

#include <string>

namespace xwb
{

std::string get_version();

struct RuntimeConfig
{
    bool block_compare;
    bool ignore_punctuation;
    std::string punctuation; //一个std::string类型的变量，用于存储指定要忽略的标点符号。
    bool merge_diff;


    RuntimeConfig()
        :block_compare(true), ignore_punctuation(false), merge_diff(true){}
};

//这是一个声明了但未在命名空间内实现的函数。它应该接受一个const std::string&类型的docs参数，
//表示要比较的文档内容，以及一个RuntimeConfig&类型的config参数，表示运行时配置。该函数应该返回一个std::string类型的结果
std::string contract_compare(const std::string& docs, RuntimeConfig& config);

} // namespace xwb
