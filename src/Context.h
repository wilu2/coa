#pragma once

#include <string>
#include <algorithm>
#include "str_utils.h"
#include "contract_compare.h"

namespace xwb
{
    


struct Context
{
    bool block_compare;//用于表示是否进行块级别的比对。
    bool ignore_punctuation;//用于表示是否忽略标点符号。
    std::wstring punctuation;
    bool merge_diff;//一个布尔类型的变量，用于表示是否合并比对结果的差异。

    void ReadRuntimeConfig(RuntimeConfig& config)//用于从传入的 RuntimeConfig 对象中读取配置信息，并根据配置信息更新 Context 对象的成员变量。
    {
        block_compare = config.block_compare;
        ignore_punctuation = config.ignore_punctuation;
        merge_diff = config.merge_diff;
        if(!ignore_punctuation)//如果不忽略标点符号
        {
            ignore_punctuation = true;
            punctuation = L" ";
        }
        else
        {
            if(config.punctuation.empty())
            {
                punctuation = L" !\"#$%&'()*+,-./:：;；<=>?@[\\]^_`{|}~~¢£¤¥§©«®°±·»àáèéìíòó÷ùúāēěīōūǎǐǒǔǘǚǜΔΣΦΩ฿“”‰₣₤₩₫€₰₱₳₴℃℉≈≠≤≥①②③④⑤⑥⑦⑧⑨⑩⑪⑫⑬⑭⑮⑯⑰⑱⑲⑳■▶★☆☑☒✓、。々《》「」『』【】〒〖〗〔〕㎡㎥・･…\r\n";
            }
            else
            {
                punctuation = from_utf8(config.punctuation);
                punctuation += L' ';
            }
        }

        if(punctuation.size() > 1)
        {
            std::sort(punctuation.begin(), punctuation.end());
        }
    }
};


} // namespace xwb
