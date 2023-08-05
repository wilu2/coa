#include "XChar.h"
#include <vector>
#include "str_utils.h"

namespace xwb
{

std::string statusString(const CharStatus& cs)
{
    switch (cs)
    {
    case NORMAL:
        return "NORMAL";
        break;
    case INSERT:
        return "INSERT";
        break;
    case DELETE:
        return "DELETE";
        break;
    case CHANGE:
        return "CHANGE";
        break;
    default:
        return "NORMAL";
        break;
    }
}


XChar::XChar()
{
    page_index = -1;
    line_index = -1;
    char_status = NORMAL;
}

void XChar::parse(my_json& js, int idx)
{//提取char_positions数组的第idx个元素，并将其转换为std::vector<int>类型的变量polygon。char_positions数组中存储了字符的位置信息。
    std::vector<int> polygon = js["char_positions"][idx].get<std::vector<int> >();
    box = BoundBox::toBoundBox(polygon);//这行代码使用BoundBox类的静态成员函数toBoundBox，将上面获取到的位置信息polygon转换为BoundBox对象，并将其赋值给XChar类的成员变量box。box代表字符的边界框。
    pos_list.push_back(box);//这行代码将刚刚解析得到的字符边界框box添加到XChar类的成员变量pos_list中。pos_list是一个存储了字符边界框的列表。
    // std::vector<float> candidates_score = js["char_candidates_score"][idx].get<std::vector<float> >();
    // candi_score.push_back(candidates_score);

    // std::vector<std::string> candidates = js["char_candidates"][idx].get<std::vector<std::string> >();
    // std::vector<std::wstring> wcandiates(candidates.size());
    // for(int i =0; i < wcandiates.size(); i++)
    // {
    //     wcandiates[i] = from_utf8(candidates[i]);
    // }
    // candi.push_back(wcandiates);
}


void XChar::merge(const XChar& c)
{
    text += c.text;
    box.merge(c.box);
    // candi.insert(candi.end(), c.candi.begin(), c.candi.end());
    // candi_score.insert(candi_score.end(), c.candi_score.begin(), c.candi_score.end());
    pos_list.insert(pos_list.end(), c.pos_list.begin(), c.pos_list.end());
}
    

bool XChar::can_merge(const XChar& c)
{
    if(page_index != c.page_index || line_index != c.line_index)
        return false;

    if(box.is_same_line(c.box))
        return true;

    return false;
}


} // namespace xwb
